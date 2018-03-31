/******************************************************************
**  RCSID: $Id: is-drpm.c,v 2.59 1998/11/25 20:02:50 cmalek Exp $
** Program: iserver
**  Module: is-drpm.c
**  Author: mazer
** Descrip: sparc/s56/drpm specific I.S. (Integral Server) code
**
** Revision History (most recent last)
**
** Sun Jun 21 18:30:53 1992 mazer
**  added is_ppDelay to compensate for the proport filters
**  all data files Revsion 2.1 and later are adjusted..
**
**  is_ppDelay (~80 ticks) corresponds to t=0 with respect to
**  the outgoing waveform. The first 80 ticks will be discarded
**  by is_getRaster and friends.
**
** Tue Feb  9 23:10:49 1993 mazer
**  is_getRaster is gone: it's now a separate module "spiker.c"
**  in order to cleanly support more intelligent spike sorting
**
** Sun Sep 26 11:32:04 1993 mazer
**  modified is_ppDelay usage to allow for specification of
**  two delays:
**	is_ppDelay1 -> number of tks to go through the FIR filter
**      is_ppDelay2 -> number of ms's to delay EVERYTHING. The
**                     delayed portions of the buffers will be
**                     invisible to the user, but sampling WILL
**                     take that much longer.. this is one way to
**                     kludge around the transient problem without
**                     changing all the other code...
**
** Wed Mar  9 15:45:49 1994 mazer
**  this is now set up to be included at the end of iserver.c
**
** Mon Jun  6 10:54:08 1994 mazer
**  moved is_postsample_hook stuff into iserver() and changed
**  the calling interface so is_sample() is in iserver.c and
**  it calls is_sample(), which is machine specific. This
**  was to make it possible to put the postsample_hook in once
**  source location.
**
**  Environment Variables:
**
**		DAQPROG:	set this to the location of drpmproc.  If this is
**                  not set, is_init() will look along the user's path.
**
**
*******************************************************************/

/******************************************************************\
|*                                                                *|
|*  is-drpm.c: Sparc + Ariel S-56x/ProPort Specific Code (libdrp) *|
|*                                                                *|
\******************************************************************/

#include <string.h>
#include <signal.h>
#include <errno.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <strings.h>
#include "port.h"

/* -----------------------------------------------------------------------
    Local Variables
   ----------------------------------------------------------------------- */
static int hzFc = -1;
static int is_ppDelay1 = 82;	/* 82 tick delay (1.6ms at 48khz): FIR lats */
static int is_ppDelay2 = 0;	/* ms enforced deley (to avoid clicks) */
static int is_ppDelay;		/* total delay in ticks */

static FILE *drpmproc_fp = NULL;
static int drpmproc_pid = -1;
static volatile int drpm_running = 0;
static volatile int drpm_ready = 0;
static volatile int drpm_timeout = 0;

static int is_outId = -1;
static int is_inId = -1;

/* -----------------------------------------------------------------------
    Private Prototypes
   ----------------------------------------------------------------------- */
static char *shm_request(int, int, int	*);
static int 	shm_release(char *, int);
static int 	valid_fc(float);
static void drpm_ready_handler(void);
static void drpm_done_handler(void);
static void drpm_timeout_handler(void);

/* -----------------------------------------------------------------------
   							PUBLIC FUNCTIONS  
   -----------------------------------------------------------------------
   isdrpm_set_ppDelays
   ----------------------------------------------------------------------- */
void isdrpm_set_ppDelays(
    int     ntks,
    int     nms)
{
    if (ntks >= 0) {
		is_ppDelay1 = ntks;
		fprintf(stderr, "Note: ppDelay1 is now %d ticks\n", is_ppDelay1);
    }
    if (nms >= 0) {
		is_ppDelay2 = nms;
		fprintf(stderr, "Note: ppDelay2 is now %d ms\n", is_ppDelay2);
    }
}

/* -----------------------------------------------------------------------
   is_init
   ----------------------------------------------------------------------- */
int is_init(
    float   hz_daFc,
    float   hz_adFc,
    float   hz_evtFc,
    int     ms_da,
    int     ms_ad)
{
    int     i;
	int		bufsize;
    float   f;
    char    drpmproc_cmd[1000];
	char	buf[100];
	char	*drpm_loc;
	char	*getenv();
    static char *drpm_cmdstr =
		"exec %s -nice %d -fc %d -out %d -in %d -sig %d -dashmid %d -adshmid %d\
		%s%s%s%s";
    static float last_hz_daFc;
    static float last_hz_adFc;
    static float last_hz_evtFc;
    static int last_ms_da;
    static int last_ms_ad;
	double timestamp;


    if (debugflag) {
		fprintf(stderr,
			"is_init(da=%f,ad=%f,evt=%f khz, da=%d,ad=%dms)\n",
			hz_daFc, hz_adFc, hz_evtFc, ms_da, ms_ad);
    }

	/* Determine the sampling frequency */
    if (hz_daFc == IS_LAST) {
		if (debugflag)
			fprintf(stderr, "is_init: trying to auto restart drpmproc\n");
		is_shutdown();

		hz_daFc = last_hz_daFc;
		hz_adFc = last_hz_adFc;
		hz_evtFc = last_hz_evtFc;
		ms_da = last_ms_da;
		ms_ad = last_ms_ad;

    } else if (!is_inited ||
	    hz_daFc != last_hz_daFc || hz_adFc != last_hz_adFc ||
	    hz_evtFc != last_hz_evtFc ||
	    ms_da != last_ms_da || ms_ad != last_ms_ad) {
		is_shutdown();
    } else {
		if (debugflag)
			fprintf(stderr, "is_init: reusing existing drpmproc\n");

#if(0)
		forbid_detach();
#endif 

		return (IS_OK);
    }

    if (hz_daFc == IS_BEST) hz_daFc = 48000.0;
    if (hz_adFc == IS_BEST) hz_adFc = 48000.0;
    if (hz_evtFc == IS_BEST) hz_evtFc = 48000.0;

    if (hz_adFc != IS_NONE && hz_evtFc != IS_NONE) {
		(*is_alert)("iserver: Can't use A/D and EVT channels together");
		return (IS_ERROR);
    } else if (hz_evtFc != IS_NONE)
		hz_adFc = hz_evtFc;

    f = -1.0;

    if (hz_daFc != IS_NONE) {
		f = hz_daFc;
    } else if (hz_adFc != IS_NONE) {
		f = hz_adFc;
    } else if (hz_evtFc != IS_NONE) {
		f = hz_evtFc;
    } else {
		(*is_alert)("iserver:\nAt least one hz_nnFc must != IS_NONE");
		return (IS_ERROR);
    }

    if ((hz_daFc != IS_NONE && (float) hz_daFc != f) ||
		(hz_adFc != IS_NONE && (float) hz_adFc != f) ||
		(hz_evtFc != IS_NONE && hz_evtFc != f)) {
		(*is_alert)("iserver: \nAll sampling frequencies must be equal");

		return (IS_ERROR);
    }

    if (valid_fc(f) < 0) {
		char    buf[500];

		sprintf(buf, "is_init:\n%g khz invalid, try:\n", f);
		strcat(buf, "48, 44.1, 32, 16, 11.025 or 8 khz");

		(*is_alert)(buf);
		return (IS_ERROR);
    }

    hzFc = is_rnd2int(f);

    if (hz_daFc != IS_NONE) is_daFc = hzFc;
    if (hz_adFc != IS_NONE) is_adFc = hzFc;
    if (hz_evtFc != IS_NONE) is_evtFc = hzFc;

	/* Determine how many samples are to be in each buffer */
    if (ms_da < 0) {
		printf("da: %d samples = ", -ms_da);
		ms_da = -ms_da / hz_daFc;
		printf("%d ms\n", ms_da);
    }

    if (ms_ad < 0) {
		printf("ad: %d samples = ", -ms_ad);
		ms_ad = -ms_ad / hz_adFc;
		printf("%d ms\n", ms_ad);
    }

    is_ppDelay = is_ppDelay1 + (is_ppDelay2 * hzFc / 1000);

    if (ms_da >= ms_ad) 
		bufsize = (ms_da * hzFc / 1000) + is_ppDelay;
    else
		bufsize = (ms_ad * hzFc / 1000) + is_ppDelay;

	/* Allocate the shared memory for the outpuBuffer */
    if (is_daFc == 0.0 && is_outputBuffer) {
		/* sampling frequency is zero for DA buf, so free the buffer */
		shm_release((char *)is_outputBuffer, is_outId);

		is_outputBuffer = NULL;
		is_outputSize = 0;
		is_outId = -1;
    } else {
		if (bufsize != is_outputSize) {
			/* requested buffer size is different than that used to create the
			   shared memory buffer, so resize the buffer */
			if (is_outputBuffer) shm_release((char *) (is_outputBuffer), 
				is_outId);

			is_outputSize = bufsize;
			is_outputBuffer = (xword *) shm_request(100, is_outputSize * 
				sizeof(xword) * da_nchans, &is_outId);

			if (is_outputBuffer == NULL) return (IS_ERROR);

			bzero((char *) is_outputBuffer, is_outputSize * sizeof(xword) * 
				  ad_nchans);
			is_outputSize -= is_ppDelay;

			if (debugflag)
				fprintf(stderr, 
					"is_init: att'd %d x %d + %d %ldbit frames output\n",
					da_nchans, is_outputSize, is_ppDelay, sizeof(xword) * 8);
		} else {
			/* the previously allocated buffer is the right size, so do nothing */
			if (debugflag)
				fprintf(stderr, 
					"is_init: reused %d x %d + %d %ldbit frames output\n",
					da_nchans, is_outputSize, is_ppDelay, sizeof(xword) * 8);
		}
    }

	/* Allocate the shared memory for the inputBuffer*/
    if (is_adFc == 0 && is_inputBuffer) {
		/* sampling frequency is zero for AD buf, so free the buffer */

		shm_release((char *) (is_inputBuffer-(is_ppDelay * ad_nchans)), 
			is_inId);

		is_inputBuffer = NULL;
		is_inputSize = 0;
		is_inId = -1;
    } else {
		if (bufsize != is_inputSize) {
			/* requested buffer size is different than that used to create the
			   shared memory buffer, so resize the buffer */

			if (is_inputBuffer)
				shm_release((char *)(is_inputBuffer - (is_ppDelay *ad_nchans)),
					is_inId);

			is_inputSize = bufsize;
			is_inputBuffer = (xword *) shm_request(105, is_inputSize * 
				sizeof(xword) * ad_nchans, &is_inId);

			if (is_inputBuffer == NULL) return (IS_ERROR);

			bzero((char *) is_inputBuffer, is_inputSize * sizeof(xword) * ad_nchans);
			is_inputBuffer += (is_ppDelay * ad_nchans);
			is_inputSize -= is_ppDelay;

			if (debugflag)
				fprintf(stderr, 
					"is_init: attached %d x %d + %d xword frames input\n",
					ad_nchans, is_inputSize, is_ppDelay);
		} else {
			/* the previously allocated buffer is the right size, so do nothing */
			if (debugflag)
				fprintf(stderr, 
					"is_init: reused %d x %d + %d xword frames input\n",
					ad_nchans, is_inputSize, is_ppDelay);
		}
    }

    if (!is_testmode) {
		int     trycnt;

		/* kick off the drpmproc program */
		if ((drpm_loc = getenv("DAQPROG")) == NULL) 
			drpm_loc = "drpmproc";
		
		if (debugflag)
			fprintf(stderr, "is-drpm: using \"%s\"\n", drpm_loc);

		trycnt = 0;
		try_unlocked:			/* come here to try removing the lock */

		sprintf(drpmproc_cmd, drpm_cmdstr,
			drpm_loc,		/* optional LOCATION/name of drpmproc */
			is_nice,		/* nice value for drpmproc */
			hzFc,			/* sampling frequency */
			bufsize,		/* output buffer size */
			bufsize,		/* input buffer size */
			getpid(),		/* my process id for signals */
			is_outId,		/* shm-id for output buffer */
			is_inId,		/* shm-id for input buffer */
		/* word size (nothing == use 32 bit words */
			sizeof(xword) == 2 ? " -16" : "-32",
			" -swap",		/* swap L & R output channels? */
			debugflag ? " -debug" : "",
			is_syncPulse ? " -syncpulse" : "");

		/* drpm_ready will go to 1, when drpmproc is ready */

		drpm_ready = 0;
		signal(SIGUSR2, drpm_ready_handler);

		/* read pid -- should return right away */
		if ((drpmproc_fp = popen(drpmproc_cmd, "r")) == NULL) {
			perror("is-drpm: is_init");
			(*is_alert)("iserver:\nCan't popen drpmproc");
			return (IS_ERROR);
		}

		*buf = 0;
		fgets(buf, sizeof(buf), drpmproc_fp);
		if (!*buf) {
			fclose(drpmproc_fp);
			drpmproc_fp = NULL;
			(*is_alert)("iserver:\nCan't read pid from drpmproc");
			return (IS_ERROR);
		}
		sscanf(buf, "%d", &drpmproc_pid);

		if (drpmproc_pid < 0) {

			if (trycnt > 0) {
				(*is_alert)("iserver: can't unlock drpmproc\n");
				return (IS_ERROR);
			}

			fscanf(drpmproc_fp, "%d", &i);
			fclose(drpmproc_fp);
			if (is_yesno) {
				sprintf(buf, 
					"iserver: drpmproc locked by proc #%d\nOverride anyway?", i);

				if ((*is_yesno)(buf, "Yes", "No", NULL) == 2)
					return (IS_ERROR);
			} else {
				fprintf(stderr, "iserver: drpmproc locked by proc #%d\n", i);
				return (IS_ERROR);
			}

			sprintf(drpmproc_cmd, "%s -breaklock", drpm_loc);
			system(drpmproc_cmd);

			trycnt++;

			goto try_unlocked;
		} else if (drpmproc_pid == 0) {

			fclose(drpmproc_fp);
			(*is_alert)("iserver:\nCan't remove drpmproc lock");

			return (IS_ERROR);
		} else {
			if (debugflag)
				fprintf(stderr, "%d = %s\n", drpmproc_pid, drpmproc_cmd);


			is_inited = 1;
			last_hz_daFc = hz_daFc;
			last_hz_adFc = hz_adFc;
			last_hz_evtFc = hz_evtFc;
			last_ms_da = ms_da;
			last_ms_ad = ms_ad;
			bzero((char *) is_inputBuffer, is_inputSize * sizeof(xword) *
				ad_nchans);

			is_sample(X_DA | X_AD, ms_da, &timestamp);

			bzero((char *) is_outputBuffer,
				(is_outputSize * sizeof(xword) * ad_nchans));

			return (IS_OK);
		}
    } else {

		is_inited = 1;
		last_hz_daFc = hz_daFc;
		last_hz_adFc = hz_adFc;
		last_hz_evtFc = hz_evtFc;
		last_ms_da = ms_da;
		last_ms_ad = ms_ad;

		/* preTrip -- ssirp bug */
		is_sample(X_DA | X_AD, ms_da, &timestamp);
		bzero((char *) is_inputBuffer, is_inputSize * sizeof(xword) * ad_nchans);
		bzero((char *) is_outputBuffer, (is_outputSize * 
			sizeof(xword) * ad_nchans));
		/* preTrip -- ssirp bug */

		return (IS_OK);
    }
}



/* -----------------------------------------------------------------------
   is_sample
   ----------------------------------------------------------------------- */

#define suspend()		{sigemptyset(&mask); \
				if (sigsuspend(&mask) == -1) \
					if (errno != EINTR) \
											perror("iserver: ");}

int  is_sample(
    int     flags,
	int		ms_dur,
	double  *timestamp)
{
    int     result;
    int     da;
	int		ad;
	int		evt;
    int     retry_count;
    char    buf[255];
	char	*p;
	sigset_t	mask;
	struct timeval	time;
	struct timezone	zone = { 0, DST_USA };

    extern void beep();


    if (debugflag) {
		fprintf(stderr, "is_sample(%s%s%s%s%s %d ms)\n",
			(flags & X_AD) ? "X_AD," : "", (flags & X_DA) ? "X_DA," : "",
			(flags & X_EVT) ? "X_EVT," : "", (flags & X_TRIG) ? "X_TRIG," : "",
			!flags ? "X_NADA," : "", ms_dur);
    }

    if (!is_testmode) {
	
		if (drpmproc_pid < 0 || !is_inited) {
			(*is_alert)("iserver: drpmproc never started");
			is_shutdown();
			return (IS_ERROR);
		}
		else if (kill(drpmproc_pid, 0) != 0) {
			if (is_yesno) {
				if ((*is_yesno)("iserver: drpmproc dead, restart?", "Ok", "Fail", 
					NULL) == 1) {
					is_init(IS_LAST, IS_LAST, IS_LAST, 0, 0);
					return (is_sample(flags, ms_dur, timestamp));
				} else {
					is_shutdown();
					return (IS_ERROR);
				}
			} else {
				is_init(IS_LAST, IS_LAST, IS_LAST, 0, 0);
			}
		}

		evt = flags & X_EVT;
		ad = flags & X_AD;
		da = flags & X_DA;

		if ((!evt && !ad && !da) || (evt && ad)) {
			sprintf(buf, "iserver:\ninvalid request ad=%d da=%d evt=%d",
				ad, da, evt);
			(*is_alert)(buf);
			return (IS_ERROR);
		}
		retry_count = 0;

	retry:				/* retry loop.. goto's are bad! */

		drpm_timeout = 0;
		drpm_running = 1;

		signal(SIGUSR1, drpm_done_handler);
		signal(SIGALRM, drpm_timeout_handler);

		if (!debugflag) alarm(5 * (ms_dur / 1000 + 1));

		while (!drpm_ready && !drpm_timeout) 
			suspend();

		if (!drpm_timeout) {
			if (debugflag)
				fprintf(stderr, "iserver: drpmproc is ready, sending go sig\n");
			if (gettimeofday(&time, &zone) == -1)
				perror("iserver: gettimeofday");
			*timestamp = (double) time.tv_sec + 
				((double) time.tv_usec)/1000000.0;

			kill(drpmproc_pid, SIGUSR1);

			while (drpm_running && !drpm_timeout)
				suspend();

			if (!debugflag) alarm(0);

			if (!drpm_timeout) {
				fgets(buf, sizeof(buf), drpmproc_fp);
				if ((p = rindex(buf, '\n')) != NULL)
					*p = 0;
				sscanf(buf, "%d", &result);

				if (result == IS_TRANS_OK) {
					result = IS_OK;
				} else if (result == IS_TRANS_MISSED) {
					if (retry_count++ < 10) {
						if (is_report_errors)
							(*is_notify)("iserver: %s, trying again", 
								index(buf, ':') + 1);
						goto retry;
					} else {
						if (is_report_errors)
							(*is_alert)("iserver:\n%s\ngiving up", 
								index(buf, ':') + 1);

						is_shutdown();

						result = IS_ERROR;
					}
				} else {
					if (is_report_errors)
						(*is_alert)(index(buf, ':') + 1);

					is_shutdown();

					result = IS_ERROR;
				}
			}
		}

		signal(SIGALRM, SIG_DFL);
		signal(SIGUSR1, SIG_DFL);

		if (drpm_timeout) {
			if (is_report_errors)
				(*is_alert)("iserver:\ndrpmproc timed out");

			return(IS_ERROR);
		}

    } else {
		result = IS_OK;
    }

    return (result);
}

#undef suspend

/* -----------------------------------------------------------------------
   is_shutdown
   ----------------------------------------------------------------------- */
int is_shutdown(void)
{
    if (debugflag)
		fprintf(stderr, "is_shutdown()\n");

    if (!is_testmode) {
		if (drpmproc_pid > 0) {
			if (kill(drpmproc_pid, 0) == 0)
				kill(drpmproc_pid, SIGUSR2);

			drpmproc_pid = -1;
		}
		if (drpmproc_fp != NULL) {
			if (debugflag)
				fprintf(stderr, "is_shutdown: pclosing\n");

			pclose(drpmproc_fp);

			drpmproc_fp = NULL;
		}
    }

    if (is_inited) {
		if (is_outputBuffer)
			shm_release((char *) is_outputBuffer, is_outId);

		is_outputBuffer = NULL;
		is_outputSize = 0;
		is_outId = -1;

		if (is_inputBuffer)
			shm_release((char *) (is_inputBuffer - (is_ppDelay * ad_nchans)), 
				is_inId);

		is_inputBuffer = NULL;
		is_inputSize = 0;
		is_inId = -1;
		is_inited = 0;

    }

    return (IS_OK);
}


/* -----------------------------------------------------------------------
   								PRIVATE FUNCTIONS
   -----------------------------------------------------------------------
   shm_request
   ----------------------------------------------------------------------- */
static char *shm_request(
    int     key,
    int     nbytes, 
	int		*id_ptr)
{
    int     i;
    char   *block;
    extern int shmget();
    extern char *shmat();

    if (!is_testmode) {
		while ((*id_ptr = shmget((key_t) key, nbytes, IPC_CREAT | 00666)) < 0) {

			/* shmget failed: tell us about it */
			system("/bin/ipcs -m");
			fprintf(stderr, "shm_request: shmget(key=%d, nbytes=%d, perm=O%o)\n",
				key, nbytes, IPC_CREAT | 00666);
			perror("shmget");

			/* now, what should we do?? */
			if (is_yesno) {
				i = (*is_yesno)("shm_request failed, check shm status",
					"Retry", "Fail", "Exit");

				switch (i) {
					case 2:
						return (NULL);
					case 3:
						fprintf(stderr, 
							"iserver: Aborting due to failed shmget()\n");
						exit(1);
				}
			} else {
				fprintf(stderr, "iserver: Aborting due to failed shmget()\n");
				exit(1);
			}
		}
		block = shmat(*id_ptr, (char *) 0, 0);
		if ((int) block == -1) {
			perror("shmat");
			return (NULL);
		}
		bzero(block, nbytes);
		return (block);
    } else {
		return ((char *) calloc(nbytes, sizeof(char)));
    }
}

/* -----------------------------------------------------------------------
   shm_release
   ----------------------------------------------------------------------- */
static int shm_release(
    char   *block,
    int     id)
{
    extern int shmdt(), shmctl();

    if (!is_testmode) {
		if (shmdt(block) == -1) {
			perror("shmdt");
			return (0);
		} else if (shmctl(id, IPC_RMID, NULL) == -1) {
			/* this usually means someone overrode the lock while you weren't
			 * looking.. */
	/*      perror("shmctl(IPC_RMID)"); */
			return (0);
		}
		return (1);
    } else {
		free(block);
		return (1);
    }
}

/* -----------------------------------------------------------------------
   valid_fc
   ----------------------------------------------------------------------- */
static int valid_fc(
    float   khz)
{
    int     i;
    static float fclist[] = {48000.0, 44100.0, 32000, 16000, 11025.0, 8000};	/* khz */

    if (is_testmode) return (1);

    for (i = 0; i < (sizeof(fclist) / sizeof(float)); i++) {
		if (khz == fclist[i]) {
			if (debugflag)
			fprintf(stderr, "valid_fc: %f IS a valid sampling frequency\n", khz);
			return (i + 1);
		}
    }

    if (debugflag)
		fprintf(stderr, "valid_fc: %f is NOT a valid sampling frequency\n", khz);

    return (-1);
}

/* -----------------------------------------------------------------------
   drpm_done_handler
   ----------------------------------------------------------------------- */
static void drpm_done_handler(void)
{
    drpm_running = 0;
}

/* -----------------------------------------------------------------------
   drpm_ready_handler
   ----------------------------------------------------------------------- */
static void drpm_ready_handler(void)
{
    drpm_ready = 1;
    signal(SIGUSR2, drpm_ready_handler);
}

/* -----------------------------------------------------------------------
   drpm_timeout_handler
   ----------------------------------------------------------------------- */
static void drpm_timeout_handler(void)
{
    drpm_timeout = 1;
}

