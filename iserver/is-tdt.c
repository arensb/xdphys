/******************************************************************
**  RCSID: $Id: is-tdt.c,v 1.19 2000/06/13 19:07:01 cmalek Exp $
** Program: iserver
**  Module: is-tdt.c
**  Author: cmalek
** Descrip: linux/tdt specific I.S. (Integral Server) code
**
**
** ENVIRONMENT VARIABLES:
**
**  DAQPROG:	define this to be the location of the tdtproc 
**				executable, if you want.
**
*******************************************************************/

/******************************************************************\
|*  is-tdt.c: Linux + Tucker Davis Specific Code (libtdt)        *|
\******************************************************************/

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/time.h>


/* ------------------------------------------------
   Macros
   ------------------------------------------------ */
#define DEBUG	if (debugflag) fprintf
#define SHMEM_PERMS		00666

#define IS_BEST_FC		48000.0
#define IS_PLAY_KEY		12345
#define IS_REC_KEY		22345
#define IS_SPIKE_KEY	42345
#define IS_CONTROL_KEY		25106

#define IS_FREQ_TOL		100.0

#define FREQ_USE_NEW	0
#define FREQ_USE_EXISTING	1
#define FREQ_ERROR	-1

#define MAX_SHM_TRIES 3

#define send_die_signal() kill(tdtproc_pid, SIGQUIT)
#define send_load_signal() kill(tdtproc_pid, SIGHUP)
#define send_play_signal() if (is_split) kill(tdtproc_pid, SIGUSR1); \
							else kill(tdtproc_pid, SIGHUP);
#define send_record_signal() kill(tdtproc_pid, SIGUSR2)
#define tdtproc_running() ((kill(tdtproc_pid, 0) == 0) ? 1 : 0)

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
} FREQ;

/* ------------------------------------------------
   Public Variables
   ------------------------------------------------ */
float *is_attBuffer = NULL;
int 	is_useAtt = 1;
int 	is_useEq = 1;

int 	is_useLED = 0;
int		is_LEDdur = 50;
int		is_LEDdelay = 130;

int		is_maxSpikes = 1000;

unsigned long *is_spikeBuffer = NULL;
int 	is_spikeSize = 0;
int 	is_useET1 = 1;

float 	is_leftatten = 90.0;
float 	is_rightatten = 90.0;

/* ------------------------------------------------
   Private Variables
   ------------------------------------------------ */
static int is_first_init = 1;
static int is_outId = -1;
static int is_inId = -1;
static int is_spikeId = -1;
static int is_tdtControlId = -1;
static int is_split = 0;
static TDTcontrol* is_tdtControlBuf = NULL;
static int is_tdtControlSize = 0;
static FREQ last;

static FILE *tdtproc_fp = NULL;
static int tdtproc_pid = -1;

volatile sig_atomic_t tdt_ready;
volatile sig_atomic_t tdt_running;
volatile sig_atomic_t tdt_timeout;

/* ------------------------------------------------
   Prototypes
   ------------------------------------------------ */
/* is_init related functions */
static void initialize_freq(FREQ *);
static int determine_samp_freqs(FREQ *);
static int validate_freqs(FREQ *);
static int reuse_old_freqs(FREQ *);
static int check_last_freqs(FREQ *);
static void allocate_shared_mem(FREQ *);
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
static void setup_controlbuf(void);

/* is_sample related functions */
static int tdtproc_started(void);
static int setup_sample_sighandlers(void);
static int send_tdtproc_signal(int);
static int trigger_tdtproc(int, double *);
static void clear_sample_sighandlers(void);
static void silence_channel(int);

#if(0)
static int check_flags(int);
static void save_channel(int);
static void restore_channel(int);
#endif

/* --------------------------------------------------------------------
   is_getSpikebuffer
   -------------------------------------------------------------------- */
unsigned long *is_getSpikebuffer(int *nsamps)
{
	if (is_spikeBuffer != NULL) {
		/* The first spike time is always 0 for some reason, so 
		   we'll skip that one */
		if (nsamps != NULL) {
			if (is_spikeBuffer[0]) 
				*nsamps = is_spikeBuffer[0]-1;
			else 
				*nsamps = 0;

			return(is_spikeBuffer+2);
		} else {
			if (nsamps != NULL)
				*nsamps = 0;
			return(NULL);
		}
	}

	/* Should never reach here */
	return(NULL);
}

/* --------------------------------------------------------------------
   is_clearInput
   -------------------------------------------------------------------- */
void is_clearSpikes(void)
{
	bzero((char *) is_spikeBuffer, is_spikeSize * sizeof(unsigned long));
}

/* ----------------------------------------------------------------------
   is_init

	   The spike_mode returned by getSpikeRaster_getmode() must be
	   "ttl" to enable gathering spikes via the SD1/ET1.

	   pass evt_fc > 0.0 to turn on spike gathering

	   pass evt_fc < 0.0 to separate load waveform and play for tdtproc
	   					 for SIGHUP signal.

   ---------------------------------------------------------------------- */
int is_init(
    float   play_fc,									/* hz */
    float   rec_fc,										/* hz */
    float   evt_fc,										/* hz */
    int     play_dur_ms,                               /* milliseconds */
    int     rec_dur_ms)                                /* milliseconds */
{
	FREQ	current;
	int		status = IS_OK;

	if (is_first_init) {
		initialize_freq(&last);
		is_first_init = 0;
	}

	/* Always use SD1/ET1 if evt_fc > 0.0 */
	if (evt_fc > 0.0) {
		current.rec = play_fc;
		is_useET1 = 1;
	} else {
		current.rec = rec_fc;
		is_useET1 = 0;
	}

	if (evt_fc < 0.0) 
		is_split = 1;

	current.play = play_fc;
	current.play_dur = play_dur_ms;
	current.rec_dur = rec_dur_ms;

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
				is_adFc = is_daFc = is_rnd2int(current->play);
			} else {
				status = FREQ_ERROR;
			}
			break;
		case FREQ_USE_EXISTING:
			break;
		case FREQ_ERROR:
			is_adFc = -1;
			is_daFc = -1;
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
#define DIFFERENT_ARGS (!is_inited || \
	abs(current->play-last.play) > IS_FREQ_TOL || \
	abs(current->rec-last.rec) > IS_FREQ_TOL || \
	current->play_dur != last.play_dur || \
	current->rec_dur != last.rec_dur || is_useET1 != last.useET1)

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
				(*is_alert)("iserver:\n No last freqs, must specify "
					"AD/DA samp freqs"); 
				status = FREQ_ERROR;
			} 
		}
		
	} else {
		if (!DIFFERENT_ARGS) {
			DEBUG(stderr, "iserver: reusing existing tdtproc\n");

#if (0)
			forbid_detach();
#endif 
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
		(*is_alert)("iserver:\n At least one of the AD/DA freqs must be"
			" != IS_NONE");
		return(IS_ERROR);
	}

	if (current->play != current->rec) {
		current->rec = current->play;
		/* (*is_alert)("iserver:\nAll sampling frequencies must be equal");
		return (IS_ERROR); */
	}

	return(IS_OK);
}

/* ----------------------------------------------------------------------
   allocate_shared_mem: is_init
   ---------------------------------------------------------------------- */
static void allocate_shared_mem(
	FREQ *current)
{
	int bufsize = buffer_size(current);

	allocate_one_buffer(IS_PLAY_KEY, (void **) &is_outputBuffer, 
		&is_outputSize, &is_outId, da_nchans, bufsize, sizeof(xword));
	allocate_one_buffer(IS_REC_KEY, (void **) &is_inputBuffer, &is_inputSize, 
		&is_inId, ad_nchans, bufsize, sizeof(xword));
	/* ET1 spike buffer */
	if (is_useET1) {
		allocate_one_buffer(IS_SPIKE_KEY, (void **) &is_spikeBuffer, 
			&is_spikeSize, &is_spikeId, 1, is_maxSpikes, sizeof(unsigned long));
	} else {
		if ((is_spikeBuffer != NULL) && (is_spikeId != -1)) {
			shm_release(is_spikeBuffer, is_spikeId);
			is_spikeBuffer = NULL;
			is_spikeSize = 0;
			is_spikeId = -1;
		}
	}
	allocate_one_buffer(IS_CONTROL_KEY, (void **) &is_tdtControlBuf, 
		&is_tdtControlSize, &is_tdtControlId, 1, 1, sizeof(TDTcontrol));
	if (is_useAtt)
		is_attBuffer = is_tdtControlBuf->att;
} 

/* ----------------------------------------------------------------------
   buffer_size: is_init
   		
   		current->play and current->rec are in Hz
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
		bufsize = (current->play_dur * current->play / 1000.0);
	else
		bufsize = (current->rec_dur * current->rec / 1000.0);


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

		setup_controlbuf();
		save_freqs(current);
		clear_buffers();
	}

	return(status);
}

/* ------------------------------------------------------------------------
   setup_controlbuf: is_init
   ------------------------------------------------------------------------ */
static void setup_controlbuf(void)
{
	is_tdtControlBuf->use_led=is_useLED;
	is_tdtControlBuf->led_dur=is_LEDdur;
	is_tdtControlBuf->led_delay=is_LEDdelay;

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
		} else
			break;
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

		if (is_yesno) {
			sprintf(buf, "iserver: tdtproc locked by proc #%d\nOverride anyway?", tmp);
			if ((*is_yesno)(buf, "Yes", "No", NULL) == 2)
				return(IS_ERROR);
			else
				return(IS_OK);
		} else {
			fprintf(stderr, "iserver: tdtproc locked by proc #%d\n", tmp);
			return(IS_ERROR);
		}
	} else {
		if (tdtproc_pid == 0) {
			pclose(tdtproc_fp);
			(*is_alert)("iserver:\nCan't remove tdtproc lock");

			return (IS_ERROR);
		} 
	}

	/* Get our actual sampling frequency */
	fscanf(tdtproc_fp, "%d", &is_adFc);

	is_daFc = is_adFc;

	if (is_fcSetFn)
		(*is_fcSetFn)(is_daFc, is_adFc, 1000000);

	DEBUG(stderr, "iserver: tdtproc using actual fc=%d\n", is_adFc);

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
	char tmp_arg[256];
	char *tdt_cmd = NULL;

	assert(extra != NULL);

	if (get_tdtproc_location(&cmd) == IS_OK) {
		(void) memset(tmp, 0, 256);

		tdt_cmdstr = "exec %s -nice %d -fc %d -out %d -in %d -adnchans %d"
			" -sig %d -dashmid %d -adshmid %d -cntrlshmid %d %s";
		sprintf(tmp, tdt_cmdstr, cmd, is_nice, is_daFc, is_outputSize, 
			is_inputSize, ad_nchans, getpid(), is_outId, is_inId, is_tdtControlId, extra);
		free((void *) cmd);

		if (is_syncPulse) {
			sprintf(tmp_arg, "-syncpulse");
			strcat(tmp, tmp_arg);
		}

		if (is_useET1) {
			sprintf(tmp_arg, " -et1shmid %d -et1cnt %d", is_spikeId, 
				is_spikeSize);
			strcat(tmp, tmp_arg);
		}

		if (is_useAtt) {
			sprintf(tmp_arg, " -useatt");
			strcat(tmp, tmp_arg);
		}

		if (!is_useEq) {
			sprintf(tmp_arg, " -noio");
			strcat(tmp, tmp_arg);
		}

		if (is_split) {
			sprintf(tmp_arg, " -split");
			strcat(tmp, tmp_arg);
		}

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

	assert(*tdt_loc == NULL);

	if ((temp = getenv("DAQPROG")) != NULL) {

		if (temp != NULL) {
			if (!file_is_executable(temp)) {
				free(temp);
			} else {
				*tdt_loc = (char *) calloc(strlen(temp)+1, sizeof(char));
				assert(*tdt_loc != NULL);
				(void) strcpy(*tdt_loc, temp);
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
#if (0) /* stat() is screwed up in glibc */
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
			fprintf(stderr, "iserver: %s is not a regular file.\n", path);
			status = 0;
		}
	}
#endif

	return(status);
}

/* ----------------------------------------------------------------------
   is_load
   ---------------------------------------------------------------------- */
int is_load(int flags)
{
	int status = IS_OK;
	double timestamp;
	
	if (!is_split)
		return(IS_ERROR);

	flags |= X_LOADONLY;
	status = trigger_tdtproc(flags, &timestamp);

	return(status);
}

/* ----------------------------------------------------------------------
   is_sample
   		
	   ms_dur is only used to compute the timeout interval.
   ---------------------------------------------------------------------- */
int is_sample(
    int     flags,
	int		ms_dur,
	double  *timestamp)
{
	int status = IS_OK;

	silence_channel(flags);

	status = trigger_tdtproc(flags, timestamp);

	DEBUG(stderr, "iserver: timestamp = %f\n", *timestamp);

	return(status);
}
#undef MAX_RETRIES


/* ----------------------------------------------------------------------
   is_record
   ---------------------------------------------------------------------- */
int is_record(void)
{
	int status = IS_OK;
	int flags = X_RECONLY;
	double timestamp;

	if (!is_split)
		return(IS_ERROR);

	status = trigger_tdtproc(flags, &timestamp);

	return(status);
}

/* ----------------------------------------------------------------------
   trigger_tdtproc: is_sample, is_load, is_record
   ---------------------------------------------------------------------- */
#define MAX_RETRIES 10

static int trigger_tdtproc(int flags, double *timestamp)
{
	int status = IS_OK;
	int	retry_count = 0;

	if (!is_testmode) {
#if(1)
		if (tdtproc_started() == IS_OK) {
#else 
		if ((tdtproc_started() == IS_OK) && (check_flags(flags) == IS_OK)) {
#endif
			do {
				if (setup_sample_sighandlers() != IS_OK)
					break;

				status = send_tdtproc_signal(flags);
				retry_count++;
			} while ((status != IS_OK) && (retry_count < MAX_RETRIES)); 

			if (retry_count == MAX_RETRIES) {
				if (is_report_errors)
					(*is_alert)("iserver: giving up");
				is_shutdown();
				status = IS_ERROR;
			}

			if (tdt_timeout) {
				if (is_report_errors)
					(*is_alert)("iserver:\ntdtproc timed out");
				status = IS_ERROR;
			}

			clear_sample_sighandlers();
		}

		if (status == IS_OK)
			*timestamp = is_tdtControlBuf->timestamp;
		else
			*timestamp = -1.0;
	}


	return(status);
}


/* ----------------------------------------------------------------------
   silence_channel: is_sample, is_load
   ---------------------------------------------------------------------- */
static void silence_channel(
    int     flags)
{
	is_tdtControlBuf->channel = IS_BOTH;

	if (!(flags & X_LEFT) && !(flags & X_RIGHT)) 
		return;

	if (!((flags & X_LEFT) && (flags & X_RIGHT))) {
		if (flags & X_LEFT) {
			is_tdtControlBuf->channel = IS_LEFT;

			DEBUG(stderr, "iserver: silenced RIGHT channel\n");
		} else {
			is_tdtControlBuf->channel = IS_RIGHT;
			DEBUG(stderr, "iserver: silenced LEFT channel\n");
		}
	} 
}

/* ----------------------------------------------------------------------
   tdtproc_started: trigger_tdtproc
   ---------------------------------------------------------------------- */
static int tdtproc_started(void)
{
	int	status = IS_OK;

	if (tdtproc_pid < 0 || !is_inited) {
		(*is_alert)("iserver:\ntdtproc never started");
		is_shutdown();
		status = IS_ERROR;
	}
	else {
		if (!tdtproc_running()) {
			if (is_yesno) {
				if ((*is_yesno)("iserver: tdtproc dead, restart?", "Ok", "Fail", 
					NULL) == 1) {
					is_init(IS_LAST, IS_LAST, IS_LAST, 0, 0);
				} else {
					is_shutdown();
					status = IS_ERROR;
				}
			} else {
				is_init(IS_LAST, IS_LAST, IS_LAST, 0, 0);
			}
		}
	}

	return(status);
}

#if(0)
/* ----------------------------------------------------------------------
   check_flags: trigger_tdtproc

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
		(*is_alert)(buf);
		fprintf(stderr, "%s\n", buf);
		status = IS_ERROR;
	}

	return(status);
}
#endif

/* ------------------------------------------------------------------------
   setup_sample_sighandlers: trigger_tdtproc
   ------------------------------------------------------------------------ */
static int setup_sample_sighandlers(void)
{

	alarm(10);
	signal(SIGUSR1, tdt_done_handler);
	signal(SIGALRM, tdt_timeout_handler);

	tdt_timeout = 0;
	tdt_running = 1;

	return(IS_OK);
}


/* ------------------------------------------------------------------------
   send_tdtproc_signal: trigger_tdtproc
   ------------------------------------------------------------------------ */
#define suspend()		{sigemptyset(&mask); \
				if (sigsuspend(&mask) == -1) \
					if (errno != EINTR) \
											perror("iserver: ");}
static int send_tdtproc_signal(int flags)
{
	int status = IS_OK;
	int	result;
	sigset_t	mask;
	char buf[256];
	char *end;

	if (flags & X_LOADONLY) {
		DEBUG(stderr, "iserver: tdtproc ready: sending load signal\n");
		fflush(stderr);
		send_load_signal();
	} else {
		if (flags & X_RECONLY) {
			DEBUG(stderr, "iserver: tdtproc ready: sending rec signal\n");
			fflush(stderr);
			send_record_signal();
		} else {
			DEBUG(stderr, "iserver: tdtproc ready: sending play signal\n");
			fflush(stderr);
			send_play_signal();
		}
	}

	while(tdt_running && !tdt_timeout)
		suspend();

	alarm(0);

	if (!tdt_timeout) {
		DEBUG(stderr, "iserver: recieved done signal\n");
		fflush(stderr);

		fscanf(tdtproc_fp, "%s", buf);
		end=index(buf,':');
		*end = '\0';
		sscanf(buf, "%d", &result);

		switch(result) {
			case IS_TRANS_OK:
				break;
			case IS_TRANS_MISSED:
				if (is_report_errors)
					(*is_notify)("iserver: missed, trying again");
				status = IS_ERROR;
				break;
			default:
				if (is_report_errors)
					(*is_alert)("iserver: unknown error from tdtproc");
				is_shutdown();
				status = IS_ERROR;
				break;
		}
	}

	return(status);
}

#undef suspend

/* ------------------------------------------------------------------------
   clear_sample_sighandlers: trigger_tdtproc
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

		if (is_tdtControlBuf != NULL)
			shm_release(is_tdtControlBuf, is_tdtControlId);

		is_tdtControlBuf = NULL;
		is_tdtControlSize = 0;
		is_tdtControlId = -1;

		is_attBuffer = NULL;

		is_inited = 0;
	}

#if (0)
	allow_detach();
#endif

	return(IS_OK);
}


/* ----------------------------------------------------------------------
   is_et1_enable
   ---------------------------------------------------------------------- */
void is_et1_enable(void)
{
	is_useET1 = 1;
}

/* ----------------------------------------------------------------------
   is_et1_disable
   ---------------------------------------------------------------------- */
void is_et1_disable(void)
{
	is_useET1 = 0;
}

/* ----------------------------------------------------------------------
   is_led_enable
   ---------------------------------------------------------------------- */
void is_led_enable(void)
{
	is_useLED=1;
}

/* ----------------------------------------------------------------------
   is_led_disable
   ---------------------------------------------------------------------- */
void is_led_disable(void)
{
	is_useLED=0;
}

/* ----------------------------------------------------------------------
   is_setLedDur
   ---------------------------------------------------------------------- */
void is_setLedDur(int dur)
{
	is_LEDdur=dur;
}


/* ----------------------------------------------------------------------
   is_setLedDelay
   ---------------------------------------------------------------------- */
void is_setLedDelay(int delay)
{
	is_LEDdelay=delay;
}


/* ----------------------------------------------------------------------
   is_setatten
   ---------------------------------------------------------------------- */
void is_setAtten(
	float latten,
	float ratten)
{
	if (is_attBuffer != NULL) {
		is_attBuffer[0] = latten;
		is_attBuffer[1] = ratten;
	}
}



/* ------------------------------------------------------------------ */
/* sigchld_handler                                                    */
/* ------------------------------------------------------------------ */
static void sigchld_handler(
	int sig)
{
	DEBUG(stderr, "iserver: tdtproc has died!\n");
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
		int			count=0;
    void   *block;

    if (!is_testmode) {
		while ((*id_ptr = shmget((key_t) key, nbytes, 
			IPC_CREAT | SHMEM_PERMS)) < 0)  {

			/* now, what should we do?? */
			if (count < MAX_SHM_TRIES) {
					count++;
					fprintf(stderr, "iserver: shmget failed (%s), retry #%d\n",
							strerror(errno), count);
					sleep(2);
					continue;
			} else {
					/* shmget failed: tell us about it */
					system("/usr/bin/ipcs -m");
					fprintf(stderr, "iserver: shm_request: "
						"shmget(key=%d, nbytes=%d, perm=O%o)\n", key, nbytes, 
						IPC_CREAT | 00666);
					perror("shmget");


					if (is_yesno) {
						i = (*is_yesno)(
							"iserver: shm_request failed, check shm status",
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
						fprintf(stderr, 
							"iserver: Aborting due to failed shmget()\n");
						exit(1);
					}
			}
		}
		block = (void *) shmat(*id_ptr, (char *) 0, 0);
		if (block == (void *) -1) {
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
		} 

		if (shmctl(id, IPC_RMID, NULL) == -1) {
		    perror("iserver: shmctl(IPC_RMID)"); 
			return (0);
		}
		return (1);
    } else {
		free(block);
		return (1);
    }
}


#ifdef __test__
void is_setUseAtten(int use_atten)	
{
	is_useAtt = use_atten;
}

int	is_getUseAtten(void)	
{
	return(is_useAtt);
}

void is_setUseEq(int use_eq)	
{
	is_useEq = use_eq;
}

int	is_getUseEq(void)	
{
	return(is_useEq);
}				
#endif

void is_setMaxSpikes(int maxspikes)	
{
	is_maxSpikes = maxspikes;
}

int is_getMaxSpikes(void)	
{
	return(is_maxSpikes);
}


