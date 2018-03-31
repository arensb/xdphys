/*
**	This file is part of XDowl
**	Copyright (c) 1997 Chris Malek
**	California Institute of Technology
**	<mazer@asterix.cns.caltech.edu>
*/

static char *rcsid2 = "@(#) $Id: is-tdt.c,v 1.4 1997/03/10 23:42:12 cmalek Exp $";

/******************************************************************
**  RCSID: $Id: is-tdt.c,v 1.4 1997/03/10 23:42:12 cmalek Exp $
** Program: dowl
**  Module: is-tdt.c
**  Author: cmalek
** Descrip: linux/tdt specific I.S. (Integral Server) code
**
**
**
** ENVIRONMENT VARIABLES:
**
**  TDTPROC:	define this to be the location of the tdtproc 
**				executable, if you want.
**
*******************************************************************/

/******************************************************************\
|*  is-drpm.c: Linux + Tucker Davis Specific Code (libtdt)        *|
\******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>

#include "dowlio.h"
#include "svars.h"
#include "spiker.h"
/* comment out the next line to use is_tdt.c with init_test */
/* #include "xloop.h" */
#include "pop.h"
#include "iserver.h"
#include "iserverInt.h"
#include "tdtproc.h"

/* ------------------------------------------------
   Macros
   ------------------------------------------------ */
#define DEBUG	if (debugflag) fprintf
#define SHMEM_PERMS		00666

#define ET1_NSPIKES		5000
#define IS_BEST_FC		48.0
#define IS_PLAY_KEY		12345
#define IS_REC_KEY		22345
#define IS_SPIKE_KEY	42345

#define FREQ_USE_NEW	0
#define FREQ_USE_EXISTING	1
#define FREQ_ERROR	-1

#define send_die_signal() kill(tdtproc_pid, SIGUSR2)
#define send_go_signal() kill(tdtproc_pid, SIGUSR1)
#define tdtproc_running() ((kill(tdtproc_pid, 0) == 0) ? 1 : 0)
#define send_done_signal() kill(tdtproc_pid, SIGUSR1)

/* ------------------------------------------------
   Typedefs
   ------------------------------------------------ */
/* freq_struct is used by is_init */
typedef struct freq_struct {
	float		play;
	float		rec;
	int		play_dur;
	int		rec_dur;
	int		useET1;
	void	(*idlefn)();
} FREQ;

/* ------------------------------------------------
   Public Variables
   ------------------------------------------------ */
unsigned long *is_spikeBuffer = NULL;
int 	is_spikeSize = 0;
int 	is_useET1 = 1;

/* ------------------------------------------------
   Private Variables
   ------------------------------------------------ */
static int is_first_init = 1;
static int is_outId = -1;
static int is_inId = -1;
static int is_spikeId = -1;
static int is_nice = -10;
static FREQ last;

static FILE *tdtproc_fp = NULL;
static int tdtproc_pid = -1;

volatile sig_atomic_t tdt_ready;
volatile sig_atomic_t tdt_running;
volatile sig_atomic_t tdt_timeout;

extern int rnd2int(float);
/* ------------------------------------------------
   Prototypes
   ------------------------------------------------ */
/* is_init related functions */
static void initialize_freq(FREQ *);
static int determine_samp_freqs(FREQ *);
static int validate_freqs(FREQ *);
static int reuse_old_freqs(FREQ *);
static int check_last_freqs(FREQ *);
static int allocate_shared_mem(FREQ *);
static int buffer_size(FREQ *);
static void  allocate_one_buffer(int, void **, int *, int *, int, int,
	int);
static int start_tdtproc(FREQ *);
static void clear_buffers(void);
static void setup_init_sighandlers(void);
static void clear_init_sighandlers(void);
static void save_freqs(FREQ	*);
static int call_tdtproc(void);
static int get_tdtproc_retval(void);
static char *format_cmdstring(char *);
static int get_tdtproc_location(char **);
static int file_is_executable(char *);
static void tdt_done_handler(int);
static void sigchld_handler(int);
static void tdt_timeout_handler(int);
static void tdt_ready_handler(int);
static int shm_release(void *, int);
static char *shm_request(int, int, int *);

/* is_sample_aux related functions */
static int tdtproc_started(void);
static int check_flags(int);
static int setup_sample_sighandlers(int);
static int trigger_tdtproc(void);
static void clear_sample_sighandlers(void);

/* --------------------------------------------------------------------
   is_getSpikebuffer
   -------------------------------------------------------------------- */
unsigned long *is_getSpikebuffer(int *nsamps)
{
  if (is_spikeBuffer != NULL) {
	  if (nsamps != NULL)
		*nsamps = is_spikeBuffer[0];
	  return(is_spikeBuffer+1);
  } else {
	  if (nsamps != NULL)
		  *nsamps = 0;
	  return(NULL);
  }
}

/* --------------------------------------------------------------------
   is_clearInput
   -------------------------------------------------------------------- */
int is_clearSpikes(void)
{
	bzero((char *) is_spikeBuffer, is_spikeSize * sizeof(unsigned long));
}

/* ----------------------------------------------------------------------
   is_init

	   The spike_mode returned by getSpikeRaster_getmode() must be
	   "ttl" to enable gathering spikes via the SD1/ET1.

	   Ok, to make everything work correctly, in xdowl, while to gather
	   spike times, we have to set evt_fc to 1MHz.  This way, the A/D tick 
	   interval will be 1ms, and all the conversions between A/D ticks and 
	   microseconds will work.

	   We can't sample the analog waveforms at 1Mz with the DD1, of
	   course, so when the mod_*.c files call is_init() with evt_fc=1MHz
	   we will use the play_fc as the freq to use for the ad buffers.

	   ignore ign_maxSpikes, since is-drpm.c ignores it.
   ---------------------------------------------------------------------- */
int is_init(
    float   play_fc,
    float   rec_fc,
    float   evt_fc,							   
    int     play_dur_ms,                               /* milliseconds */
    int     rec_dur_ms,                                /* milliseconds */
    int     ign_maxSpikes,
    IDLEFN  idlefn)
{
	FREQ	current;
	int		bufsize;
	int		status = IS_OK;
	char	*spike_mode;

	if (is_first_init) {
		initialize_freq(&last);
		is_first_init = 0;
	}

	is_adFc = -1;
	is_daFc = -1;

	/* Always use SD1/ET1 if evt_fc > 0.0 */
	if (evt_fc > 0.0) {
		current.rec = play_fc;
		is_evtFc = evt_fc;
		is_useET1 = 1;
	} else {
		current.rec = rec_fc;
		is_evtFc = -1;
		is_useET1 = 0;
	}
	current.play = play_fc;
	current.play_dur = play_dur_ms;
	current.rec_dur = rec_dur_ms;
	current.idlefn = idlefn;

	switch(determine_samp_freqs(&current)) {
		case FREQ_USE_NEW:
			is_shutdown();
			allocate_shared_mem(&current);
			status = start_tdtproc(&current);
			break;
		case FREQ_USE_EXISTING:
			status = IS_OK;
			break;
		case FREQ_ERROR:
			is_shutdown();
			status = IS_ERROR;
			break;
	}

	return(status);
}

/* ----------------------------------------------------------------------
   initialize_freq: is_init
   ---------------------------------------------------------------------- */
static void initialize_freq(
	FREQ	*f)
{
	f->play = IS_NONE;
	f->rec = IS_NONE;
	f->play_dur = -1;
	f->rec_dur = -1;
	f->idlefn = NULL;
}

/* ----------------------------------------------------------------------
   determine_samp_freq: is_init
   ---------------------------------------------------------------------- */
static int determine_samp_freqs(
	FREQ  *current)
{
	int	status;

	status = reuse_old_freqs(current);
	switch (status) {
		case FREQ_USE_NEW:
			if (current->rec == IS_BEST) current->rec = IS_BEST_FC;
			if (current->play == IS_BEST) current->play = IS_BEST_FC;

			if (validate_freqs(current) == IS_OK) {
				/* A/D and D/A freqs have to be the same */
				is_adFc = is_daFc = rnd2int(current->play*1000.0);
			} else {
				status = FREQ_ERROR;
			}
			break;
		case FREQ_USE_EXISTING:
			break;
		case FREQ_ERROR:
			break;
	}

	return(status);
}

/* ----------------------------------------------------------------------
   reuse_old_freqs: is_init

        return:

			-1 if no last frequencies exist, and there are no new freqs
			0 if last frequencies exist, but some new args are different
			1 if last frequencies exist, and they're ok
   ---------------------------------------------------------------------- */
#define DIFFERENT_ARGS (!is_inited || current->play != last.play || \
	current->rec != last.rec || current->play_dur != last.play_dur || \
	current->rec_dur != last.rec_dur || is_useET1 != last.useET1 || \
	current->idlefn != last.idlefn)

static int reuse_old_freqs(
	FREQ *current)
{
	int status = FREQ_USE_NEW;

	if (current->play == IS_LAST) {
		if (is_inited && tdtproc_running()) {
			DEBUG(stderr, "iserver: reusing existing tdtproc\n");
			status = FREQ_USE_EXISTING;
		} else {
			if (check_last_freqs(current) != IS_OK) {
				alert("iserver:\n No last freqs, must specify "
					"AD/DA samp freqs"); 
				status = FREQ_ERROR;
			} 
		}
		
	} else {
		if (!DIFFERENT_ARGS) {
			DEBUG(stderr, "iserver: reusing existing tdtproc\n");

			forbid_detach();
			status = FREQ_USE_EXISTING;
		} 
	}

	return(status);
}

/* ----------------------------------------------------------------------
   check_last_freqs: is_init
   ---------------------------------------------------------------------- */
static int check_last_freqs(
	FREQ *current)
{
	int		status = IS_OK;

	if (last.play > 0.0) {
		current->play = last.play;
		current->rec = last.rec;
		current->play_dur = last.play_dur;
		current->rec_dur = last.rec_dur;
		current->useET1 = last.useET1;
		current->idlefn = last.idlefn;
	} else 
		status = IS_ERROR;

	return(status);
}

/* ----------------------------------------------------------------------
   validate_freqs: is_init
   ---------------------------------------------------------------------- */
static int validate_freqs(
	FREQ *current)
{
	if ((current->play == IS_NONE) && (current->rec == IS_NONE)) {
		alert("iserver:\n At least one of the AD/DA freqs must be"
			" != IS_NONE");
		return(IS_ERROR);
	}

	if (current->play != current->rec) {
		alert("iserver:\nAll sampling frequencies must be equal");
		return (IS_ERROR);
	}

	return(IS_OK);
}

/* ----------------------------------------------------------------------
   allocate_shared_mem: is_init
   ---------------------------------------------------------------------- */
static int allocate_shared_mem(
	FREQ *current)
{
	int bufsize = buffer_size(current);

	allocate_one_buffer(IS_PLAY_KEY, &is_outputBuffer, &is_outputSize, 
		&is_outId, da_nchans, bufsize, sizeof(xword));
	allocate_one_buffer(IS_REC_KEY, &is_inputBuffer, &is_inputSize, &is_inId, 
		ad_nchans, bufsize, sizeof(xword));
	/* ET1 spike buffer */
	if (is_useET1)
		allocate_one_buffer(IS_SPIKE_KEY, (void **) &is_spikeBuffer, 
			&is_spikeSize, &is_spikeId, 1, ET1_NSPIKES, sizeof(unsigned long));
	else {
		if ((is_spikeBuffer != NULL) && (is_spikeId != -1)) {
			shm_release(is_spikeBuffer, is_spikeId);
			is_spikeBuffer = NULL;
			is_spikeSize = 0;
			is_spikeId = -1;
		}
	}
} 

/* ----------------------------------------------------------------------
   buffer_size: is_init
   		
   		current->play and current->rec are in kHz
		current->play_dur and current->rec_dur are in milliseconds

		if current->play_dur or current->rec_dur are < 0,
		they have been given in # samples.
   ---------------------------------------------------------------------- */
static int buffer_size(
	FREQ *current)
{
	int	bufsize;


    if (current->play_dur < 0) {
		printf("iserver: da size: %d samples = ", -(current->play_dur));
		current->play_dur= -(current->play_dur) / current->play;
		printf("iserver: %d ms\n", current->play_dur);
    }

    if (current->rec_dur < 0) {
		printf("iserver: ad size: %d samples = ", -(current->rec_dur));
		current->rec_dur= -(current->rec_dur) / current->rec;
		printf("iserver: %d ms\n", current->rec_dur);
    }

	if (current->play_dur > current->rec_dur)
		bufsize = (current->play_dur * current->play);
	else
		bufsize = (current->rec_dur * current->rec);


	return(bufsize);
}

/* ----------------------------------------------------------------------
   allocate_one_buffer: is_init
   		
   		The buffer will be zeroed by clear_buffers() in start_tdtproc()
   ---------------------------------------------------------------------- */
static void  allocate_one_buffer(
	int	key,
	void **buffer,
	int *bufsize,
	int *id,
	int nchans,
	int new_bufsize,
	int samp_size)
{
	if (new_bufsize != *bufsize) {
		if (*buffer != NULL) 
			shm_release(*buffer, *id);

		*bufsize = new_bufsize;
		*buffer = (void *) shm_request(key, (*bufsize)*samp_size*nchans, 
			id);
		assert(*buffer != NULL);

		DEBUG(stderr, "iserver: reallocated shm_buffer\n");
	} else
		DEBUG(stderr, "iserver: reusing old shm_buffer\n");
}

/* ----------------------------------------------------------------------
   start_tdtproc: is_init
   ---------------------------------------------------------------------- */
static int start_tdtproc(
	FREQ *current)
{
	int	status = IS_OK;

    if (!is_testmode) {

		setup_init_sighandlers();
		if (call_tdtproc() == IS_OK) {
			DEBUG(stderr, "iserver: tdtproc launched succssfully\n");
			DEBUG(stderr, "iserver: tdtproc pid = %d \n", tdtproc_pid);
		} else {
			DEBUG(stderr, "iserver: failed to launch tdtproc\n");

			clear_init_sighandlers();
			status = IS_ERROR;
		} 
	} 

	if (status == IS_OK) {
		is_inited = 1;

		idle_set(current->idlefn);
		save_freqs(current);
		is_sample_aux(X_DA | X_AD, current->play_dur); 
		clear_buffers();
		forbid_detach();
	}

	return(status);
}

/* ------------------------------------------------------------------------
   clear_buffers: is_init
   ------------------------------------------------------------------------ */
static void clear_buffers(void)
{
	bzero((char *) is_inputBuffer, is_inputSize * sizeof(xword) * ad_nchans);
	bzero((char *) is_outputBuffer, is_outputSize * sizeof(xword) * da_nchans);
	if (is_useET1)
		bzero((char *) is_spikeBuffer, is_spikeSize * sizeof(unsigned long));
}			

/* ------------------------------------------------------------------------
   setup_init_sighandlers: is_init
   ------------------------------------------------------------------------ */
static void setup_init_sighandlers(void)
{
	tdt_ready = 0;

	signal(SIGUSR2, tdt_ready_handler);
	signal(SIGCHLD, sigchld_handler);
}

/* ------------------------------------------------------------------------
   clear_init_sighandlers: is_init
   ------------------------------------------------------------------------ */
static void clear_init_sighandlers(void)
{
	signal(SIGUSR2, SIG_DFL);
	signal(SIGCHLD, SIG_DFL);
}

/* ----------------------------------------------------------------------
   save_freqs: is_init
   ---------------------------------------------------------------------- */
static void save_freqs(
	FREQ	*current)
{
	last.play = current->play;
	last.rec = current->rec;
	last.play_dur = current->play_dur;
	last.rec_dur = current->rec_dur;
	last.useET1 = is_useET1;
	last.idlefn = current->idlefn;
}

/* ------------------------------------------------------------------------
   call_tdtproc: is_init

   		Try twice to start tdtproc.
   ------------------------------------------------------------------------ */
static int call_tdtproc(void)
{
	int status = IS_ERROR;
	int trycnt = 0;
	char *tdt_cmdstr = NULL;

	while (trycnt < 2) {
		if (trycnt == 0) 
			tdt_cmdstr = format_cmdstring("\0");
		else
			tdt_cmdstr = format_cmdstring("-breaklock");

		if (tdt_cmdstr != NULL) {
			if ((tdtproc_fp = popen(tdt_cmdstr, "r")) == NULL) {
				perror("iserver: popen");
				return(IS_ERROR);
			}
			free(tdt_cmdstr); 

			if (get_tdtproc_retval() != IS_ERROR) {
				status = IS_OK;
				break;
			}

			trycnt++;
		} 
	}

	return(status);
}

/* ------------------------------------------------------------------------
   get_tdtproc_retval: is_init
   ------------------------------------------------------------------------ */
static int get_tdtproc_retval(void)
{
	char buf[80];
	int tmp;

	fscanf(tdtproc_fp, "%d", &tdtproc_pid);
	if (tdtproc_pid == -1) {
		fprintf(stderr, "iserver: couldn't read pid from tdtproc\n");
		return(IS_ERROR);
	}

	if (tdtproc_pid < 0) {

		/* tdtproc returns -(pid) if a lock exists */
		fscanf(tdtproc_fp, "%d", &tmp);
		pclose(tdtproc_fp);

		sprintf(buf, "iserver: tdtproc locked by proc #%d\nOverride anyway?", tmp);
		if (pop_box(buf, "Yes", "No", NULL) == 2)
			return(IS_ERROR);
		else
			return(IS_OK);
	} else {
		if (tdtproc_pid == 0) {
			pclose(tdtproc_fp);
			alert("iserver:\nCan't remove tdtproc lock");

			return (IS_ERROR);
		} 
	}
	return(IS_OK);
}

/* ------------------------------------------------------------------------
   format_cmdstring: is_init

   		Remember to free the string returned by this function.

		extra is either some extra flag to pass to tdtproc, or
		the empty string: ""
   ------------------------------------------------------------------------ */
static char *format_cmdstring(
	char	*extra)
{
	char *tdt_cmdstr = NULL;
	char *cmd = NULL;
	char tmp[256];
	char *tdt_cmd = NULL;
	int	tmp_nice;

	assert(extra != NULL);

	if (get_tdtproc_location(&cmd) == IS_OK) {

		tmp_nice = GI("drpm-nice");
		if (!LastVarUndefined)
			is_nice = tmp_nice;

		(void) memset(tmp, 0, 256);
		if (is_useET1) {
			tdt_cmdstr = "exec %s -nice %d -fc %d -out %d -in %d"
				" -sig %d -dashmid %d -adshmid %d -et1shmid %d -et1cnt %d %s";

			sprintf(tmp, tdt_cmdstr, cmd, is_nice, is_daFc, is_outputSize, 
				is_inputSize, getpid(), is_outId, is_inId, is_spikeId,
				is_spikeSize, extra);
		} else {
			tdt_cmdstr = "exec %s -nice %d -fc %d -out %d -in %d"
				" -sig %d -dashmid %d -adshmid %d %s";

			sprintf(tmp, tdt_cmdstr, cmd, is_nice, is_daFc, is_outputSize, 
				is_inputSize, getpid(), is_outId, is_inId, extra);
		}
		free(cmd);

		tdt_cmd = (char *) calloc(256, sizeof(char));
		assert(tdt_cmd != NULL);

		strcpy(tdt_cmd, tmp);

		DEBUG(stderr, "iserver: using following string for tdtproc:\n"
			"iserver: %s\n", tdt_cmd);

		return(tdt_cmd);
	} else {
		return(NULL);
	}
}

/* ----------------------------------------------------------------------
   get_tdtproc_location: is_init

   		memory allocated for tdt_loc must be free'd by calling
		function
   ---------------------------------------------------------------------- */
static int get_tdtproc_location(
	char **tdt_loc)
{	
	char *temp = NULL;
	char *locptr = NULL;

	assert(*tdt_loc == NULL);

	if ((temp = getenv("TDTPROC")) != NULL) {

		locptr = strtok(temp, "=");
		locptr++;

		if (locptr != NULL) {
			if (!file_is_executable(locptr)) {
				free(temp);
			} else {
				*tdt_loc = (char *) calloc(strlen(locptr), sizeof(char));
				assert(*tdt_loc != NULL);
				(void) strcpy(*tdt_loc, locptr);
			}

		}
	} else {
		*tdt_loc = (char *) calloc(strlen("tdtproc")+1, sizeof(char));
		assert(*tdt_loc != NULL);

		strcpy(*tdt_loc, "tdtproc");
	}
		

	if (!file_is_executable(*tdt_loc)) 
		return(IS_ERROR);
	else
		return(IS_OK);
}


/* ----------------------------------------------------------------------
   file_is_executable: is_init
   ---------------------------------------------------------------------- */
static int file_is_executable(
	char *path)
{
	int		status = 1;
	struct stat m;

	if (stat(path, &m) == -1) {
		perror("iserver: stat tdtproc");
		status = 0;
	} else {
		if (S_ISREG(m.st_mode)) {
			if (access(path,  R_OK | X_OK) == -1) {
				perror("iserver: access TDTPROC");
				status = 0;
			} 
		} else {
			fprintf(stderr, "iserver: %s is not a regular file.", path);
			status = 0;
		}
	}

	return(status);
}

/* ----------------------------------------------------------------------
   is_sample_aux
   		
       don't know what "flags" is for, since it's never used, and
	   ms_dur is only used to compute the timeout interval.
   ---------------------------------------------------------------------- */
#define MAX_RETRIES 10

int is_sample_aux(
    int     flags,
	int		ms_dur)
{
	int status = IS_OK;
	int	retry_count = 0;

	if (!is_testmode) {
		if ((tdtproc_started() == IS_OK) && (check_flags(flags) == IS_OK)) {
			do {
				if (setup_sample_sighandlers(ms_dur) != IS_OK)
					break;
				status = trigger_tdtproc();
				retry_count++;
			} while ((status != IS_OK) && (retry_count < MAX_RETRIES)); 

			if (retry_count == MAX_RETRIES) {
				if (is_report_errors)
					alert("iserver: giving up");
				is_shutdown();
				status = IS_ERROR;
			}

			if (tdt_timeout) {
				if (is_report_errors)
					alert("iserver:\ntdtproc timed out");
				status = IS_ERROR;
			}

			clear_sample_sighandlers();
		}
	}

	view_output();
	view_input();

	return(single_step(status));
}
#undef MAX_RETRIES

/* ----------------------------------------------------------------------
   tdtproc_started: is_sample_aux
   ---------------------------------------------------------------------- */
static int tdtproc_started(void)
{
	int	status = IS_OK;

	if (tdtproc_pid < 0 || !is_inited) {
		alert("iserver:\ntdtproc never started");
		is_shutdown();
		status = IS_ERROR;
	}
	else {
		if (!tdtproc_running()) {
			if (pop_box("iserver: tdtproc dead, restart?", "Ok", "Fail", 
				NULL) == 1) {
				is_init(IS_LAST, IS_LAST, IS_LAST, 0, 0, 0, NULL);
			} else {
				is_shutdown();
				status = IS_ERROR;
			}
		}
	}

	return(status);
}

/* ----------------------------------------------------------------------
   check_flags: is_sample_aux

   		Why is this necessary?
   ---------------------------------------------------------------------- */
static int check_flags(
	int flags)
{
	int status = IS_OK;
	int	ad,da,evt;
	char buf[80];

	evt = flags & X_EVT;
	ad = flags & X_AD;
	da = flags & X_DA;

	if ((!evt && !ad && !da) || (evt && ad)) {
		sprintf(buf, "iserver:\ninvalid request ad=%d da=%d evt=%d", ad, 
			da, evt);
		alert(buf);
		status = IS_ERROR;
	}

	return(status);
}

/* ------------------------------------------------------------------------
   setup_sample_sighandlers: is_sample_aux
   ------------------------------------------------------------------------ */
static int setup_sample_sighandlers(
	int ms_dur)
{
	int status = IS_OK;

	tdt_timeout = 0;
	tdt_running = 1;

	signal(SIGUSR1, tdt_done_handler);
	signal(SIGALRM, tdt_timeout_handler);

	if (!debugflag) alarm(5 * (ms_dur / 1000 + 1));

	while (!tdt_ready && !tdt_timeout)
		dloop_empty();

	if (tdt_timeout)
		status = IS_ERROR;

	return(status);
}

/* ------------------------------------------------------------------------
   trigger_tdtproc: is_sample_aux
   ------------------------------------------------------------------------ */
static int trigger_tdtproc(void)
{
	int status = IS_OK;
	char buf[255];
	char *p;
	int	result;

	DEBUG(stderr, "iserver: tdtproc ready: sending go signal\n");
	fflush(stderr);
	send_go_signal();
	set_led(3,1);

	while(tdt_running && !tdt_timeout)
		dloop_empty();

	set_led(3,0);
	if (!debugflag) alarm(0);

	if (!tdt_timeout) {
		DEBUG(stderr, "tdt_client: recieved done signal\n");
		fflush(stderr);
		fscanf(tdtproc_fp, "%d", &result);

		switch(result) {
			case DP_OK:
				break;
			case DP_MISSED:
				if (is_report_errors)
					notify("iserver: %s, trying again", index(buf, ':') + 1);
				status = IS_ERROR;
				break;
			default:
				if (is_report_errors)
					alert(index(buf, ':') + 1);
				is_shutdown();
				status = IS_ERROR;
				break;
		}
	}

	return(status);
}

/* ------------------------------------------------------------------------
   clear_sample_sighandlers: is_sample_aux
   ------------------------------------------------------------------------ */
static void clear_sample_sighandlers(void)
{
	signal(SIGUSR1, SIG_DFL);
	signal(SIGALRM, SIG_DFL);
}

/* ----------------------------------------------------------------------
   is_shutdown
   ---------------------------------------------------------------------- */
int is_shutdown(void)
{
	DEBUG(stderr, "iserver: is_shutdown() called\n");

	if (!is_testmode) {
		if (tdtproc_pid != -1) {
			if (tdtproc_running())
				send_die_signal();
			tdtproc_pid = -1;
		}
		if (tdtproc_fp != NULL) {
			if (pclose(tdtproc_fp) == -1)
				perror("iserver: pclose\n");

			tdtproc_fp = NULL;
		}
	}

	if (is_inited) {
		if (is_outputBuffer != NULL)
			shm_release(is_outputBuffer, is_outId);

		is_outputBuffer = NULL;
		is_outputSize = 0;
		is_outId = -1;

		if (is_inputBuffer != NULL)
			shm_release(is_inputBuffer, is_inId);

		is_inputBuffer = NULL;
		is_inputSize = 0;
		is_inId = -1;

		if (is_spikeBuffer != NULL)
			shm_release(is_spikeBuffer, is_spikeId);

		is_spikeBuffer = NULL;
		is_spikeSize = 0;
		is_spikeId = -1;
		is_inited = 0;
	}

	set_led(3, 0);

	allow_detach();

	return(IS_OK);
}

/* ----------------------------------------------------------------------
   is_halt
   ---------------------------------------------------------------------- */
int is_halt(
	int nsamps)
{
	return(IS_OK);
}


/* -----------------------------------------------------------------------
   						     SIGNAL HANDLERS
/* ------------------------------------------------------------------ */
/* sigchld_handler                                                    */
/* ------------------------------------------------------------------ */
static void sigchld_handler(
	int sig)
{
	fprintf(stderr, "iserver: tdtproc has died!\n");
}

/* -----------------------------------------------------------------------
   tdt_timeout_handler
   ----------------------------------------------------------------------- */
static void tdt_timeout_handler(
	int	sig)
{
    tdt_timeout = 1;
}

/* -----------------------------------------------------------------------
   tdt_done_handler
   ----------------------------------------------------------------------- */
static void tdt_done_handler(
	int sig)
{
    tdt_running = 0;
}

/* -----------------------------------------------------------------------
   tdt_ready_handler
   ----------------------------------------------------------------------- */
static void tdt_ready_handler(
	int	sig)
{
    tdt_ready = 1;
    signal(SIGUSR2, tdt_ready_handler);
}


/* -----------------------------------------------------------------------
   shm_request
   ----------------------------------------------------------------------- */
static char *shm_request(
    int     key,
    int     nbytes, 
	int		*id_ptr)
{
    int     i;
    void   *block;

    if (!is_testmode) {
		while ((*id_ptr = shmget((key_t) key, nbytes, 
			IPC_CREAT | SHMEM_PERMS)) < 0) {

			/* shmget failed: tell us about it */
			system("/usr/bin/ipcs -m");
			fprintf(stderr, "iserver: shm_request: "
				"shmget(key=%d, nbytes=%d, perm=O%o)\n", key, nbytes, 
				IPC_CREAT | 00666);
			perror("shmget");

			/* now, what should we do?? */
			i = pop_box("iserver: shm_request failed, check shm status",
				"Retry", "Fail", "Exit");

			switch (i) {
				case 2:
					return (NULL);
				case 3:
					fprintf(stderr, "iserver: Aborting due to failed shmget()\n");
					exit(1);
			}
		}
		block = (void *) shmat(*id_ptr, (char *) 0, 0);
		if ((int) block == -1) {
			perror("iserver: shm_request: shmat");
			return (NULL);
		}

		return (block);
    } else {
		return ((char *) calloc(nbytes, sizeof(char)));
    }
}

/* -----------------------------------------------------------------------
   shm_release
   ----------------------------------------------------------------------- */
static int shm_release(
    void   *block,
    int     id)
{

    if (!is_testmode) {
		if (shmdt(block) == -1) {
			perror("iserver: shmdt");
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

