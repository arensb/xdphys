/******************************************************************
**  RCSID: $Id: drpmproc.c,v 1.4 1998/11/24 19:58:51 cmalek Exp $
** Program: iserver
**  Module: drpmproc.c
**  Author: mazer
** Descrip: standalone drpm process code
**
** Revision History (most recent last)
**
** Tue Jun  9 11:59:15 1992 mazer
**  creation date :: de novo
**
** Sat Jun 20 16:57:47 1992 mazer
**  eliminated pipe support and changed to use sigpause() if possible
**
** Fri May 21 13:15:19 1993 mazer
**  added ability to take raw binary data from file into either
**  a or b channels (but not both) and to set the debugflag via
**  an environment var
**
** Tue Jun  1 18:59:00 1993 mazer
**  added -noio flag for use w/o attaching to dsp
**
** Wed Jul 28 12:23:11 1993 mazer
**  updated to use the non-alpha release of the BCE drp library
**  and new s56x device driver. Can still be compiled for old
**  library and driver operation by #def'ing ALPHA
**
** Mon Aug 30 18:03:47 1993 mazer
**  misc cleanup -- trying to figure out why A and B are being
**  swapped on input..
**
** Wed Sep  1 22:02:52 1993 mazer
**  Cleaned up a bit. Everything now works reliably with the
**  current version of the BCE tools (as far as I can tell).
**  "Current version" means the 28-Aug-93 disks Mike Peck
**  fedex'd Allison + the libdrp/ssirp.asm patch I recieved
**  from Phil Lapsley last night.
**
** Sun Sep 26 21:28:50 1993 mazer
**  added DAQ_LOOPBACK option
**
** Mon Jun 13 16:18:33 1994 mazer
**  added -breaklock option
**
** Tue Oct 11 14:38:19 1994 mazer
**  added SAUDIO stuff to put out synchronized pulses on the 8bit
**  ulaw /dev/audio line (to make drpmproc+)
**
** Wed Oct 12 17:23:10 1994 mazer
**  added -syncpulse option
**
*******************************************************************/

/*
** Key environment vars:
**  S56DSP             -- required by the BCE drpmlib (points to *.lod files)
**  DAQ_DEBUG     -- if set, forced debugflag on
**  DAQ_FAKE_A    -- if set, points to binary file to read for A chn
**  DAQ_FAKE_B    -- if set, points to binary file to read for B chn
**   .. note set either DAQ_FAKE_A or DAQ_FAKE_B, but not both
**  DAQ_REBOOT    -- if set, force reboot each time 'round shm_loop
**  DAQ_LOOPBACK  -- if set, copies output->input buffers, no io
**  DAQ_SYNCPULSE -- if set, force -syncpulse option..
**
** Command line options:
**  -fc	hz		sampling freq
**  -dashmid id		shared memory id for DA segment
**  -adshmid id		shared memory id for AD segment
**  -out n		size of DA segment
**  -in	n		size of AD segment
**  -16			assume shared memory is 16bit buffer (optimal)
**  -32			assume shared memory is 32bit buffer (non-optimal)
**  -swap		swap l and r on output (fix for an s56 bug)
**  -sig pid		parent's proc id for signaling
**  -unlock		override lock
**  -breaklock		delete/break lock and exit
**  -nice pri		set nice level to pri during collections
**  -debug		set debug mode
**  -noio		don't attach to the dsp
**  -reboot		reboot each time 'round shm_loop (VERY SLOW)
**  -syncpulse		generate sync pulses on /dev/audio
**			(this is only available if compiled with -DSAUDIO)
**
** Method of operation:
**   stdin   --> NOT USED
**   stdout  --> communication line TO the parent (error returns & pid info)
**   stderr  --> console i/o
**   SIGUSR1 --> buffers are ready, play and record
**   SIGUSR2 --> cleanup and exit
**
*/

#include <stdio.h>
#include <sys/fcntl.h>

#include <signal.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <malloc.h>


extern char *shmat();
extern int shmdt();

#include <s56dspUser.h>
#include <qckMon.h>
#include <drp.h>
#include <drp_rpm.h>

#include "daq.h"
#include "port.h"

#ifdef SAUDIO  
# include "saudio.h"
#endif

#define DRPMLOCK "/tmp/DrpmLock"

static int    debugflag = 0;	/* report debugging information */
static int    parent_pid = -1;	/* pid of parent process */
static int    bits_per_samp = 16; /* 16 (sun-short) or 32 (sun-long) */
static int    swap_ab_out = 0;	/* swap left and right on output */
static int    reboot_mode = 0;	/* == 0, don't qckBoot dsp each shm_loop */
static int    loopback_mode = 0; /* copy output->input instead of play/rec */
static int    fc = 48000;	/* sampling frequency in Hz */
static int    nice_value = 0;	/* optional nice value root only */
static int    no_io = 0;	/* don't try to do actual i/o */
#ifdef SAUDIO
static int    sync_pulse = 0;	/* send syncpulse out using /dev/audio */
#endif

static int    dacnt;		/* number of da samples */
static short *dabuf = NULL;	/* 16bit da buffer */
static int   *dabuf_int = NULL;	/* optional 32bit da buffer */
static int    da_shmid = -1;	/* handle for da shared memory */

static int    adcnt;		/* number of ad samples */
static short *adbuf = NULL;	/* 16bit ad buffer */
static int   *adbuf_int = NULL;	/* optional 32bit ad buffer */
static int    ad_shmid = -1;	/* handle for ad shared memory */

static Drp   *drp = NULL;	/* handle for drp device */
static Drp_rpm rpm;		/* handle for to rpm structure */

static char  *dummydata = NULL;	/* get data from file?? */
static int    dummydata_on_a;	/* load dummy data into a channel? */

static char  *lodfil = "lib/ssirp.lod";

static int start_flag = 0;	/* start flag (raised on SIGUSR1) */

static char  errbuf[256];

static void get_dummydata(short *ptr, int nsamps, int skip)
{
  int i;
  static FILE *fp = NULL;

  if (ptr == NULL) {
    if (fp != NULL) {
      fclose(fp);
    }
  } else {
    if (fp == NULL) {
      if ((fp = fopen(dummydata, "r")) == NULL) {
	perror(dummydata);
	exit(1);
      } else {
	fprintf(stderr, "drpmproc: beginning of %s\n", dummydata);
      }
    }
    for (i = 0; i < nsamps; i++) {
    retry:
      if (fread(ptr + (skip * i), sizeof(short), 1, fp) != 1) {
	fclose(fp);
	if ((fp = fopen(dummydata, "r")) == NULL) {
	  perror(dummydata);
	  exit(1);
	} else {
	  fprintf(stderr, "drpmproc: beginning of %s\n", dummydata);
	}
	goto retry;
      }
    }
  }
}


static void choke(label, msg)
     char *label;
     char *msg;
{
  static void cleanup();

  fprintf(stderr, "drpmproc fatal error in '%s'\n%s\n", label, msg);
  fflush(stderr);
  cleanup();
  exit(1);
}

static void open_drp()
{
  if (no_io)
    return;

  if ((drp = drp_bootdsp("qckMon.lod", lodfil)) == NULL)
    choke("open_drp1", "drp_bootdsp failed");
  else if (drp->error[0])
    choke("open_drp2", drp->error);
  
  if (drp_confio(drp, 16, 128*1024, 16384) == -1)
    choke("open_drp3", drp->error);

  if (drp_confperiph(drp, "proport", fc, /*ignored*/ 1, DRP_DIR_RECPLAY) == -1)
    choke("open_drp4", drp->error);

  if (drp_confmem(drp, drp->dspwords/2, drp->dspwords/2) == -1)
    choke("open_drp5", drp->error);

  if (debugflag)
    fprintf(stderr, "drpmproc: DSP HAS JUST BEEN BOOTED\n");
}

static void close_drp()
{
  if (drp != NULL && drp->dsp->fd != -1) {
    qckDetach(drp->dsp);
    close(drp->dsp->fd);
    drp->dsp->fd = -1;
  }
}

static void cleanup()
{
  close_drp(drp);

  if (bits_per_samp == 16) {
    if (dabuf != NULL)
      shmdt(dabuf);
    if (adbuf != NULL)
      shmdt(adbuf);
  } else {
    free(dabuf);
    free(adbuf);
    if (dabuf_int != NULL)
      shmdt(dabuf_int);
    if (adbuf_int != NULL)
      shmdt(adbuf_int);
  }
  unlink(DRPMLOCK);
  get_dummydata(NULL, 0, 0);	/* maybe clean up.. */
}

static int boink(drp, fc, dabuf, dacnt, adbuf, adcnt)
     Drp *drp;
     int fc;
     short *dabuf;
     int dacnt;
     short *adbuf;
     int adcnt;
{
  int nmissed, missrec, missplay;
  int old_pri;
  int mlocked_adbuf = 0;
  int mlocked_dabuf = 0;

  if (no_io)
    return(IS_TRANS_OK);

  drp_rpm_constructor(drp, &rpm);

  /* 16=bits/sample, 1=stero (0=mono), fc=samples/sec */
  drp_stream_constructor(drp, &rpm.rec_stream, 16, 1, fc);
  if (drp_stream_config_buf(drp, &rpm.rec_stream, -adcnt,
			    (char *) adbuf, NULL, 0) == -1) {
    sprintf(errbuf, "drp_stream_config_buf: input side failed");
    return(IS_TRANS_FAIL);
  }

  /* 16=bits/sample, 1=stero (0=mono), fc=samples/sec */
  drp_stream_constructor(drp, &rpm.play_stream, 16, 1, fc);
  if (drp_stream_config_buf(drp, &rpm.play_stream, -dacnt,
			    (char *)dabuf, NULL, 0) == -1) {
    sprintf(errbuf, "drp_stream_config_buf: output side failed");
    return(IS_TRANS_FAIL);
  }
  rpm.play_stream.buflen = 2 * dacnt * sizeof(short);

  if (debugflag)
    fprintf(stderr, "drpmproc: built rpm stuff\n");
  
  if (geteuid() == 0) {		/* root? */
    if (nice_value != 0) {
      errno = 0;
      old_pri = getpriority(PRIO_PROCESS, 0);
      if (errno != 0) {
	old_pri = -99;
      } else {
	if (setpriority(PRIO_PROCESS, 0, nice_value) < 0) {
	  if (debugflag) 
	    perror("setpriority");
	} else {
	  if (debugflag)
	    fprintf(stderr, "drpmproc: setpri=%d\n", nice_value);
	}
      }
    }
  } else {
    static int first_warning = 1;
    if (first_warning) {
      fprintf(stderr, "Warning: drpmproc is NOT suid root\007\n");
      first_warning = 0;
    }
  }

  if (drp_start(drp, DRP_DIR_RECPLAY) == -1) {
    sprintf(errbuf, "drp_start: failed on %s", drp->error);
    return(IS_TRANS_FAIL);
  }

  if (geteuid() == 0) {
    if (mlock(adbuf, adcnt * 2 * sizeof(short)) == -1 )
      fprintf(stderr, "drpmproc: can't mlock adbuf\n");
    else {
      if (debugflag)
	fprintf(stderr, "drpmproc: locked adbuf\n");
      mlocked_adbuf = 1;
    }
    if (mlock(dabuf, dacnt * 2 * sizeof(short)) == -1 )
      fprintf(stderr, "drpmproc: can't mlock dabuf\n");
    else {
      if (debugflag)
	fprintf(stderr, "drpmproc: locked dabuf\n");
      mlocked_dabuf = 1;
    }
  }

#ifdef SAUDIO
  if (sync_pulse)
    sparc_audio_play_click(5);
#endif

  if (drp_rec_play_mem(drp, &rpm) == -1) {
    sprintf(errbuf, "drp_rec_play_mem: %s", drp->error);
    return(IS_TRANS_FAIL);
  }
  if (drp_end(drp) == -1) {
    sprintf(errbuf, "drp_end: %s", drp->error);
    return(IS_TRANS_FAIL);
  }
#ifdef SAUDIO
  if (sync_pulse)
    sparc_audio_shutdown();
#endif

  if (geteuid() == 0) {		/* root? */
    if (nice_value != 0 && old_pri != -99) {
      if (setpriority(PRIO_PROCESS, 0, old_pri) < 0) {
	if (debugflag)
	  perror("setpriority");
      }
    }
    if (mlocked_adbuf)
      munlock(adbuf, adcnt * 2 * sizeof(short));
    if (mlocked_dabuf)
      munlock(dabuf, dacnt * 2 * sizeof(short));
  }

  nmissed = drp_recplay_missed(drp, &missrec, &missplay);

  if (nmissed == -1) {
    sprintf(errbuf, "drp_missed: %s", drp->error);
    return(IS_TRANS_FAIL);
  } else if (nmissed != 0) {
    sprintf(errbuf, "WARNING: missed %d play, %d record samples",
	    missplay, missrec);
    return(IS_TRANS_MISSED);
  }

  drp_stream_destructor(&rpm.rec_stream);
  drp_stream_destructor(&rpm.play_stream);

  return(IS_TRANS_OK);
}

static void check_parent()
{
  if (kill(parent_pid, 0) != 0) {
    if (debugflag)
      fprintf(stderr, "drpmproc: parent died --  cleaning & exiting\n");
    cleanup();
    if (da_shmid >= 0)
      if (shmctl(da_shmid, IPC_RMID, NULL) != -1)
	if (debugflag)
	  fprintf(stderr, "drpmproc: IPC_RMID'd floating da_shm seg\n");
    if (ad_shmid >= 0)
      if (shmctl(ad_shmid, IPC_RMID, NULL) != -1)
	if (debugflag)
	  fprintf(stderr, "drpmproc: IPC_RMID'd floating ad_shm seg\n");
    exit(1);
  } else {
    alarm(5);
  }
}

static void sigusr1_handler()
{
  if (debugflag) {
    fprintf(stderr, "drpmproc: recieved go signal...\n");
  }
  start_flag = 1;
  signal(SIGUSR1, sigusr1_handler);
}

static void sigusr2_handler()
{
  cleanup();
  if (debugflag) {
    fprintf(stderr, "drpmproc: recieved exit signal...exiting\n");
  }
  exit(0);
}

static void sigfatal_handler(sig, code, scp, addr)
     int sig, code;
     struct sigcontext *scp;
     char *addr;
{
  char *p;

  switch (sig)
    {
    case SIGHUP:	p = "SIGHUP"; break;
    case SIGQUIT:	p = "SIGQUIT"; break;
    case SIGBUS:	p = "SIGBUS"; break;
    case SIGSEGV:	p = "SIGSEGV"; break;
    case SIGTERM:	p = "SIGTERM"; break;
    default:		p = NULL; break;
    }
  if (p == NULL)
    fprintf(stderr, "drpmproc: recieved fatal signal #%d...cleaning\n", sig);
  else
    fprintf(stderr, "drpmproc: recieved fatal signal %s...cleaning\n", p);
  fflush(stderr);
  sleep(1);			/* give parent a second to die then cleanup */
  cleanup();			/* cleanup shared memory */
  fprintf(stderr, "drpmproc: resending fatal signal -- check /tmp for core\n");
  signal(sig, SIG_DFL);		/* restore default handler */
  				/* returning will retry and possibly */
  return;			/* dump core.. ending up in /tmp/core */
}


static void shm_loop()
{
  int ret;
  int i, j;
  short t;


  if (!reboot_mode)
    open_drp();

  while (1) {
    if (reboot_mode)
      open_drp();

    kill(parent_pid, SIGUSR2);		/* send ready signal */

    if (debugflag)
      fprintf(stderr, "drpmproc: waiting for start_flag to go high\n");

    while (!start_flag)
      if (sigpause(0) < 0)		/* wait for a signal to come in */
    	usleep((unsigned)500);		/* sigpause failed, usleep instead */

    /* ok to reset the start_flag, since another one shouldn't arrive until
     * after done-signal and result code is sent back to parent */
    start_flag = 0;

    if (debugflag)
      fprintf(stderr, "drpmproc: start_flag is now high..\n");
    
    if (bits_per_samp == 16) {
      if (swap_ab_out) {
	if (debugflag)
	  fprintf(stderr, "drpmproc: swapping DA %d short samps\n", dacnt);
	for (j = i = 0; i < dacnt; i++, j += 2) {
	  t = dabuf[j];
	  dabuf[j] = dabuf[j + 1];
	  dabuf[j + 1] = t;
	}
      }

      if (dummydata == NULL && !loopback_mode) {
	ret = boink(drp, fc, dabuf, dacnt, adbuf, adcnt); /* KABOOM */
      } else {
	if (loopback_mode) {
	  bcopy((char *)dabuf, (char *)adbuf,
		sizeof(int) * (i = (2 * (adcnt < dacnt ? adcnt : dacnt))));
	  fprintf(stderr, "drpmproc: copied %d samples\n", i);
	} else if (dummydata_on_a) {
	  get_dummydata(adbuf + 0, adcnt, 2);
	} else {
	  get_dummydata(adbuf + 1, adcnt, 2);
	}
	ret = IS_TRANS_OK;
      }

      if (swap_ab_out) {
	if (debugflag)
	  fprintf(stderr, "drpmproc: reswapping DA %d short samps\n", dacnt);
	for (j = i = 0; i < dacnt; i++, j += 2) {
	  t = dabuf[j];
	  dabuf[j] = dabuf[j + 1];
	  dabuf[j + 1] = t;
	}
      }

      if (ret == IS_TRANS_OK) {
	printf("%d :Ok\n", ret);
	if (debugflag)
	  fprintf(stderr, "<%d :Ok>\n", ret);
      } else {
	printf("%d :%s\n", ret, errbuf);
	if (debugflag)
	  fprintf(stderr, "<%d :%s>\n", ret, errbuf);
      }
      fflush(stdout);
      kill(parent_pid, SIGUSR1);
    } else {
      if (swap_ab_out) {
	if (debugflag)
	  fprintf(stderr, "drpmproc: swapping int data\n");
	for (i = 0; i < dacnt; i++) {
	  j = i * 2;
	  dabuf[j] = dabuf_int[j + 1];
	  dabuf[j + 1] = dabuf_int[j];
	  }
      } else {
	j = dacnt * 2;
	for (i = 0; i < j; i++)
	  dabuf[i] = dabuf_int[i];
      }

      ret = boink(drp, fc, dabuf, dacnt, adbuf, adcnt);

      j = adcnt * 2;
      for (i = 0; i < j; i++)
	adbuf_int[i] = adbuf[i];

      if (ret == IS_TRANS_OK) {
	printf("%d :Ok\n", ret);
	if (debugflag)
	  fprintf(stderr, "<%d :Ok>\n", ret);
      } else {
	printf("%d :%s\n", ret, errbuf);
	if (debugflag)
	  fprintf(stderr, "<%d :%s>\n", ret, errbuf);
      }
      fflush(stdout);
      kill(parent_pid, SIGUSR1);
    }
    if (reboot_mode)
      close_drp();
  }
}

static void breaklock()
{
  if (unlink(DRPMLOCK) == 0)
    fprintf(stderr, "drpmproc: deleted lock\n");
  else
    fprintf(stderr, "drpmproc: can't delete lock\n");
}


static int is_locked()
{
  struct stat buf;
  FILE *fp;
  int pid;

  if (stat(DRPMLOCK, &buf) == 0) {
    /* lock file exists, try to see if process is still around.. */
    if ((fp = fopen(DRPMLOCK, "r")) == NULL) {
      /* can't open lock, assume it's locked */
      return(666);
    } else if (fscanf(fp, "%d", &pid) != 1) {
      fprintf(stderr, "drpmproc: invalid lockfile format\n");
      fclose(fp);
      /* invalid lock file format! */
      return(666);
    } if (kill(pid, 0) != 0) {
      /* proc no longer exists, so lock is invalid */
      fclose(fp);
      fprintf(stderr, "drpmproc: lock holder died\n");
      unlink(DRPMLOCK);
      return(0);
    } else {
      /* proc exists, no lock IS valid */
      return(pid);
    }
  } else {
    /* no lock file at all, so it's available */
    return(0);
  }
}

static void write_lockinfo(fp)
     FILE *fp;
{
  fprintf(fp, "%d holds drpm\n", getpid());
  fprintf(fp, "STARTING PARAMETERS\n");
  fprintf(fp, "parent_pid = %d\n", parent_pid);
  fprintf(fp, "bits_per_samp = %d\n", bits_per_samp);
  fprintf(fp, "reboot_mode = %d\n", reboot_mode);
  fprintf(fp, "fc = %d\n", fc);
  fprintf(fp, "dacnt = %d\n", dacnt);
  fprintf(fp, "adcnt = %d\n", adcnt);
  fprintf(fp, "da_shmid = %d\n", da_shmid);
  fprintf(fp, "ad_shmid = %d\n", ad_shmid);
  fprintf(fp, "nice_value = %d\n", nice_value);
  fprintf(fp, "no_io = %d\n", no_io);
  fprintf(fp, "debugflag = %d\n", debugflag);
  fprintf(fp, "lodfil = %s\n", lodfil);
}


int main(ac, av)
     int ac;
     char **av;
{
  int	i,  unlock;
  char  *p;
  FILE  *fp;
  extern char *getenv();

#ifdef RECON_ERR
  /* connect stderr to the console
   */

  if ((fd = open("/dev/console", O_WRONLY)) >= 0) {
    close(2);			/* close stderr */
    dup2(fd, 2);		/* dup /dev/console to stderr */
    close(fd);			/* close direct handle one /dev/console */
  }
#endif

  /* check for DAQ_DEBUG environment var
   */
  
  if (getenv("DAQ_DEBUG") != NULL)
    debugflag = 1;

  if (getenv("DAQ_REBOOT") != NULL)
    reboot_mode = 1;

  if ((p = getenv("DAQ_LODFIL")) != NULL)
    lodfil = p;

  if ((p = getenv("DAQ_LOOPBACK")) != NULL) {
    loopback_mode = 1;
    fprintf(stderr, "drpmproc: in loopback_mode\n");
  }

  /*  cd to /tmp: this little safety feature is to allow core
   *  dumps (god forbid) and other files not to collide with
   *  the main application's crashes!
   */

  if (chdir("/tmp") != 0) {
    perror("chdir");
    fprintf(stderr, "warning: drpmproc can't chdir /tmp\n");
  }

  adcnt = dacnt = 0;
  unlock = 0;

  for (i = 1; i < ac; i++) {
    if (strcmp(av[i], "-fc") == 0) {
      fc = atoi(av[++i]);
    } else if (strcmp(av[i], "-dashmid") == 0) {
      da_shmid = atoi(av[++i]);
    } else if (strcmp(av[i], "-adshmid") == 0) {
      ad_shmid = atoi(av[++i]);
    } else if (strcmp(av[i], "-in") == 0) {
      adcnt = atoi(av[++i]);
    } else if (strcmp(av[i], "-out") == 0) {
      dacnt = atoi(av[++i]);
    } else if (strcmp(av[i], "-16") == 0) {
      bits_per_samp = 16;
    } else if (strcmp(av[i], "-32") == 0) {
      bits_per_samp = 32;
    } else if (strcmp(av[i], "-swap") == 0) {
      swap_ab_out = 1;
    } else if (strcmp(av[i], "-syncpulse") == 0) {
#ifdef SAUDIO
      sync_pulse = 1;
#else
      fprintf(stderr, "drpmproc: -syncpulse option ignored\n");
#endif
    } else if (strcmp(av[i], "-reboot") == 0) {
      reboot_mode = 1;
    } else if (strcmp(av[i], "-sig") == 0) {
      parent_pid = atoi(av[++i]);
    } else if (strcmp(av[i], "-unlock") == 0) {
      unlock = 1;
    } else if (strcmp(av[i], "-breaklock") == 0) {
      breaklock();
      exit(0);
    } else if (strcmp(av[i], "-noio") == 0) {
      no_io = 1;
    } else if (strcmp(av[i], "-nice") == 0) {
      nice_value = atoi(av[++i]);
      if (nice_value < -20)
	nice_value = -20;
      else if (nice_value > 19)
	nice_value = 19;
    } else if (strcmp(av[i], "-debug") == 0) {
      debugflag = 1;
    } else if (strcmp(av[i], "-v") == 0) {
#ifdef BUILDAT
      fprintf(stderr, "drpmproc: %s\n", BUILDAT);
#endif
    } else {
      fprintf(stderr, "drpmproc: unknown flag -- %s\n", av[i]);
      exit(1);
    }
  }

  if (getenv("DAQ_SYNCPULSE") != NULL) {
#ifdef SAUDIO
    sync_pulse = 1;
#else
    fprintf(stderr, "drpmproc: $DAQ_SYNCPULSE setting ignored\n");
#endif
  }

  if (debugflag) {
    fprintf(stderr, "\ndrpmproc: started in debug mode\n");
    for (i = 0; i < ac; i++)
      fprintf(stderr, "drpmproc: %s\n", av[i]);
  }

  if ((dummydata = (char *)getenv("DAQ_FAKE_A")) != NULL) {
    if (debugflag)
      fprintf(stderr, "drpmproc: taking A data from %s\n", dummydata);
    dummydata_on_a = 1;
  } else if ((dummydata = (char *)getenv("DAQ_FAKE_B")) != NULL) {
    if (debugflag)
      fprintf(stderr, "drpmproc: taking B data from %s\n", dummydata);
    dummydata_on_a = 0;
  }

  /* now look for a lock file or make one .. this can deadlock!
   *
   * if there is a lockfile, return a negative pid and exit,
   * parent's responsible for clearing the lock.
   */
  if (debugflag) {
    fprintf(stderr, "drpmproc: debugmode -- ignoring lock status\n");
    write_lockinfo(stderr);
  } else {
    if (!no_io) {
      int lholder;
      if (!unlock && (lholder = is_locked())) {
	printf("%d\n", -getpid());	/* tell parent: my -pid */
	printf("%d\n", lholder); 	/* tell parent: lockholder's pid  */
	fflush(stdout);
	exit(1);
      }
      if ((fp = fopen(DRPMLOCK, "w")) != NULL) {
	write_lockinfo(fp);
	fclose(fp);
      } else {
	printf("%d\n", 0);		/* tell parent there's a problem! */
	fprintf(stderr, "drpmproc: can't unlock drpm\n");
	exit(1);
      }
    }
  }

  /* periodicaly check to make sure the parent's still alive */
  if (parent_pid < 0) {
    choke("main", "drpmproc: must specify parent using -sig <pid>");
  } else {
    signal(SIGALRM, check_parent); /* handler re-signal()s */
    alarm(5);
  }

  /* catch exit signals from parent -- only happends once*/
  signal(SIGUSR2, sigusr2_handler);

  if (da_shmid < 0)
    choke("main", "drpmproc: must use da_shmid parent using -dashmid <id>");
  if (ad_shmid < 0)
    choke("main", "drpmproc: must use da_shmid parent using -adshmid <id>");

  if ((p = (char *)getenv("S56DSP")) == NULL)
    choke("main", "drpmproc: please set environment var S56DSP");

  /*
   * just in case, if any of these things happen, cleanup to avoid
   * leaving shm in use..
   */
  if (!debugflag) {
    signal(SIGHUP, 	sigfatal_handler);
    signal(SIGQUIT,	sigfatal_handler);
    signal(SIGBUS, 	sigfatal_handler);
    signal(SIGSEGV,	sigfatal_handler);
    signal(SIGTERM,	sigfatal_handler);
  }

  if (bits_per_samp == 16) {
    if ((int)(dabuf = (short *)shmat(da_shmid, (char *)0, 0)) == -1) {
      perror("dabuf");
      cleanup();
      exit(1);
    }
    
    if ((int)(adbuf = (short *)shmat(ad_shmid, (char *)0, 0)) == -1) {
      perror("adbuf");
      cleanup();
      exit(1);
    }

  } else {
    if ((dabuf = (short *) malloc(2 * sizeof(short) * dacnt)) == NULL) {
      perror("dabuf");
      cleanup();
      exit(1);
    }

    if ((adbuf = (short *) malloc(2 * sizeof(short) * adcnt)) == NULL) {
      perror("adbuf");
      cleanup();
      exit(1);
    }

    if ((int)(dabuf_int = (int *)shmat(da_shmid, (char *)0, 0)) == -1) {
      perror("dabuf_int");
      cleanup();
      exit(1);
    }

    if ((int)(adbuf_int = (int *)shmat(ad_shmid, (char *)0, 0)) == -1) {
      perror("adbuf_int");
      cleanup();
      exit(1);
    }
  }

  /* clear start_flag, which will be raised by sigusr1_handler */
  start_flag = 0;

  /* now can catch go signals from the parent -- handler re-signal()s */
  signal(SIGUSR1, sigusr1_handler);

  printf("%d\n", getpid());	/* tell parent my pid */
  fflush(stdout);

  if (debugflag)
    fprintf(stderr, "drpmproc: mypid = %d\n", getpid());

  shm_loop();			/* loop until quit signal recieved */
  cleanup();			/* should NEVER happen.. */
  exit(0);
}
