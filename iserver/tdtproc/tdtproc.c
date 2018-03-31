/*
**	This file is part of XDowl
**	California Institute of Technology
*/

/******************************************************************
**  RCSID: $Id: tdtproc.c,v 1.29 2001/03/27 07:02:51 cmalek Exp $
** Program: xdphys
**  Module: tdtproc.c
**  Author: cmalek, mazer
** Descrip: standalone Tucker Davis process code
**
** Environment vars:
** ----------------------
**  DAQ_DEBUG     -- if set, forced debugflag on
**  DAQ_NOIO      -- if set, don't actually talk to daq hardware
**  DAQ_TIME      -- if set, print timestamps for certain events
**   .. NOTE: set either DAQ_FAKE_A or DAQ_FAKE_B, but not both
**                     da/ad
**  DAQ_VERBOSE      -- if set, do verbose syslog reporting
**  DAQ_AD2_USE_SH   -- if set, set AD2 to use sample and hold sampling
**  DAQ_AD2_CHANS    -- set to "<chan1>,<chan2>" to use chan1 and chan2 for AD2
**  DAQ_DA_DUMP      -- write DA buffers to files, one per channel
**  DAQ_AP2_DA_DUMP  -- read from AP2 play buffers, and write them to files, 
**                      one per channel
**
** Command line switches:
** ----------------------
**  -help               print usage message
**  -adshmid <id>       shared memory id for AD segment
**  -adnchans <nchans>	# ad channels (must be either 2 or 4)
**  -in	<nsamples>      # samples in ONE ad channel
**
**  -dashmid <id>       shared memory id for DA segment
**  -out <nsamples>	    # samples in ONE da channel
**
**  -et1shmid <id>      shared memory id for spike segment
**  -et1cnt <nspikes>   size of spike segment 
**
**  -cntrlshmid <id>    shared memory id for control segment
**  -useatt	[1|0]       use PA4's, if they exist
**
**  -fc	<hz>            sampling freq
**  -sig <pid>          parent's proc id for signaling
**  -nice <priority>    set nice level to <priority> during conversion
**
**  -unlock             override lock
**  -breaklock          delete/break lock and exit
**
**  -debug		        set debug mode
**  -devices		    show all TDT devices detected by tdtproc, and exit
**  -noio		        don't attach to the dsp
**  -verbose	        use more verbose syslog reporting
**  -version	        print version info, and exit
**
**  -syncpulse	        Generate a sync pulse on TG6 output #3 for Axoclamp and
**				        or oscilloscope, etc. Only used if -et1 is also used.
**                      (used by xdphys)
**  -split		        split play/rec cycle into separate load, play, rec 
**                      ops	(used by Exp)
**
**  -16	                no longer supported (ignored)
**  -32	                no longer supported (ignored)
**  -swap				no longer supported (ignored)	
**  -reboot		        no longer supported (ignored)
**
**
** Method of operation:
**   stdin   --> NOT USED
**   stdout  --> communication line TO the parent (error returns & pid info)
**   stderr  --> debug and error i/o
**   SIGHUP  --> If is_split=0, load, play, and record. If is_split=1, just load.
**   SIGUSR1 --> If is_split=1, play. If is_split=0, nothing.
**   SIGUSR2 --> If is_split=1, rec. If is_split=0, nothing.
**   SIGQUIT --> cleanup and exit
**
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>

#include <string.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sched.h>
#include <asm/system.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <syslog.h>
#include <assert.h>

#include "xbdrv.h"
#include "apos.h"
#include "daq.h"

/* ------------------------------------------------------------------ */
/* Macros                                                             */
/* ------------------------------------------------------------------ */
#define TDTLOCK "/tmp/TdtLock"

#define AP2_PLAY1     1     	/* Setup logical name for play buffer 1 */
#define AP2_PLAY2     2     	/* Setup logical name for play buffer 2 */
#define AP2_REC1      3     	/* Setup logical name for rec buffer 1 */
#define AP2_REC2      4     	/* Setup logical name for rec buffer 2 */
#define AP2_REC3      5     	/* Setup logical name for rec buffer 2 */
#define AP2_REC4      6     	/* Setup logical name for rec buffer 2 */
#define AP2_SILENCE   7     	/* Setup logical name for silence buffer */
#define CHAN_SPEC     8     	/* Channel record for mrecord() */

#define AD_LEFT_FILENAME "left_ad.out"
#define DA_LEFT_FILENAME "left_da.out"
#define AD_RIGHT_FILENAME "right_ad.out"
#define DA_RIGHT_FILENAME "right_da.out"


#define LEFT		0
#define RIGHT		1

#ifndef COMPILE_DATE
#define COMPILE_DATE			" "
#endif /*COMPILE_DATE*/
#ifndef OSINFO
#define OSINFO				" "
#endif /*OSINFO*/

#define VERSION 					"1.1"
#define TIMESTAMP		if (show_time) fprintf

#define send_ready_signal(x) kill(x, SIGUSR2)
#define send_done_signal(x) kill(x, SIGUSR1)

/* ------------------------------------------------------------------ */
/* Prototypes                                                         */
/* ------------------------------------------------------------------ */
static void detect_devices(void);
static void check_devices(void);
static void print_devices(void);
static void add_device(int);
static void initialize_tdt(void);
static inline void allocate_buffers(void);
#if(0)
static inline void deallocate_buffers(void);
static inline void check_da_buflen(void);
#endif
static void detach_from_equip(void);
static void clean_exit(int);
static inline int doplay(void);
static inline void dorecord(void);
static inline void increase_priority(void);
static inline void reset_priority(void);
static void check_parent(int sig);
static void simulate_recording_simple(void);
static inline void trigger_equipment(void);
static void wait_loop(void);
static void breaklock(void);
static int is_locked(void);
static void write_lockinfo(FILE *);
static void check_env_vars(void);
static void parse_command_line(int, char **);
static void setup_sighandlers(void);
static void map_shared_mem(void);
static void lock_tdtproc(void);
static int root_check(void);
void dump_dabufs_to_file(xword *,xword *,int);
static inline void check_ap2_play_buffers(void);
static inline void deinterlace_dabuf(short *, int);
static inline void interlace_adbuf(short *, int);
static inline void setup_equipment(void);
static inline void setup_ap2_play_buffers(void);
static inline void retrieve_rec_buffers(void);
static inline void arm_equipment(void);
static inline void get_spikes(void);
static inline void program_dd1(void);
static inline void program_tg6(void);
static inline void program_et1(void);
static inline void program_da1(void);
static inline void program_ad2(void);
static inline void program_pa4(void);
static double timestamp(void);
static void usage(void);
static void parse_ad2_env(char *, int *, int *);

/* ------------------------------------------------------------------ */
/* Global Variables                                                   */
/* ------------------------------------------------------------------ */
static struct devices {
	int	pa4;
	int da1;
	int dd1;
	int ad2;
	int et1;
	int sd1;
	int tg6;
	int da;
	int ad;
} tdt_devices = {0,0,0,0,0,0,0,0,0};
static char *rcs_revision = "$Revision: 1.29 $";
static char *rcs_name = "$Name:  $";
static char *rcs_date = "$Date: 2001/03/27 07:02:51 $";
static char *rcs_path = "$Source: /home/konishi11/cvsroot/iserver/tdtproc/tdtproc.c,v $";

static int    debugflag = 0;	/* report debugging information */
static int    show_time = 0;	/* show timestamps for certain events */
static int    is_split = 0;	    /* is_split play/rec cycle into load/play/rec 
								   events */
static int    verbose = 0;	    /* reporting mode: 1 = print more messages */

static int    parent_pid = -1;	/* pid of parent process */
static int    bits_per_samp = 16; /* 16 (sun-short) or 32 (sun-long) */
static int    swap_channels = 0;	/* swap left and right on output */
static int    reboot_mode = 0;	/* == 0, don't qckBoot dsp each wait_loop */
static int    fc = 48000;	/* sampling frequency in Hz */
static int    nice_value = 0;	/* optional nice value root only */
static int    old_pri = 0;	/* old priority, used by increase_priority() */
static int    no_io = 0;	/* don't try to do actual i/o */
static int    unlock = 0;	
static int    sync_pulse = 0;	
static int    devices_only = 0;	
static int    dump_da = 0;	 /* dump DA buffers to files */
static int    dump_ap2_da = 0;	 /* dump AP2 DA buffers to files */

static int    use_et1 = 0;	/* Use the ET1 event timer? */
static int    et1cnt = 0;	/* # points in the et1 shared mem buffer */
static unsigned long   *et1buf = NULL;	/* pointer to the et1 shared mem buffer */
static int    et1_shmid = -1;	/* shared mem ID of the et1 buffer */

static int    use_att = 0;	/* Use the PA4 attenuators ? */
static int	  control_shmid = -1;   /* shared mem ID of the attenuator buffer */
static TDTcontrol  *controlbuf = NULL;   /* pointer to the tdtcontrol shared mem buffer */

static int    dacnt = 0;			/* number of da samples in ONE channel */
static short *dabuf = NULL;			/* 16bit da buffer */
static int    da_shmid = -1;		/* handle for da shared memory */

static int    adcnt = 0;		/* number of ad samples in ONE channel*/
static int    ad_nchans = 2;	/* number of ad channels to use */
static short *adbuf = NULL;	/* 16bit ad buffer */
static int    ad_shmid = -1;	/* handle for ad shared memory */

static float    fc_actual;	/* our actual sampling frequency  */

static volatile sig_atomic_t play_flag = 0;	/* start flag (raised on SIGUSR1) */
static volatile sig_atomic_t load_flag = 0;	/* start flag (raised on SIGHUP) */
static volatile sig_atomic_t rec_flag = 0;	/* start flag (raised on SIGUSR2) */

static int tdt_inited = 0;


/* ------------------------------------------------------------------ */
/* Functions                                                          */
/* ------------------------------------------------------------------ */
/* main                                                               */
/*                                                                    */
/*   Parse the command line options, and environment variables.       */
/*   Register the signal handlers, set up the parent checking alarm,  */
/*   and attach to the shared memory.                                 */
/*                                                                    */
/* Returns:                                                           */ 
/*                                                                    */ 
/*    Nothing.                                                        */
/*                                                                    */
/* ------------------------------------------------------------------ */

int main(int ac, char **av)
{

	openlog("tdtproc", LOG_PID, LOG_USER);

	/* can't access AP2 and XBDRV stuff if we're not root */
	if (!root_check())
		clean_exit(1);


#ifdef DO_CHDIR
	/*  cd to /tmp: this little safety feature is to allow core
	 *  dumps (god forbid) and other files not to collide with
	 *  the main application's crashes!
	 */

	if (chdir("/tmp") != 0) {
		syslog(LOG_WARNING, "chdir: %m");
	}
#endif /* DO_CHDIR */

	check_env_vars();
	parse_command_line(ac, av);
	initialize_tdt();
	lock_tdtproc();
	map_shared_mem();
	setup_sighandlers();
	setup_equipment();

	assert(tdt_inited == 1);

	printf("%d\n", getpid());	/* tell parent my pid */
	fflush(stdout);

	/* tell our actual sampling frequency */
	printf("%d\n", (int) (fc_actual+0.5));	
	fflush(stdout);

	DEBUG(stderr, "  tdtproc: mypid = %d\n", getpid());

	syslog(LOG_WARNING, "tdtproc started successfully");

	wait_loop();

	clean_exit(0);			/* should NEVER happen.. */

	exit(0);
}

/* ================================================================== */
/* Signal Handlers                                                    */
/* ------------------------------------------------------------------ */
/* check_parent                                                       */
/*                                                                    */
/*   This check is performed via alarm() every 5 seconds.	          */
/* ------------------------------------------------------------------ */

static void check_parent(int sig)
{

	if (kill(parent_pid, 0) != 0) {

		errno=0;

		syslog(LOG_ERR, "parent died --  cleaning & exiting");

		if (da_shmid >= 0) {
			if (shmctl(da_shmid, IPC_RMID, NULL) != -1)
				DEBUG(stderr, "  tdtproc: IPC_RMID'd floating da_shm seg\n");
			else 
				syslog(LOG_ERR, "unable to remove play shmem: %m");
		}

		if (ad_shmid >= 0) {
			if (shmctl(ad_shmid, IPC_RMID, NULL) != -1)
				DEBUG(stderr, "  tdtproc: IPC_RMID'd floating ad_shm seg\n");
			else 
				syslog(LOG_ERR, "unable to remove rec shmem: %m");
		}

		if (control_shmid >= 0) {
			if (shmctl(control_shmid, IPC_RMID, NULL) != -1)
				DEBUG(stderr, 
					"  tdtproc: IPC_RMID'd floating control_shm seg\n");
			else 
				syslog(LOG_ERR, "unable to remove control_shm shmem: %m");
			}

		if (tdt_devices.et1) {
			if (use_et1) {
				if (et1_shmid >= 0) {
					if (shmctl(et1_shmid, IPC_RMID, NULL) != -1)
						DEBUG(stderr, 
							"  tdtproc: IPC_RMID'd floating et1_shm seg\n");
					else 
						syslog(LOG_ERR, "unable to remove et1 shmem: %m");
				}
			}
		}

		clean_exit(1);

	} else {
		signal(SIGALRM, check_parent); 
		alarm(5);
	}
}

/* ------------------------------------------------------------------ */
/* sigusr1_handler                                                    */
/*                                                                    */ 
/*    Parent sends SIGUSR1 to tdtproc to tell it to start A/D-D/A     */ 
/*    conversion.                                                     */ 
/*                                                                    */ 
/* ------------------------------------------------------------------ */

void sigusr1_handler(int sig)
{
	DEBUG(stderr, "  tdtproc: received play signal...\n");
	fflush(stderr);

	TIMESTAMP(stderr, "tdtproc: recieved play signal: %f\n", timestamp());
	if (load_flag != 1 && load_flag != 1)
		play_flag = 1;
	else 
		syslog(LOG_ERR, "  recv'd play signal when not ready");

	signal(SIGUSR1, sigusr1_handler);
}

/* ------------------------------------------------------------------ */
/* sigusr2_handler                                                    */
/*                                                                    */ 
/*    Parent sends SIGUSR2 to tdtproc to tell it to retrieve recorded */ 
/*    data from the AP2 rec buffers                                   */
/*                                                                    */ 
/* ------------------------------------------------------------------ */

void sigusr2_handler(int sig)
{
	DEBUG(stderr, "  tdtproc: received rec signal...\n");
	fflush(stderr);

	if (play_flag != 1 && load_flag != 1)
		rec_flag = 1;
	else 
		syslog(LOG_ERR, "  recv'd rec signal when not ready");

	signal(SIGUSR2, sigusr2_handler);
}

/* ------------------------------------------------------------------ */
/* sighup_handler                                                     */
/*                                                                    */ 
/*    Parent sends SIGHUP to tdtproc to tell it to load a new         */
/*    stimulus into the play buffers, and start playback.             */
/*                                                                    */ 
/* ------------------------------------------------------------------ */

void sighup_handler(int sig)
{
	if (is_split) {
		DEBUG(stderr, "  tdtproc: received load signal...\n");
		fflush(stderr);
	} else {
		DEBUG(stderr, "  tdtproc: received go signal...\n");
		fflush(stderr);
	}

	if (!is_split) {
		play_flag = 1;
		rec_flag = 1;
	}

	if (!is_split || (is_split && play_flag != 1 && rec_flag != 1))
		load_flag = 1;
	else
		syslog(LOG_ERR, "  recv'd load signal when not ready");

	signal(SIGHUP, sighup_handler);
}

/* ------------------------------------------------------------------ */
/* sigquit_handler                                                    */
/*                                                                    */ 
/*    Parent sends SIGQUIT to tdtproc to tell it to quit.             */ 
/*                                                                    */ 
/* ------------------------------------------------------------------ */

void sigquit_handler(int sig)
{
  clean_exit(0);				/* clean_exit shared memory */
}

/* ------------------------------------------------------------------ */
/* sigfatal_handler                                                   */
/* ------------------------------------------------------------------ */

void sigfatal_handler(int sig)
{
  syslog(LOG_ERR, "  received fatal signal: %s", sys_siglist[sig]);

  sleep(1);					/* give parent a second to die then clean_exit */

  clean_exit(1);				/* clean_exit shared memory */

}


/* ------------------------------------------------------------------ */
/* root_check                                                         */
/* ------------------------------------------------------------------ */
static int root_check(void)
{
  if (geteuid() != 0) {		
	  fprintf(stderr, "tdtproc: must be run as ROOT for tdt stuff to work!\n");
	  return(0);
  } else {
	  return(1);
  }
}

/* ================================================================== */
/* Functions visible only to this file below here                     */
/* ------------------------------------------------------------------ */
/* wait_loop                                                          */
/*                                                                    */
/*    This is the main loop once the program is started.  tdtproc     */
/*    waits for a SIGUSR1 to indicate that it should start the next   */
/*    play/record cycle.                                              */
/*                                                                    */
/* ------------------------------------------------------------------ */

#define suspend(err)		{sigemptyset(&mask); \
				if (sigsuspend(&mask) == -1) \
					if (errno != EINTR) \
											syslog(LOG_ERR, "%s: %m",err);}

static void wait_loop(void)
{
	int ret = IS_TRANS_OK;
	sigset_t	mask;
	char  errbuf[256];


	while (1) {
		send_ready_signal(parent_pid);

		DEBUG(stderr, "  tdtproc: waiting for signal\n");

		/* play_flag is set in sighup () */
		while (!play_flag && !load_flag && !rec_flag) 
			suspend("  tdtproc: sigsuspend");

		alarm(0);

		/* ok to reset the play_flag, since another one 
		   shouldn't arrive until after done-signal and result 
		   code is sent back to parent */

		if (load_flag) {
			setup_ap2_play_buffers();
			load_flag = 0;
		}


		if (play_flag) {
			ret = doplay();
			play_flag = 0;
			if (dump_ap2_da)
				check_ap2_play_buffers();
		}



		if (rec_flag) {
			dorecord();
			rec_flag = 0;
		}

		if (ret == IS_TRANS_OK) {
			printf("%d:Ok\n", ret);
			DEBUG(stderr, "<%d :Ok>\n", ret);
		} else {
			printf("%d:%s\n", ret, errbuf);
			DEBUG(stderr, "<%d :%s>\n", ret, errbuf);
		}
		fflush(NULL);

		send_done_signal(parent_pid);

		TIMESTAMP(stderr, "tdtproc: completely done playback: %f\n", 
			  timestamp());

		alarm(5);
	}
}

#undef suspend


/* ------------------------------------------------------------------ */
/* initialize_tdt: main                                               */
/* ------------------------------------------------------------------ */

static void initialize_tdt(void)
{
	char *ap_port;
	int ap_id=APa;

	if (no_io) {
		tdt_inited = 1;
		fc_actual = fc;
		return;
	}
	

	if ((ap_port=getenv("DAQ_AP2_PORT"))!=NULL) {
	  if (!strncmp(ap_port,"apa",3)||!strncmp(ap_port,"APA",3)||
	      !strncmp(ap_port,"APa",3)) {
	    ap_id=APa;
	  } else {
	      if (!strncmp(ap_port,"apb",3)||!strncmp(ap_port,"APB",3)||
		  !strncmp(ap_port,"APb",3)) {
		ap_id=APb;
	      }
	  }
	} else
	  ap_id=APa;
	


	assert(tdt_inited != 1);

	/* Initialize AP2 and XBUS drivers */
	if (!apinit(ap_id)) {
		syslog(LOG_ERR, "Error initializing AP2!");
		clean_exit(1);
	}
	if (!XB1init(ap_id)) {
		syslog(LOG_ERR, "Error initializing XBUS!");
		clean_exit(1);
	}

	detect_devices();

	if (devices_only) {
		clean_exit(0);
	}

	DEBUG(stderr, "  tdtproc: initialized and XBUS\n");

	allocate_buffers();

	tdt_inited = 1;
}

/* ------------------------------------------------------------------ */
/* allocate_buffers: initialize_tdt                                   */
/* ------------------------------------------------------------------ */
static inline void allocate_buffers(void)
{
	assert(dacnt >= 2);
	assert(adcnt >= 2);


	/* Now allocate some playback and record buffers on the AP2 */
	allot16(AP2_PLAY1,dacnt);
	allot16(AP2_PLAY2,dacnt);
	allot16(AP2_REC1,adcnt);
	allot16(AP2_REC2,adcnt);
	if ((tdt_devices.ad == AD2_CODE) && (ad_nchans > 2)) {
		if (verbose)
			syslog(LOG_WARNING, "using 4 A/D channels");
		allot16(AP2_REC3,adcnt); 
		allot16(AP2_REC4,adcnt);

		/* Make the channel specifier record for mrecord() */
		allot16(CHAN_SPEC, 10);
		dpush(10);
		make(0, AP2_REC1);
		make(1, AP2_REC2);
		make(2, AP2_REC3);
		make(3, AP2_REC4);
		make(4, 0);
		qpop16(CHAN_SPEC);
	}

	/* silence buffer used when playing only one channel */
	allot16(AP2_SILENCE,dacnt);
	qpush16(AP2_SILENCE);
	value(0.0);
	qpop16(AP2_SILENCE);

}


/* ------------------------------------------------------------------ */
/* setup_equipment: main                                              */
/* ------------------------------------------------------------------ */
static inline void setup_equipment(void)
{
	if (no_io)
		return;

	assert(tdt_inited == 1);

	DEBUG(stderr, "  tdtproc: setting up equipment\n");

	/* D/A devices */
	switch (tdt_devices.da) {
		case DD1_CODE:
			program_dd1();
			break;
		case DA1_CODE:
			program_da1();
			break;
		default:
			syslog(LOG_ERR, "tdt_devices.da has invalid value!");
			clean_exit(1);
			break;
	}

	/* A/D devices */
	switch (tdt_devices.ad) {
		case DD1_CODE:
			program_dd1();
			break;
		case AD2_CODE:
			program_ad2();
			break;
		default:
			syslog(LOG_ERR, "tdt_devices.ad has invalid value!");
			clean_exit(1);
			break;
	}

	program_pa4();
	program_et1();

	/* We don't have to do anything for the SD1, and the TG6
	   we leave alone until later */
}

/* ------------------------------------------------------------------ */
/* program_dd1: setup_equipment                                       */
/* ------------------------------------------------------------------ */
static inline void program_dd1(void)
{
	double	samp_rate;
	int	mode = 0x00;
	static 	int dd1_inited = 0;

	assert(tdt_inited == 1);

	if (dd1_inited) return;
	assert(tdt_devices.dd1);

	DD1clear(1);
	/* samp_rate has to be in microseconds */
	samp_rate = (1.0 / (double) fc) * 1e6; 
	DEBUG(stderr, "  tdtproc: trying %d Hz as samp freq (%f)\n", 
		fc, samp_rate);
	samp_rate = DD1speriod(1,samp_rate);
	fc_actual = 1.0/(samp_rate/1e6);
	DEBUG(stderr, "  tdtproc: using %.1f Hz as sampling freq\n", fc_actual);

	if (tdt_devices.ad == DD1_CODE)
		mode |= DUALADC;
	else
		DD1clkout(1, XCLK1);

	if (tdt_devices.da == DD1_CODE)
		mode |= DUALDAC;

	DD1mode(1,mode);
	DD1npts(1,dacnt);
	DD1strig(1);

	dd1_inited = 1;
}

/* ------------------------------------------------------------------ */
/* program_da1: setup_equipment                                       */
/* ------------------------------------------------------------------ */
static inline void program_da1(void)
{
	double	samp_rate;

	assert(tdt_inited == 1);
	assert(tdt_devices.da1);

	DA1clear(1);
	/* samp_rate has to be in microseconds */
	samp_rate = (1.0 / (double) fc) * 1e6; 
	DEBUG(stderr, "  tdtproc: trying %d Hz as DA1 samp freq (%f)\n", 
		fc, samp_rate);
	samp_rate = DA1speriod(1,samp_rate);
	fc_actual = 1.0/(samp_rate/1e6);
	DEBUG(stderr, "  tdtproc: using %.1f Hz as DA1 sampling freq\n", fc_actual);
	DA1mode(1,DUALDAC);
	DA1clkout(1, XCLK1);
	DA1npts(1,dacnt);
	DA1strig(1);

}

/* ------------------------------------------------------------------ */
/* program_ad2: setup_equipment                                       */
/* ------------------------------------------------------------------ */
static inline void program_ad2(void)
{
	double	samp_rate;
	char *chans;
	int	i;
	int	chan_a, chan_b;

	assert(tdt_inited == 1);
	assert(tdt_devices.ad2);

	AD2clear(1);
	samp_rate = (1.0 / (double) fc) * 1e6; 
	DEBUG(stderr, "  tdtproc: trying %d Hz as AD2 samp freq (%f)\n", 
		fc, samp_rate);
	samp_rate = AD2speriod(1,samp_rate);
	fc_actual = 1.0/(samp_rate/1e6);
	DEBUG(stderr, "  tdtproc: using %.1f Hz as AD2 sampling freq\n", 
		fc_actual);
	if (getenv("DAQ_AD2_USE_SH") != NULL) {
		AD2sh(1,1);
		syslog(LOG_WARNING, "AD2 sample/hold ON");
	} else
		AD2sh(1,0);
	if (ad_nchans > 2)
		AD2mode(1, ADC1+ADC2+ADC3+ADC4);
	else {
		if ((chans=getenv("DAQ_AD2_CHANS")) == NULL) 
			AD2mode(1, DUALADC);
		else {
			parse_ad2_env(chans, &chan_a, &chan_b);
			AD2mode(1,chan_a+chan_b);
		}
	}

	AD2clkin(1, XCLK1); /* AD2 uses either DD1's or DA1's 
						   clock to sync samples */
	for (i=1; i<5; i++)
		AD2gain(1, i, x1);
	AD2npts(1,adcnt);
	AD2strig(1);

}

/* ------------------------------------------------------------------ */
/* parse_ad2_env:                                                     */
/* ------------------------------------------------------------------ */
static void parse_ad2_env(char *chans, int *chan_a, int *chan_b)
{
	int a,b;
	char buf[50];

	sscanf(chans,"%d,%d",&a, &b);
	if (a<1) a=1;
	if (a>4) a=4;
	if (b<1) b=1;
	if (b>4) b=4;
	if ((a==b) && (a==4)) 
		a=3;
	else
		if ((a==b) && (a==1)) 
			b=2;
		else
			if (a==b)
				b++;
	sprintf(buf, "using channels %d & %d of AD2",a,b);
	syslog(LOG_WARNING, buf);

	switch (a) {
	  case 1: *chan_a=ADC1; break;
	  case 2: *chan_a=ADC2; break;
	  case 3: *chan_a=ADC3; break;
	  case 4: *chan_a=ADC4; break;
	}
	switch (b) {
	  case 1: *chan_b=ADC1; break;
	  case 2: *chan_b=ADC2; break;
	  case 3: *chan_b=ADC3; break;
	  case 4: *chan_b=ADC4; break;
	}
}

/* ------------------------------------------------------------------ */
/* program_tg6: setup_equipment                                        */
/* ------------------------------------------------------------------ */
static inline void program_tg6(void)
{
	double samp_rate;
	int dur_ms;

	assert(tdt_inited == 1);

	if (!tdt_devices.tg6)
		return;

	/* samp_rate has to be in microseconds */
	samp_rate = (1.0 / (double) fc) * 1e6;
	dur_ms = (int) ((samp_rate / 1e3) * (double) (dacnt));

	DEBUG(stderr, "  tdtproc: buffer length is %d ms\n", dur_ms);

	/* New timing seq. #0, (dur_ms+20) ms long */
	TG6clear(1);
	TG6baserate(1, _1ms);
	TG6new(1, 0, dur_ms + 20, 0x00);
	if (sync_pulse) {
		/* Set output #3 high for 0 to 10 ms */
		TG6high(1, 0, 0, 10, 0x04);
		TG6low(1, 0, 10, dur_ms+20, 0x04);
	}
	if (controlbuf->use_led) {
		/* output #4 controls an LED panel */
		/* Set to high for led_delay->(led_delay+led_dur) */
		DEBUG(stderr, "  tdtproc: using led: dur=%d delay=%d\n",
		      controlbuf->led_dur, controlbuf->led_delay);
		TG6high(1, 0, controlbuf->led_delay,
		      controlbuf->led_delay + controlbuf->led_dur, 0x08);
	}
	if (tdt_devices.et1 && use_et1) {
		/* Set outputs #1 and #2 high for 0->(dur_ms) ms */
		TG6high(1, 0, 0, dur_ms, 0x03); 
		TG6low(1, 0, dur_ms, dur_ms+20, 0x03); 

	} else {
		/* Set output #1 high for 0->(dur_ms) ms */
		TG6high(1, 0, 0, dur_ms, 0x01); 
		TG6low(1, 0, dur_ms, dur_ms+20, 0x01); 
	}
}

/* ------------------------------------------------------------------ */
/* program_pa4: setup_equipment                                       */
/* ------------------------------------------------------------------ */
static inline void program_pa4(void)
{
	assert(tdt_inited == 1);

	if (!tdt_devices.pa4) return;

	if (use_att) {
		PA4man(1);
		if (tdt_devices.pa4 == 2) 
			PA4man(2);
	}
}

/* ------------------------------------------------------------------ */
/* program_et1: setup_equipment                                       */
/* ------------------------------------------------------------------ */
static inline void program_et1(void)
{
	assert(tdt_inited == 1);

	if (!tdt_devices.et1) return;

	if (use_et1)
		ET1clear(1);
}

/* ------------------------------------------------------------------ */
/* detach_from_equip: clean_exit                                      */
/* ------------------------------------------------------------------ */
static void detach_from_equip(void)
{
	if (no_io) return;

	if (tdt_inited) {
		DEBUG(stderr, "  tdtproc: closing down equipment\n");

		if (tdt_devices.et1)
			if (use_et1)
				ET1clear(1);

		if (tdt_devices.tg6)
			TG6clear(1);

		if (tdt_devices.dd1) {
			DD1stop(1);
			DD1clear(1);
		}

		if (tdt_devices.da1) {
			DA1stop(1);
			DA1clear(1);
		}

		if (tdt_devices.ad2) {
			AD2stop(1);
			AD2clear(1);
		}
	}
}

/* ------------------------------------------------------------------ */
/* doplay: wait_loop                                                  */
/*                                                                    */
/*   The AP2 play buffers are AP2_PLAY1 and AP2_PLAY2.                */
/*   The AP2 rec buffers are AP2_REC1 and AP2_REC2.                   */
/*                                                                    */
/*   Be sure to check that play and record durations are the same     */
/*   before doing dplay() and drecord()                               */
/*                                                                    */
/* ------------------------------------------------------------------ */

static inline int doplay(void)
{


	if (no_io) {
		simulate_recording_simple();
		return(IS_TRANS_OK);
	}

	assert(tdt_inited == 1);

	increase_priority();

	DEBUG(stderr, "  tdtproc: starting play/record cycle\n");

	arm_equipment();

#if(0)
	check_da_buflen();
#endif

	controlbuf->timestamp = timestamp();

	TIMESTAMP(stderr, "tdtproc: before trigger: %f\n", timestamp());
	trigger_equipment();
	TIMESTAMP(stderr, "tdtproc: after trigger: %f\n", timestamp());

	DEBUG(stderr, "  tdtproc: triggered!\n");

	reset_priority();

	DEBUG(stderr, "  tdtproc: done play/record cycle\n");


	DEBUG(stderr, "  tdtproc: done playback\n");
	fflush(stderr);

	return(IS_TRANS_OK);
}

#if(0)
/* ------------------------------------------------------------------ */
/* deallocate_buffers: check_da_buflen                                */
/* ------------------------------------------------------------------ */
static inline void deallocate_buffers(void)
{
	trash();
}

/* ------------------------------------------------------------------ */
/* check_da_buflen                                                    */
/* ------------------------------------------------------------------ */
static inline void check_da_buflen(void) 
{
	static int numsounds=0;
	int buflen1, buflen2 = 0;;

	qpush16(AP2_PLAY1);
	buflen1=topsize();
	dropall();
	qpush16(AP2_PLAY2);
	buflen2=topsize();
	dropall();

	fprintf(stderr,"%d: tdtproc dacnt = %d, AP2 dacnt = (%d + %d)\n", 
		numsounds, dacnt, buflen1, buflen2);

	if ((buflen1 != dacnt) || (buflen2 != dacnt)) {
		deallocate_buffers();
		allocate_buffers();

		qpush16(AP2_PLAY1);
		buflen1=topsize();
		dropall();
		qpush16(AP2_PLAY2);
		buflen2=topsize();
		dropall();
		fprintf(stderr,"%d: now AP2 dacnt = (%d + %d)\n", numsounds, buflen1, 
			buflen2);
	}

	numsounds++;

}
#endif

/* ------------------------------------------------------------------ */
/* trigger_equipment                                                  */
/* ------------------------------------------------------------------ */
static inline void trigger_equipment(void) 
{
	if (tdt_devices.tg6) {
		TG6go(1);
	} else {
		XB1gtrig();
	}
}

/* ------------------------------------------------------------------ */
/* dorecord: wait_loop                                                */
/* ------------------------------------------------------------------ */
static inline void dorecord(void) 
{
	if (no_io)
		return;

	if (tdt_devices.tg6)
		do {} while (TG6status(1) != 0);

	switch (tdt_devices.da) {
		case DD1_CODE:
			assert(tdt_devices.dd1);
			do {} while (DD1status(1) != 0);
			break;
		case DA1_CODE:
			assert(tdt_devices.da1);
			do {} while (DA1status(1) != 0);
			break;
		default:
			syslog(LOG_ERR, "dorecord: invalid da device code");
			clean_exit(1);
	}
	
	if (tdt_devices.et1)
		if (use_et1)
			get_spikes();

	retrieve_rec_buffers();

	DEBUG(stderr, "  tdtproc: done recording\n");
	fflush(stderr);

}

/* ------------------------------------------------------------------ */
/* setup_ap2_play_buffers: play_record                                */
/* ------------------------------------------------------------------ */
static inline void setup_ap2_play_buffers(void)
{
	static short *left_dabuf = NULL;
	static short *right_dabuf = NULL;

	if (no_io)
		return;

	/* keep buffers allocated to save time each cycle:
	   does it save much time? Probably not */
	if (left_dabuf == NULL) {
		left_dabuf = (short *) calloc(dacnt, sizeof(short));
		assert(left_dabuf != NULL);
	} else 
		left_dabuf = memset(left_dabuf, 0, dacnt*sizeof(short));

	if (right_dabuf == NULL) {
		right_dabuf = (short *) calloc(dacnt, sizeof(short));
		assert(right_dabuf != NULL);
	} else
		right_dabuf = memset(right_dabuf, 0, dacnt*sizeof(short));

	/* xdphys synth stuff is set up to write to interlaced buffers,
	   i.e. LRLRLRLR..., dd1 wants separate Left and Right buffers */
	deinterlace_dabuf(left_dabuf, LEFT);
	deinterlace_dabuf(right_dabuf, RIGHT);

	if (dump_da)
		dump_dabufs_to_file(left_dabuf,right_dabuf,dacnt);

#if(0)
	/* clear the buffers on the AP2 */
	DEBUG(stderr, "tdtproc: clearing AP2 buffers\n");
	dpush(dacnt);
	value(0.0);
	qpop16(AP2_PLAY1);

	dpush(dacnt);
	value(0.0);
	qpop16(AP2_PLAY2);
#endif

	assert(left_dabuf != NULL);
	push16(left_dabuf, dacnt);
	qpop16(AP2_PLAY1);

	assert(right_dabuf != NULL);
	push16(right_dabuf, dacnt);
	qpop16(AP2_PLAY2);

	DEBUG(stderr, "tdtproc: loaded play buffers\n");
	fflush(stderr);

}

/* ------------------------------------------------------------------ */
/* arm_equipment: play_record                                          */
/* ------------------------------------------------------------------ */
static inline void arm_equipment(void)
{
	int dd1_armed = 0;

	program_tg6();

	TIMESTAMP(stderr, "tdtproc: set attenuators: %f\n", timestamp());
	if (tdt_devices.pa4) {
		if (use_att) {
			PA4atten(1, controlbuf->att[0]);
			DEBUG(stderr, "  tdtproc: set attenuation to (%.1f, %.1f)\n",
				controlbuf->att[0], controlbuf->att[1]);

			if (tdt_devices.pa4 == 2) {
				PA4atten(2, controlbuf->att[1]);
				DEBUG(stderr, "  tdtproc: set attenuation to (%.1f)\n",
					controlbuf->att[0]);
			}
		}
	}

	TIMESTAMP(stderr, "tdtproc: post-atten, pre-config ad/da: %f\n", 
			  timestamp());

	if (controlbuf->channel == IS_BOTH) {
		DEBUG(stderr, "  tdtproc: playing both channels\n");
		dplay(AP2_PLAY1, AP2_PLAY2);
	} else {
		if (controlbuf->channel == IS_LEFT) {
			DEBUG(stderr, "  tdtproc: playing left channel\n");
			dplay(AP2_PLAY1, AP2_SILENCE);
		} else {
			if (controlbuf->channel == IS_RIGHT) {
				DEBUG(stderr, "  tdtproc: playing right channel\n");
				dplay(AP2_SILENCE, AP2_PLAY2);
			}
		}
	}
	fflush(stderr);

	if ((tdt_devices.ad != AD2_CODE) || (ad_nchans == 2))
		drecord(AP2_REC1, AP2_REC2);
	else
		mrecord(CHAN_SPEC);

	TIMESTAMP(stderr, "tdtproc: post-config, pre-arm ad/da: %f\n", 
			  timestamp());

	switch (tdt_devices.da) {
		case DD1_CODE:
			assert(tdt_devices.dd1);
			DD1arm(1);
			dd1_armed = 1;
			break;
		case DA1_CODE:
			assert(tdt_devices.da1);
			/* program_da1(); */
			DA1arm(1);
			break;
		default:
			syslog(LOG_ERR, "arm D/A: invalid da device code");
			clean_exit(1);
	}

	switch (tdt_devices.ad) {
		case DD1_CODE:
			assert(tdt_devices.dd1);
			if (!dd1_armed)
				DD1arm(1); 
			break;
		case AD2_CODE:
			assert(tdt_devices.ad2);
			AD2arm(1);
			break;
		default:
			syslog(LOG_ERR, "arm A/D: invalid ad device code");
			clean_exit(1);
	}

	if (tdt_devices.et1) 
		if (use_et1) 
			ET1go(1);   

	TIMESTAMP(stderr, "tdtproc: post-arm ad/da: %f\n", 
			  timestamp());

	if (tdt_devices.tg6)
		TG6arm(1,0); 
	else {
		if (tdt_devices.da1)
			DA1tgo(1);
		if (tdt_devices.dd1)
			DD1tgo(1);
		if (tdt_devices.ad2)
			AD2tgo(1);
	}

}

/* ------------------------------------------------------------------ 
   get_spikes: play_record                                           

   		stores # spikes as et1buf[0]
   ------------------------------------------------------------------ */
static inline void get_spikes(void)
{
	static unsigned long *times = NULL;
	int ntimes;

	DEBUG(stderr, "  tdtproc: getting spikes\n");

	ET1stop(1);

	do {} while (ET1active(1) != 0);


	assert(et1buf != NULL);
	memset(et1buf, 0, et1cnt*sizeof(unsigned long));


	/* Get the spike times from the ET1 */
	ntimes = ET1readall32(1, &times);


	if (ntimes > 0) {
		if (ntimes > et1cnt-1) {
			syslog(LOG_ERR, "got more spike times than I could store");
			syslog(LOG_ERR, "gathered %d spikes, keeping %d.\n", ntimes, 
				et1cnt-1);
			ntimes = et1cnt-1;
		}

		et1buf[0] = ntimes;
		memcpy(et1buf+1, times, ntimes*sizeof(unsigned long));
	}
	DEBUG(stderr, "  tdtproc: got %d spikes\n", ntimes);
}

/* ------------------------------------------------------------------ */
/* simulate_recording_simple: doplay                                  */
/* ------------------------------------------------------------------ */

#define SIM_NSPIKES 30
static void simulate_recording_simple(void)
{
	int i;

	/* Just copy the playback bufs to the record bufs */
	assert((adbuf != NULL) && (dabuf != NULL));

	bcopy((char *)dabuf, (char *)adbuf,
		sizeof(short) * (i = (2 * (adcnt < dacnt ? adcnt : dacnt))));
	/* attenuate a little bit */
	for (i=0; i<(2*adcnt); i++)
		adbuf[i]=adbuf[i]*0.9;


	if (use_et1) {
		/* now for some spikes in us*/
		assert(et1buf != NULL);
		memset(et1buf, 0, et1cnt*sizeof(unsigned long));

		et1buf[0] = SIM_NSPIKES;
		for (i=0; i<SIM_NSPIKES; i++) 
			et1buf[2+i]=100000+5000*i;
	}


}


/* ------------------------------------------------------------------ */
/* check_ap2_play_buffers:                                            */
/* ------------------------------------------------------------------ */
static inline void check_ap2_play_buffers(void)
{
	static short *left_dabuf= NULL;
	static short *right_dabuf= NULL;

	if (left_dabuf == NULL) {
		left_dabuf = (short *) calloc(dacnt, sizeof(short));
		assert(left_dabuf != NULL);
	} else
		left_dabuf = memset(left_dabuf, 0, dacnt*sizeof(short));

	if (right_dabuf == NULL) {
		right_dabuf = (short *) calloc(dacnt, sizeof(short));
		assert(right_dabuf != NULL);
	} else
		right_dabuf = memset(right_dabuf, 0, dacnt*sizeof(short));

	qpush16(AP2_PLAY1);
	pop16(left_dabuf);

	qpush16(AP2_PLAY2);
	pop16(right_dabuf);

	dump_dabufs_to_file(left_dabuf,right_dabuf,dacnt);

}

/* ------------------------------------------------------------------ */
/* retrieve_rec_buffers: play_record                                   */ 
/* ------------------------------------------------------------------ */
static inline void retrieve_rec_buffers(void)
{
	static short *temp_adbuf= NULL;

	if (temp_adbuf == NULL) {
		temp_adbuf = (short *) calloc(adcnt, sizeof(short));
		assert(temp_adbuf != NULL);
	} else
		temp_adbuf = memset(temp_adbuf, 0, adcnt*sizeof(short));

	/* dowl synth stuff is set up to deal with interlaced buffers,
	   i.e. LRLRLRLR..., dd1 wants separate Left and Right buffers */
	qpush16(AP2_REC1);
	pop16(temp_adbuf);
	interlace_adbuf(temp_adbuf,AP2_REC1);

	temp_adbuf = memset(temp_adbuf, 0, adcnt*sizeof(short));

	qpush16(AP2_REC2);
	pop16(temp_adbuf);
	interlace_adbuf(temp_adbuf,AP2_REC2);

	if ((tdt_devices.ad == AD2_CODE) && (ad_nchans > 2)) {
		temp_adbuf = memset(temp_adbuf, 0, adcnt*sizeof(short));

		qpush16(AP2_REC3);
		pop16(temp_adbuf);
		interlace_adbuf(temp_adbuf,AP2_REC3);

		temp_adbuf = memset(temp_adbuf, 0, adcnt*sizeof(short));

		qpush16(AP2_REC4);
		pop16(temp_adbuf);
		interlace_adbuf(temp_adbuf,AP2_REC4);
	}

}

/* ------------------------------------------------------------------ */
/* deinterlace_dabuf: setup_ap2_play_buffers                          */
/* ------------------------------------------------------------------ */
static inline void deinterlace_dabuf(
	short *chanbuf,
	int	channel)
{
	int offset, i;
	int j = 0;

	assert(chanbuf != NULL);

	if (channel == LEFT)
		offset = 0;
	else
		offset = 1;

	for (i=offset; i<(2*dacnt); i+=2, j++) 
		chanbuf[j] = dabuf[i];
}

/* ------------------------------------------------------------------ */
/* interlace_adbuf: setup_ap2_play_buffers                            */
/* ------------------------------------------------------------------ */

static inline void interlace_adbuf(
	short *chanbuf,
	int	channel)
{
	int offset, i;
	int j = 0;

	assert(chanbuf != NULL);

	switch (channel) {
		case AP2_REC1:
			offset = 0;
			break;
		case AP2_REC2:
			offset = 1;
			break;
		case AP2_REC3:
			assert(ad_nchans > 2);
			offset = 2;
			break;
		case AP2_REC4:
			assert(ad_nchans > 2);
			offset = 3;
			break;
		default:
			syslog(LOG_ERR, "invalid channel ID in interlace_adbuf()!!");
			clean_exit(1);
			break;
	}

	for (i=offset; i<(ad_nchans*adcnt); i+=ad_nchans, j++) 
		adbuf[i] = chanbuf[j];
}

/* ------------------------------------------------------------------ */
/* increase_priority: play_record                                     */
/* ------------------------------------------------------------------ */

static inline void increase_priority(void)
{
  static int first_warning = 1;

  if (geteuid() == 0) {		/* root? */
	  if (nice_value != 0) {

		  /* we have to do this with errno, since -1 
			 is a valid priority */
		  errno = 0;

		  old_pri = getpriority(PRIO_PROCESS, 1);
		  if (errno != 0) {
			  old_pri = -99;
		  } else {
			  if (setpriority(PRIO_PROCESS, 0, nice_value) < 0) {
				  syslog(LOG_WARNING, "setpriority: %m");
			  } else {
				  DEBUG(stderr, "  tdtproc: setpri=%d\n", nice_value);
			  }
		  }
	  }
  } else {
	  if (first_warning) {
		  syslog(LOG_WARNING, "tdtproc is NOT suid root");
		  first_warning = 0;
	  }
  }


}

/* ------------------------------------------------------------------ */
/* reset_priority: play_record                                        */
/* ------------------------------------------------------------------ */

static inline void reset_priority(void)
{

  if (geteuid() == 0) {		/* root? */
	  if (nice_value != 0 && old_pri != -99) {
		  if (setpriority(PRIO_PROCESS, 0, old_pri) < 0) {
			  syslog(LOG_WARNING, "setpriority");
		  }
	  }
  }
}


/* ------------------------------------------------------------------ */
/* breaklock                                                          */
/* ------------------------------------------------------------------ */

static void breaklock(void)
{
  if (unlink(TDTLOCK) == 0)
    syslog(LOG_INFO, "deleted lock");
  else
    syslog(LOG_WARNING, "can't delete lock: %m");
}


/* ------------------------------------------------------------------ */
/* is_locked                                                          */
/*                                                                    */
/* Returns:                                                           */ 
/*                                                                    */ 
/*   -1:  if a lockfile exists, but we can't parse it, or it has no   */
/*        owner and we can't delete it                                */ 
/*   0:   OK to write our own lockfile                                */
/*   >0:  Pid of owner of existing lockfile.                          */
/*                                                                    */
/* ------------------------------------------------------------------ */

static int is_locked(void)
{
	struct stat buf;
	FILE *fp;
	int pid;

	if (stat(TDTLOCK, &buf) == 0) {
		syslog(LOG_WARNING, "lockfile exists!");
		/* lock file exists, try to see if process is still around.. */
		if ((fp = fopen(TDTLOCK, "r")) == NULL) {
			/* can't open lock, assume it's locked */
			return(-1);
		} else {
			if (fscanf(fp, "%d", &pid) != 1) {
				syslog(LOG_ERR, "can't parse lockfile: invalid format");
				fclose(fp);
				/* invalid lock file format! */
				return(-1);
			}
		}

		if (kill(pid, 0) != 0) {
			/* proc no longer exists, so lock is invalid */
			fclose(fp);
			syslog(LOG_WARNING, "lockfile has no owner, attempting delete");
			if (unlink(TDTLOCK) == -1)
				syslog(LOG_ERR, "can't delete lockfile: %m");
			else
				syslog(LOG_WARNING, "lockfile deleted!");
			return(0);
		} else {
			/* proc exists, no lock IS valid */
			syslog(LOG_WARNING, "lockfile has owner, sending pid to parent");
			return(pid);
		}
	} else {
		/* no lock file at all, so it's available */
		return(0);
	}
}

/* ------------------------------------------------------------------ */
/* write_lockinfo                                                     */
/* ------------------------------------------------------------------ */

static void write_lockinfo(
	FILE *fp)
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
	fprintf(fp, "control_shmid = %d\n", control_shmid);
	fprintf(fp, "nice_value = %d\n", nice_value);
	fprintf(fp, "no_io = %d\n", no_io);
	fprintf(fp, "debugflag = %d\n", debugflag);
	fprintf(fp, "version = %s\n", VERSION);
}


/* ------------------------------------------------------------------ */
/* check_env_vars                                                     */
/* ------------------------------------------------------------------ */

static void check_env_vars(void)
{
	if (getenv("DAQ_DEBUG") != NULL) {
		debugflag = 1;
		DEBUG(stderr, "tdtproc: DAQ_DEBUG acknowledged\n");
	}

	if (getenv("DAQ_NOIO") != NULL) {
		no_io = 1;
		DEBUG(stderr, "tdtproc: DAQ_NOIO acknowledged\n");
	}
	if (getenv("DAQ_TIME") != NULL) {
		show_time = 1;
		DEBUG(stderr, "tdtproc: DAQ_TIME acknowledged\n");
	}

	if (getenv("DAQ_VERBOSE") != NULL) 
		verbose = 1;

	if (getenv("DAQ_DA_DUMP") != NULL) 
		dump_da = 1;

	if (getenv("DAQ_AP2_DA_DUMP") != NULL) 
		dump_ap2_da = 1;

}

/* ------------------------------------------------------------------ */
/* dump_dabufs_to_file                                                  */
/* ------------------------------------------------------------------ */

void dump_dabufs_to_file(xword *left_dabuf,xword *right_dabuf,int nsamples)
{
	int fd;

	if ((fd = creat(DA_RIGHT_FILENAME,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH))==-1) {
		perror("opening right outfile");
		exit(1);
	} else {
		if (write(fd,right_dabuf,nsamples*sizeof(xword))!=(nsamples*sizeof(xword))) {
			perror("writing to right outfile");
			exit(1);
		}
		if (close(fd) == -1) {
			perror("closing right outfile");
			exit(1);
		}
	}

	if ((fd = creat(DA_LEFT_FILENAME,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH))==-1) {
		perror("opening left outfile");
		exit(1);
	} else {
		if (write(fd,left_dabuf,nsamples*sizeof(xword))!=(nsamples*sizeof(xword))) {
			perror("writing to left outfile");
			exit(1);
		}
		if (close(fd) == -1) {
			perror("closing left outfile");
			exit(1);
		}
	}
}


/* ------------------------------------------------------------------------
	parse_command_line
   ------------------------------------------------------------------------ */
#define DEBUG_SWITCH(arg, label, val) DEBUG(stderr, \
	"  tdtproc: parsed %s switch: %s=%d\n", arg, label, val)

static void parse_command_line(
	int		ac,
	char	**av)
{
	char		c;
	static struct option longopts[] = {
		{"adshmid", 1, NULL, 'a'},
		{"useatt", 0, NULL, 'A'},
		{"16", 0, NULL, 'b'},
		{"32", 0, NULL, 'B'},
		{"cntrlshmid", 1, NULL, 'c'},
		{"adnchans", 1, NULL, 'C'},
		{"dashmid", 1, NULL, 'd'},
		{"debug", 0, NULL, 'D'},
		{"et1shmid", 1, NULL, 'e'},
		{"et1cnt", 1, NULL, 'E'},
		{"fc", 1, NULL, 'f'},
		{"in", 1, NULL, 'i'},
		{"noio", 0, NULL, 'I'},
		{"split", 0, NULL, 'l'},
		{"nice", 1, NULL, 'n'},
		{"out", 1, NULL, 'o'},
		{"devices", 0, NULL, 'O'},
		{"sig", 1, NULL, 'p'},
		{"reboot", 0, NULL, 'r'},
		{"swap", 0, NULL, 's'},
		{"syncpulse", 0, NULL, 'S'},
		{"unlock", 0, NULL, 'u'},
		{"breaklock", 0, NULL, 'U'},
		{"version", 0, NULL, 'v'},
		{"verbose", 0, NULL, 'V'},
		{0, 0, 0, 0}};
	int options_index = 0;

	if (ac == 1) 
		usage();

	DEBUG(stderr, "tdtproc: parsing command line options\n");
	while (1) {

		if ((c = getopt_long_only(ac, av, "a:AbBc:C:d:De:E:f:i:Iln:o:Op:rsSuUvV",
			longopts, &options_index)) == -1)
			break;

		switch (c) {
			case 'd':
				da_shmid = atoi(optarg);
			  	DEBUG_SWITCH("-dashmid", "da_shmid", da_shmid);
				break;
			case 'a':
				ad_shmid = atoi(optarg);
				DEBUG_SWITCH("-adshmid", "ad_shmid", ad_shmid);
				break;
			case 'A':
				use_att = 1;
				DEBUG_SWITCH("-useatt", "use_att", use_att);
				break;
			case 'c':
				control_shmid = atoi(optarg);
				DEBUG_SWITCH("-cntrlshmid", "control_shmid", control_shmid);
				break;
			case 'C':
				ad_nchans = atoi(optarg);
				DEBUG_SWITCH("-adnchans", "ad_nchans", ad_nchans);
				break;
			case 'i':
				adcnt = atoi(optarg);
				DEBUG_SWITCH("-in", "adcnt", adcnt);
				break;
			case 'o':
				dacnt = atoi(optarg);
				DEBUG_SWITCH("-out", "dacnt", dacnt);
				break;
			case 'O':
				devices_only = 1;
				DEBUG_SWITCH("-devices", "devices", devices_only);
				break;
			case 'f':
				fc = atoi(optarg);
				DEBUG_SWITCH("-fc", "fc", fc);
				break;
			case 'b':
				bits_per_samp = 16;
				DEBUG_SWITCH("-16", "bits_per_samp", bits_per_samp);
				break;
			case 'B':
				bits_per_samp = 16;
				syslog(LOG_WARNING, 
					"32 bit data transfers not available with TDT eq.\n");
				syslog(LOG_WARNING, 
					"using 16 bit mode.\n");
				DEBUG_SWITCH("-32", "bits_per_samp", bits_per_samp);
				break;
			case 's':
				swap_channels = 1;
				syslog(LOG_WARNING, " -swap option ignored\n");
				DEBUG_SWITCH("-swap", "swap_channels", swap_channels);
				break;
			case 'S':
				sync_pulse = 1;
				DEBUG_SWITCH("-syncpulse", "sync_pulse", sync_pulse);
				break;
			case 'r':
				reboot_mode = 0;
				syslog(LOG_WARNING, " -reboot option ignored\n");
				DEBUG_SWITCH("-reboot", "reboot_mode", reboot_mode);
				break;
			case 'p':
				parent_pid = atoi(optarg);
				DEBUG_SWITCH("-sig", "parent_pid", parent_pid);
				break;
			case 'E':
				use_et1 = 1;
				et1cnt = atoi(optarg);
				DEBUG_SWITCH("-et1cnt", "et1cnt", et1cnt);
				break;
			case 'e':
				use_et1 = 1;
				et1_shmid = atoi(optarg);
				DEBUG_SWITCH("-et1shmid", "et1_shmid", et1_shmid);
				break;
			case 'u':
				unlock = 1;
				DEBUG_SWITCH("-unlock", "unlock", unlock);
				break;
			case 'U':
				syslog(LOG_WARNING, " trying to forcibly remove lock\n");
				breaklock();
				clean_exit(0);
				break;
			case 'I':
				no_io = 1;
				DEBUG_SWITCH("-noio", "no_io", no_io);
				break;
			case 'l':
				is_split = 1;
				DEBUG_SWITCH("-split", "is_split", is_split);
				break;
			case 'n':
				nice_value = atoi(optarg);
				DEBUG_SWITCH("-nice", "nice_value", nice_value);
				if (nice_value < -20)
					nice_value = -20;
				else if (nice_value > 19)
					nice_value = 19;
				break;
			case 'D':
				debugflag = 1;
				DEBUG_SWITCH("-debug", "debugflag", debugflag);
				break;
			case 'v':
				fprintf(stderr, "\ntdtproc Version %s (auto rig)\n", VERSION);
				fprintf(stderr, 
					"----------------------------------------------------\n");
				fprintf(stderr, "Compile Date: %s\n", COMPILE_DATE);
				fprintf(stderr, "OS Info: %s\n", OSINFO);
				fprintf(stderr, 
					"----------------------------------------------------\n");
				fprintf(stderr, "%s\n", rcs_path);
				fprintf(stderr, "%s\n", rcs_name);
				fprintf(stderr, "%s\n", rcs_revision);
				fprintf(stderr, "%s\n", rcs_date);
				fprintf(stderr, 
					"----------------------------------------------------\n");
				clean_exit(0);
				break;
			case 'V':
				verbose = 1;
				DEBUG_SWITCH("-verbose", "verbose", verbose);
				break;
			case '?':
				syslog(LOG_ERR, "unknown flag -- %s\n", av[optind]);
				clean_exit(1);
				break;
		}
	}
}
#undef DEBUG_SWITCH


/* ------------------------------------------------------------------ */
/* setup_sighandlers                                                  */
/* ------------------------------------------------------------------ */
static void setup_sighandlers(void)
{
	if (parent_pid < 0) {
		syslog(LOG_ERR, "must specify parent using -sig <pid>");
		clean_exit(1);
	}

	signal(SIGALRM, check_parent);
	alarm(5);

	signal(SIGQUIT, sigquit_handler);

	signal(SIGBUS, 	sigfatal_handler);
	signal(SIGSEGV,	sigfatal_handler);
	signal(SIGTERM,	sigfatal_handler);

	/* now can catch go signals from the parent -- handler re-signal()s */
	if (is_split) {
		signal(SIGUSR1, sigusr1_handler);
		signal(SIGUSR2, sigusr2_handler);
	}
	signal(SIGHUP, sighup_handler);

	DEBUG(stderr, "  tdtproc: setup signal handlers\n");
}

/* ------------------------------------------------------------------ */
/* map_shared_mem                                                     */
/* ------------------------------------------------------------------ */
static void map_shared_mem(void)
{
	if (use_et1) {
		if (et1_shmid < 0) {
			syslog(LOG_ERR, 
				"must use et1_shmid parent using -et1shmid <id>");
			clean_exit(1);
		}
	}
	if (da_shmid < 0) {
		syslog(LOG_ERR, "must use da_shmid parent using -dashmid <id>");
		clean_exit(1);
	}

	if (ad_shmid < 0) {
		syslog(LOG_ERR, "must use ad_shmid parent using -adshmid <id>");
		clean_exit(1);
	}

	if ((int)(controlbuf = (TDTcontrol *)shmat(control_shmid, (char *)0, 0))
		== -1) {
		syslog(LOG_ERR, "shmat controlbuf: %m");
		clean_exit(1);
	} else 
		DEBUG(stderr, "  tdtproc: got control shmem\n");

	if (use_et1) {
		if ((int)(et1buf = (unsigned long *)shmat(et1_shmid, (char *)0, 0)) == -1) {
			syslog(LOG_ERR, "shmat et1buf: %m");
			clean_exit(1);
		} else 
			DEBUG(stderr, "  tdtproc: got et1 shmem\n");
	} 

    if ((int)(dabuf = (short *)shmat(da_shmid, (char *)0, 0)) == -1) {
		syslog(LOG_ERR, "shmat dabuf: %m");
		clean_exit(1);
    } else 
		DEBUG(stderr, "  tdtproc: got play shmem\n");
    
    if ((int)(adbuf = (short *)shmat(ad_shmid, (char *)0, 0)) == -1) {
		syslog(LOG_ERR,"shmat adbuf: %m");
		clean_exit(1);
    } else
		DEBUG(stderr, "  tdtproc: got rec shmem\n");

}

/* ------------------------------------------------------------------ */
/* lock_tdtproc                                                       */
/* ------------------------------------------------------------------ */
static void lock_tdtproc(void)
{
	FILE  *fp;
	int lholder;

	if (debugflag) {
		fprintf(stderr, "  tdtproc: debugmode -- ignoring lock status\n");
	} else {
		if (!unlock && (lholder = is_locked())) {
				/* tell parent: lockholder's pid  */
			if (lholder != -1) {
				printf("%d\n", -getpid());	/* tell parent: my -pid */
				/* tell parent: lockholder's pid  */
				printf("%d\n", lholder); 	
				fflush(stdout);
			}
			clean_exit(1);
		}

		if ((fp = fopen(TDTLOCK, "w")) != NULL) {
			write_lockinfo(fp);
			fclose(fp);
		} else {
			printf("%d\n", 0);		/* tell parent there's a problem! */
			syslog(LOG_ERR, "can't write lockfile: %m");
			clean_exit(1);
		}
	}
}


/* ------------------------------------------------------------------ */
/* clean_exit                                                            */
/* ------------------------------------------------------------------ */
#define close_fake_data_file() get_fake_dsp_data(NULL, 0, 0)

static void clean_exit(
	int 	exit_code)
{

	detach_from_equip();

	if (dabuf != NULL)
		if (shmdt((char *) dabuf) == -1) 
			syslog(LOG_ERR, "can't shmdt() dabuf: %m");
	if (adbuf != NULL)
		if (shmdt((char *) adbuf) == -1)
			syslog(LOG_ERR, "can't shmdt() adbuf: %m");
	if (controlbuf != NULL)
		if (shmdt((char *) controlbuf) == -1)
			syslog(LOG_ERR, "can't shmdt() controlbuf: %m");
	if (et1buf != NULL)
		if (shmdt((char *) et1buf) == -1)
			syslog(LOG_ERR, "can't shmdt() et1buf: %m");

	if (unlink(TDTLOCK) == -1)
		syslog(LOG_ERR, "can't delete lockfile: %m");


	syslog(LOG_WARNING, "exiting ...");
	closelog();

	exit(exit_code);
}

#undef close_fake_data_file


/* ------------------------------------------------------------------------
   timestamp: generate a timestamp for coil samples
   ------------------------------------------------------------------------ */

static double timestamp(void)
{
	struct timeval	time;
	struct timezone	zone = { 0, 0 };
	double timestamp;

	if (gettimeofday(&time, &zone) == -1) {
		syslog(LOG_ERR, "gettimeofday: %m");
		clean_exit(1);
	}

	timestamp = (double) time.tv_sec + ((double) time.tv_usec)/1000000.0;

	return(timestamp);
}


/* ------------------------------------------------------------------------
   detect_devices: discover which devices are in our TDT racks
   ------------------------------------------------------------------------ */

static void detect_devices(void)
{
	int dev,id; 

	if (no_io) return;

	for (dev=1; dev<=XB_MAX_DEV_TYPES; dev++) {
		for (id=1; id<=XB_MAX_OF_EACH_TYPE; id++) {
			if (xbcode[dev][id][xbsel] != 0) {
				if (verbose)
					syslog(LOG_WARNING, "detected %s_(%d)", xbname[dev], id);
				add_device(dev);
			}
		}
	}

	check_devices();
	print_devices();
}

/* ------------------------------------------------------------------------
   add_device: add a device to the tdt_devices struct
   ------------------------------------------------------------------------ */

static void add_device(int dev) 
{
	switch (dev) {
		case PA4_CODE:
			tdt_devices.pa4++;
			break;
		case SD1_CODE:
			tdt_devices.sd1++;
			break;
		case ET1_CODE:
			tdt_devices.et1++;
			break;
		case TG6_CODE:
			tdt_devices.tg6++;
			break;
		case DA1_CODE:
			tdt_devices.da1++;
			tdt_devices.da = DA1_CODE;
			break;
		case DD1_CODE:
			tdt_devices.dd1++;
			tdt_devices.da = DD1_CODE;
			tdt_devices.ad = DD1_CODE;
			break;
		case AD2_CODE:
			tdt_devices.ad2++;
			tdt_devices.ad = AD2_CODE;
			break;
		default:
			/* if it's not one of the above, we're not 
			   interested in it */
			break;
	}
}

/* ------------------------------------------------------------------------
   print_devices: print a message for each type device detected
   ------------------------------------------------------------------------ */

static void print_devices(void) 
{
	char buf[255];
	char buf2[255];

	sprintf(buf, "tdtproc: detected the following devices: ");
	if (tdt_devices.da1) {
		sprintf(buf2," (%d DA1)", tdt_devices.da1);
		strcat(buf, buf2);
	}
	if (tdt_devices.dd1) {
		sprintf(buf2, " (%d DD1)", tdt_devices.dd1);
		strcat(buf, buf2);
	}
	if (tdt_devices.ad2) {
		sprintf(buf2, " (%d AD2)", tdt_devices.ad2);
		strcat(buf, buf2);
	}
	if (tdt_devices.pa4) {
		sprintf(buf2, " (%d PA4)", tdt_devices.pa4);
		strcat(buf, buf2);
	}
	if (tdt_devices.et1) {
		sprintf(buf2, " (%d ET1)", tdt_devices.et1);
		strcat(buf, buf2);
	}
	if (tdt_devices.et1) {
		sprintf(buf2, " (%d SD1)", tdt_devices.sd1);
		strcat(buf, buf2);
	}
	if (tdt_devices.tg6) {
		sprintf(buf2, " (%d TG6)", tdt_devices.tg6);
		strcat(buf, buf2);
	}
	if (devices_only)
		printf("%s\n",buf);

	switch (tdt_devices.da) {
		case DD1_CODE:
			sprintf(buf, "using DD1 as D/A device");
			break;
		case DA1_CODE:
			sprintf(buf, "using DA1 as D/A device");
			break;
		default:
			syslog(LOG_ERR, "tdt_devices.da has invalid value!");
			clean_exit(1);
			break;
	}
	if (verbose)
		syslog(LOG_WARNING, buf);
	if (devices_only)
		printf("tdtproc: %s\n", buf);


	/* A/D devices */
	switch (tdt_devices.ad) {
		case DD1_CODE:
			sprintf(buf, "using DD1 as A/D device");
			break;
		case AD2_CODE:
			sprintf(buf, "using AD2 as A/D device");
			break;
		default:
			syslog(LOG_ERR, "tdt_devices.ad has invalid value!");
			clean_exit(1);
			break;
	}
	if (verbose)
		syslog(LOG_WARNING, buf);
	if (devices_only) 
		printf("tdtproc: %s\n\n",buf);

}

/* ------------------------------------------------------------------------
   check_devices: check tdt_devices struct to see if we have all the
                  devices we need in order to proceed
   ------------------------------------------------------------------------ */

static void check_devices(void) 
{
	if (tdt_devices.dd1 && tdt_devices.da1) {
		/* Choose DA1 over DD1 for D/A */
		tdt_devices.da = DA1_CODE;
	} 

	if (tdt_devices.dd1 && tdt_devices.ad2) {
		/* Choose AD2 over DD1 for A/D */
		tdt_devices.ad = AD2_CODE;
	} 

	if (!tdt_devices.dd1 && !tdt_devices.da1) {
		syslog(LOG_ERR, "no D/A devices detected! Unable to proceed.");
		if (devices_only)
			printf("tdtproc: no D/A devices detected! Unable to proceed.\n");
		clean_exit(1);
	}

	if (!tdt_devices.dd1 && !tdt_devices.ad2) {
		syslog(LOG_ERR, "no A/D devices detected! Unable to proceed.");
		if (devices_only)
			printf("tdtproc: no A/D devices detected! Unable to proceed.\n");
		clean_exit(1);
	}

	if (!tdt_devices.pa4) {
		syslog(LOG_ERR, "no PA4's detected!");
		syslog(LOG_ERR, "No sound level calibration will be performed."); 
	}

	if (verbose)
		syslog(LOG_WARNING, "device check OK");
	DEBUG(stderr, "tdtproc: device check OK\n");
}

/* ------------------------------------------------------------------------
   usage
   ------------------------------------------------------------------------ */
static void usage(void) 
{

	printf("tdtproc -- Tucker Davis data acquisition process\n");
	printf("------------------------------------------------\n");
	printf("Environment variables recognized by tdtproc:\n");
	printf("  DAQ_DEBUG     -- if set, tdtproc prints tons of debugging info to stderr\n");
	printf("  DAQ_NOIO      -- if set, pretend, but don't actually talk to daq hardware\n");
	printf("  DAQ_TIME      -- if set, print timestamps for certain events to stderr\n");
	printf("  DAQ_VERBOSE   -- if set, print more verbose syslog messages\n");
	printf("  DAQ_AD2_USE_SH  -- if set, set AD2 to use sample and hold\n"); 
    printf("  DAQ_AD2_CHANS -- set to \"<chan1>,<chan2>\" to use chan1 and chan2 for AD2 recording\n\n");
	printf("usage: tdtproc <switches>\n");
	printf("-------------------------\n");

	printf("  -breaklock            delete/break lock and exit\n");
	printf("  -devices              show all TDT devices detected by tdtproc, and exit\n");
	printf("  -version              print version info, and exit\n\n");

	printf("The following switches are only useful when tdtproc is\n"
			"called by a client program (xdphys, Exp, etc):\n");
	printf("------------------------------------------------------\n");
	printf("  -debug                set debug mode\n");
	printf("  -noio                 don't attach to the dsp\n");
	printf("  -unlock               override lock\n");
	printf("  -verbose              use more verbose syslog reporting\n\n");
	printf("  -adshmid <id>         shared memory id for AD segment\n");
	printf("  -adnchans <nchans>    # ad channels (must be either 2 or 4)\n");
	printf("  -in	<nsamples>      # samples in ONE ad channel\n");
	printf("  -dashmid <id>         shared memory id for DA segment\n");
	printf("  -out <nsamples>       # samples in ONE da channel\n");
	printf("  -et1shmid <id>        shared memory id for spike segment\n");
	printf("  -et1cnt <nspikes>     size of spike segment \n");
	printf("  -cntrlshmid <id>      shared memory id for control segment\n");
	printf("  -useatt [1|0]         use PA4's, if they exist\n");
	printf("  -fc <hz>              sampling freq\n");
	printf("  -sig <pid>            parent's proc id for signaling\n");
	printf("  -nice <priority>      set nice level to <priority> during conversion\n\n");
	printf("  -syncpulse            Generate a sync pulse on TG6 output #3 for Axoclamp and\n");
	printf("                        or oscilloscope, etc. Only used if -et1 is also used.\n");
	printf("                        (used by xdphys)\n");
 	printf("  -split                split play/rec cycle into separate load, play, rec \n");
	printf("                        ops (used by Exp)\n\n");
	printf("  -16	                no longer supported (ignored)\n");
	printf("  -32	                no longer supported (ignored)\n");
	printf("  -swap                 no longer supported (ignored)\n");
	printf("  -reboot               no longer supported (ignored)\n\n");
	clean_exit(0);
}

