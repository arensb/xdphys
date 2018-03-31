/******************************************************************
**  RCSID: $Id: tdt_client.c,v 1.12 2000/03/01 23:06:56 cmalek Exp $
** Program: dowl
**  Module: tdt_client.c
**  Author: cmalek
** Descrip: test client for tdtproc
**
** Environment Variables:
**
**      TDTCLIENT_DEBUG:		if defined, turn on debug messages
**
*******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <curses.h>

#include <tdtplot.h>
#include <daq.h>

#define USER_STEP

/* ------------------------------------------------
   Macros
   ------------------------------------------------ */
#define SUCCESS 0
#define FAIL -1
#define DEBUG	if (debugflag) fprintf
#define SHMEM_PERMS		0666

#define LEFT		0
#define RIGHT		1

#ifndef COMPILE_DATE
#define COMPILE_DATE			" "
#endif /*COMPILE_DATE*/
#ifndef OSINFO
#define OSINFO				" "
#endif /*OSINFO*/

#define VERSION 					"1.2"
#define IS_TRANS_OK		0
#define IS_TRANS_MISSED	1

/* ------------------------------------------------
   Typedefs
   ------------------------------------------------ */
static struct ctx_struct {
	short 	*play_sound;				
	short 	*rec_sound;				
#ifdef _FREE_FIELD_
	unsigned long 	*spikes;				
#endif /* _FREE_FIELD_ */
	TDTcontrol 	*control;				
	int		play_shmid;
	int		rec_shmid;

	int		fc_actual;

#ifdef _DICHOTIC_
	int		use_et1;
	int		spikes_shmid;
#endif /* _DICHOTIC_ */
	int 	nsamples;		

	int		latten;
#ifdef _DICHOTIC_
	int		ratten;
#endif /* _DICHOTIC_ */
	int		control_shmid;
	int		use_att;

#ifdef _DICHOTIC_
	int 	nspikes;		
#endif /* _DICHOTIC_ */
	int		nice;
	int 	fc;					/* sampling frequency in Hz */
	int		duration;			/* tone duration in msec */
	int		freq; 				/* tone freq in Hz */

	int 	rise;
	int		fall;

	FILE 	*tdtproc_fp;
	int		tdt_pid;

	plotinfo p0;
	int		use_graphics;
	int		use_strace;
	int		split;
	int		channel;


} ctx;

volatile sig_atomic_t tdt_inited;
volatile sig_atomic_t tdt_ready;
volatile sig_atomic_t tdt_running;
volatile sig_atomic_t tdt_timeout;


/* ------------------------------------------------
   Global variables
   ------------------------------------------------ */
int debugflag;

/* ------------------------------------------------
 * Static variables
 * ------------------------------------------------ */
static char *rcs_revision = "$Revision: 1.12 $";
static char *rcs_name = "$Name:  $";
static char *rcs_date = "$Date: 2000/03/01 23:06:56 $";
static char *rcs_path = "$Source: /home/konishi11/cvsroot/iserver/tdtproc/tdt_client.c,v $";

/* ------------------------------------------------
 * Static function prototypes
 * ------------------------------------------------ */
static void initialize_ctx(void);
static void parse_env(void);
static void parse_cmdline(int, char **);
static void create_shared_mem(void);
static void check_retval(void);
static void setup_graphics(void);
static void cleanup_graphics(void);
static void startup(int, char **);
static void cleanup(void);
static void command_info(void);
static int get_one_shm_buffer(int, void **);
static int launch_tdtproc(void);
static void synth_sound(void);
static void perform_trial(void);
static void sigchld_handler(int);
static void register_signal_handlers(void);
static void register_io_signal_handlers(void);
static void deinterlace_recbuf(short *, int);
static void display_waveforms(void);
static char *format_cmdstring(void);

/* ------------------------------------------------
   main
   ------------------------------------------------ */
int main(
	int	argc,
	char **argv)
{
	char 	c;
	int		i=0;

	startup(argc, argv);

	for (;;) {

#ifdef USER_STEP
		do {
			c = getch();
		} while (c == ERR);

	    if (c == 'x' || c == 'X') 
			break;

		if (c == 'l' || c == 'L') {
			fprintf(stderr, "tdt_client: playing only left channel\n");
			fflush(stderr);
			ctx.channel = IS_LEFT;
			continue;
		}

		if (c == 'r' || c == 'R') {
			fprintf(stderr, "tdt_client: playing only right channel\n");
			fflush(stderr);
			ctx.channel = IS_RIGHT;
			continue;
		}

		if (c == 'b' || c == 'B') {
			fprintf(stderr, "tdt_client: playing both channels\n");
			fflush(stderr);
			ctx.channel = IS_BOTH;
			continue;
		}



#else /* USER_STEP */

		c = getch();
	    if (c == 'x' || c == 'X') 
			break;

#endif /* USER_STEP */
		perform_trial();

		i++;
		fprintf(stderr, "<Iteration #%d>\n", i);
		fflush(stderr);
		
	}

	fprintf(stderr, "tdt_client: cleaning up ...\n");

	cleanup();

	fprintf(stderr, "tdt_client: exiting ...\n");

	return(SUCCESS);
}
/* ------------------------------------------------------------------
   						     SIGNAL HANDLERS
   ------------------------------------------------------------------ 
   sigchld_handler                                                    
   ------------------------------------------------------------------ */
void sigchld_handler(int sig)
{
	fprintf(stderr, "tdt_client: tdtproc has died!\n");
	tdt_inited = 0;
}

/* -----------------------------------------------------------------------
   tdt_timeout_handler
   ----------------------------------------------------------------------- */

void tdt_timeout_handler(
	int	sig)
{
    tdt_timeout = 1;
}
/* -----------------------------------------------------------------------
   tdt_done_handler
   ----------------------------------------------------------------------- */

void tdt_done_handler(
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
   						     LOCAL FUNCTIONS
   ------------------------------------------------------------------------
   startup   
   ------------------------------------------------------------------------ */
static void startup(
	int 	argc, 
	char 	**argv)
{


	parse_env();
	initialize_ctx();
	parse_cmdline(argc, argv);

	initscr();
	cbreak();
	noecho();
	nodelay(stdscr, TRUE);

	create_shared_mem();
	register_signal_handlers();
	setup_graphics();
	synth_sound();

	if (launch_tdtproc() == FAIL) 
		cleanup();

	assert(ctx.tdt_pid > 0);
	tdt_inited = 1;
}


/* ------------------------------------------------------------------------
   cleanup:    Cleanup files and shared memory before exiting.
   ------------------------------------------------------------------------ */

static void cleanup(void)
{
	DEBUG(stderr, "tdt_client: cleaning up\n");

	endwin();

	cleanup_graphics();

	if (tdt_inited) {
		kill(ctx.tdt_pid, SIGQUIT);

		DEBUG(stderr, "tdt_client: sent SIGQUIT to tdtproc\n");
	}

	if (ctx.play_shmid >= 0) {
		if (shmctl(ctx.play_shmid, IPC_RMID, NULL) != -1)
			DEBUG(stderr, "tdt_client: removed play shared memory\n");
		ctx.play_shmid = -1;
	}

	if (ctx.rec_shmid >= 0) {
		if (shmctl(ctx.rec_shmid, IPC_RMID, NULL) != -1)
			DEBUG(stderr, "tdt_client: removed rec shared memory\n");
		ctx.rec_shmid = -1;
	}

	if (ctx.control_shmid >= 0) {
		if (shmctl(ctx.control_shmid, IPC_RMID, NULL) != -1)
			DEBUG(stderr, "tdt_client: removed rec shared memory\n");
		ctx.control_shmid = -1;
	}

#ifdef _DICHOTIC_
	if (ctx.use_et1) {
		if (ctx.spikes_shmid >= 0) {
			if (shmctl(ctx.spikes_shmid, IPC_RMID, NULL) != -1)
				DEBUG(stderr, "tdt_client: removed rec shared memory\n");
			ctx.spikes_shmid = -1;
		}
	}
#endif /* _DICHOTIC_ */


	DEBUG(stderr, "tdt_client: finished cleanup\n");
}

/* ------------------------------------------------------------------------
	initialize_ctx
   ------------------------------------------------------------------------ */
static void initialize_ctx(void) 
{
	ctx.play_sound = NULL;
	ctx.rec_sound = NULL;
	ctx.control = NULL;
	ctx.play_shmid = -1;
	ctx.rec_shmid = -1;
#ifdef _DICHOTIC_
	ctx.spikes	= NULL;
	ctx.use_et1 = 0;
	ctx.spikes_shmid = -1;
	ctx.nspikes = 1000;
#endif /* _DICHOTIC_ */
	ctx.use_att = 0;
	ctx.latten = 1.0;
#ifdef _DICHOTIC_
	ctx.ratten = 1.0;
#endif /* _DICHOTIC_ */
	ctx.use_graphics = 0;
	ctx.fc = 32000;
	ctx.freq = 400;
	ctx.nsamples = 0;
	ctx.duration = 1000;
	ctx.rise = 10;
	ctx.fall = 10;
	ctx.nice = -1000;
	ctx.tdtproc_fp = NULL;
	ctx.tdt_pid = -1;
	tdt_ready = 0;
	tdt_running = 0;
	tdt_timeout = 0;
	ctx.use_strace = 0;
	ctx.split = 0;
	ctx.channel = IS_BOTH;
}

/* ------------------------------------------------------------------------
	parse_env
   ------------------------------------------------------------------------ */
static void parse_env(void) 
{
	if (getenv("TDTCLIENT_DEBUG") != NULL)
		debugflag = 1;
	else
		debugflag = 0;

	if (getenv("TDTCLIENT_STRACE") != NULL)
		ctx.use_strace = 1;
}

/* ------------------------------------------------------------------------
	parse_cmdline
   ------------------------------------------------------------------------ */
#define DEBUG_SWITCH(arg, label, val) DEBUG(stderr, \
	"tdt_client: parsed %s switch: %s=%d\n", arg, label, val)

static void parse_cmdline(
	int		ac,
	char	**av)
{
	char		c;
	static struct option longopts[] = {
		{"att", 0, 0, 'a'},
		{"latten", 1, 0, 'l'},
		{"ratten", 1, 0, 'r'},
		{"duration", 1, 0, 'd'},
		{"spikes", 0, 0, 'e'},
		{"fc", 1, 0, 'F'},
		{"split", 1, 0, 'p'},
		{"tonefreq", 1, 0, 'f'},
		{"graph", 0, 0, 'g'},
		{"nspikes", 1, 0, 'g'},
		{"strace", 1, 0, 'S'},
		{"version", 1, 0, 'S'},
		{"help", 1, 0, 'h'},
		{0, 0, 0, 0}};
	int options_index = 0;

	while (1) {

		if ((c = getopt_long_only(ac, av, "adl:eF:f:g:n:pr:sSvh?",
			longopts, &options_index)) == -1)
			break;

		switch (c) {
			case 'a':
				ctx.use_att = atoi(optarg);
				DEBUG_SWITCH("-a", "use_att", ctx.use_att);
				break;
			case 'd':
				ctx.duration = atoi(optarg);
				DEBUG_SWITCH("-d", "duration", ctx.duration);
				break;
			case 'e':
#ifdef _DICHOTIC_
				ctx.use_et1 = 1;
				DEBUG_SWITCH("-e", "use_et1", ctx.use_et1);
#endif /* _DICHOTIC_ */
#ifdef _FREE_FIELD_
				fprintf(stderr, " -e (use_et1) option ignored\n");
#endif /* _FREE_FIELD_ */
				break;
			case 'F':
				ctx.fc = atoi(optarg);
				DEBUG_SWITCH("-F", "fc", ctx.fc);
				break;
			case 'f':
				ctx.freq = atoi(optarg);
				DEBUG_SWITCH("-f", "tonefreq", ctx.freq);
				break;
			case 'g':
				ctx.use_graphics = 1;
				DEBUG_SWITCH("-g", "use_graphics", ctx.use_graphics);
				break;
			case 'l':
				ctx.latten = atoi(optarg);
				DEBUG_SWITCH("-l", "latten", ctx.latten);
				break;
			case 'p':
				ctx.split = 1;
				DEBUG_SWITCH("-p", "split", ctx.split);
				break;
			case 'r':
#ifdef _DICHOTIC_
				ctx.ratten = atoi(optarg);
				DEBUG_SWITCH("-r", "ratten", ctx.ratten);
#endif /* _DICHOTIC_ */
#ifdef _FREE_FIELD_
				fprintf(stderr, " -r (ratten) option ignored\n");
#endif /* _FREE_FIELD_ */
				break;
			case 's':
#ifdef _DICHOTIC_
				ctx.nspikes = atoi(optarg);
				DEBUG_SWITCH("-s", "nspikes", ctx.nspikes);
#endif /* _DICHOTIC_ */
#ifdef _FREE_FIELD_
				fprintf(stderr, " -s (nspikes) option ignored\n");
#endif /* _FREE_FIELD_ */
				break;
			case 'S':
				ctx.use_strace = 1;
				DEBUG_SWITCH("-S", "use_strace", ctx.use_strace);
				break;
			case 'n':
				ctx.nice = atoi(optarg);
				DEBUG_SWITCH("-n", "nice", ctx.nice);
				break;
			case 'v':
#ifdef _DICHOTIC_
				printf("\ntdt_client Version %s (%s rig)\n", VERSION,
					"DICHOTIC");
#endif /* _DICHOTIC_ */
#ifdef _FREE_FIELD_
				printf("\ntdt_client Version %s (%s rig)\n", VERSION,
					"FREE FIELD");
#endif /* _FREE_FIELD_ */
				printf("----------------------------------------------------\n");
				printf("Compile Date: %s\n", COMPILE_DATE);
				printf("OS Info: %s\n", OSINFO);
				printf("----------------------------------------------------\n");
  				printf("%s\n", rcs_path);
				printf("%s\n", rcs_name);
				printf("%s\n", rcs_revision);
				printf("%s\n", rcs_date);
				printf("----------------------------------------------------\n");
				exit(SUCCESS);
				break;
			
			case 'h':
			case '?': 
				command_info();
				exit(SUCCESS);
				break;
		}
	}
}
#undef DEBUG_SWITCH


/* ------------------------------------------------------------------------
	setup_graphics
   ------------------------------------------------------------------------ */
static void setup_graphics(void)
{
	if (ctx.use_graphics) {
		initplotinfo(&ctx.p0);            

		ctx.p0.ymin=-32767.0;   
		ctx.p0.ymax=32768.0;
		ctx.p0.xmin=0.0;     
		ctx.p0.xmax=100.0;
		ctx.p0.samp_period = 1e6/((double) ctx.fc);
		ctx.p0.skip = 1;
		strcpy(ctx.p0.lab_x,"Milliseconds");
		strcpy(ctx.p0.lab_y," ");
#ifdef _DICHOTIC_
		if (ctx.use_et1)
			strcpy(ctx.p0.title,"Signal From ADC1 of DD1 and spikes from ET1");
		else 
			strcpy(ctx.p0.title,"Signal From ADC1 of DD1");
#endif /* _DICHOTIC_ */
#ifdef _FREE_FIELD_
		strcpy(ctx.p0.title,"Signal From ADC1 of AD2");
#endif /* _FREE_FIELD_ */

		gon();
	}
}

/* ------------------------------------------------------------------------
	cleanup_graphics
   ------------------------------------------------------------------------ */
static void cleanup_graphics(void)
{
	if (ctx.use_graphics) 
		goff();
}

/* ------------------------------------------------------------------------
	synth_sound
   ------------------------------------------------------------------------ */
static void synth_sound(void)
{
	int			i, j, envsamples;
	double		slope, ratio;

	ratio = ((double) ctx.freq) / ((double) (ctx.fc));

	for (i=0; i<ctx.nsamples; i+=2) {
		ctx.play_sound[i] = (short) ((double) (MAXSHORT/2) * (sin(2.0*M_PI*
			ratio * (double) (i/2))));
#ifdef _DICHOTIC_
		ctx.play_sound[i+1] = ctx.play_sound[i];
#endif /* _DICHOTIC_ */
	}

	envsamples = (int) (((double) ctx.rise)/1000.0 * (double) ctx.fc);
	slope = 1.0 / ((double) envsamples);

	for (i=0, j=0; i<2*envsamples; j++,i+=2) {
		ctx.play_sound[i] = (short) (((double) ctx.play_sound[i]) * 
			slope*((double) j));
#ifdef _DICHOTIC_
		ctx.play_sound[i+1] = (short) (((double) ctx.play_sound[i+1]) * 
			slope*((double) j));
#endif /* _DICHOTIC_ */
	}

	envsamples = (int) (((double) ctx.fall)/1000.0 * (double) ctx.fc);
	slope = 1.0 / ((double) envsamples);

	for (i=ctx.nsamples-2, j=0; i>(ctx.nsamples - 2*envsamples); j++,i-=2) {
		ctx.play_sound[i] = (short) (((double) ctx.play_sound[i]) * 
			slope*((double) j));
#ifdef _DICHOTIC_
		ctx.play_sound[i+1] = (short) (((double) ctx.play_sound[i+1]) * 
			1.0-slope*((double) j));
#endif /* _DICHOTIC_ */
	}
}

/* ------------------------------------------------------------------------
    command_info:    
   ------------------------------------------------------------------------ */

static void command_info(void)
{
	fprintf(stderr, "usage:  tdt_client <options>\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "   options are:\n");
#ifdef _DICHOTIC_
	fprintf(stderr, "      -a            use PA4 attenuators\n");
#endif /* _DICHOTIC_ */
#ifdef _FREE_FIELD_
	fprintf(stderr, "      -a            use PA4 attenuator\n");
#endif /* _FREE_FIELD_ */
	fprintf(stderr, "      -d <msec>     tone duration\n");
#ifdef _DICHOTIC_
	fprintf(stderr, "      -e            use ET1/SD1 setup\n");
#endif /* _DICHOTIC_ */
	fprintf(stderr, "      -f <Hz>       tone frequency in Hz\n");
	fprintf(stderr, "      -F <Hz>       sampling freq\n");
	fprintf(stderr, "      -g            display graph of recorded waveform\n");
	fprintf(stderr, "      -n <pri>      make tdtproc run at nice level <pri>\n");
#ifdef _DICHOTIC_
	fprintf(stderr, "      -s <spikes>   # spikes in spike shm buffer\n");
#endif /* _DICHOTIC_ */
	fprintf(stderr, "      -S            use strace to run tdtproc\n");
	fprintf(stderr, "      -p            split play/rec into load, play, rec\n");
	fprintf(stderr, "      -v            version\n");

}

/* ------------------------------------------------------------------------
   create_shared_mem
   ------------------------------------------------------------------------ */
static void create_shared_mem(void)
{
	ctx.nsamples = (int) (2.0 * ((double) ctx.duration/1000.0) * 
		(double) ctx.fc);

	assert(ctx.nsamples > 0);
 	DEBUG(stderr, "tdt_client: nsamples=%d\n",ctx.nsamples);

	ctx.play_shmid = get_one_shm_buffer(ctx.nsamples * sizeof(short), 
		(void *) &ctx.play_sound);
	DEBUG(stderr, "tdt_client: got play shmem\n");

	ctx.rec_shmid = get_one_shm_buffer(ctx.nsamples * sizeof(short), 
		(void *) &ctx.rec_sound);
	DEBUG(stderr, "tdt_client: got rec shmem\n");

	ctx.control_shmid = get_one_shm_buffer(sizeof(TDTcontrol), 
		(void *) &ctx.control);
	DEBUG(stderr, "tdt_client: got control shmem\n");

#ifdef _DICHOTIC_
	if (ctx.use_et1) {
		assert(ctx.nspikes > 0);

		DEBUG(stderr, "tdt_client: nspikes=%d\n",ctx.nspikes);

		ctx.spikes_shmid = get_one_shm_buffer(ctx.nspikes*sizeof(long), 
			(void *) &ctx.spikes);

		DEBUG(stderr, "tdt_client: got spikes shmem\n");
	}
#endif /* _DICHOTIC_ */

}

/* ------------------------------------------------------------------------
   get_one_shm_buffer
   ------------------------------------------------------------------------ */
static int get_one_shm_buffer(
	int		nbytes,
	void	**buf)
{
	int	shmid;

	if ((shmid = shmget(IPC_PRIVATE, nbytes, IPC_CREAT | SHMEM_PERMS)) 
		== -1) {
		perror("tdt_client: shmget");
		exit(FAIL);
	}

	if ((int) (*buf = (void *) shmat(shmid, (char *) 0, 0)) == -1) {
		perror("tdt_client: shmat");
		exit(FAIL);
	}

	return(shmid);
}


  
/* ------------------------------------------------------------------------
   format_cmdstring

   		Remember to free the string returned by this function.
   ------------------------------------------------------------------------ */
static char *format_cmdstring(void)
{
	char *tdt_cmdstr = NULL;
	char *cmd = NULL;
	char tmp[256];
	char tmp_arg[256];
	char *tdt_cmd = NULL;

	if (ctx.use_strace)
		if (geteuid() != 0) {
			fprintf(stderr, "tdt_client: you must be root to use strace"
				" with tdtproc\n");
			cleanup();
			exit(1);
		} else 
			cmd = "strace"; 
	else
		cmd = "";

	tdt_cmdstr = "exec %s tdtproc -fc %d -out %d -in %d"
		" -sig %d -dashmid %d -adshmid %d -cntrlshmid %d";
	sprintf(tmp, tdt_cmdstr, cmd, ctx.fc, ctx.nsamples/2, ctx.nsamples/2,
		getpid(), ctx.play_shmid, ctx.rec_shmid, ctx.control_shmid);
	free(cmd);

#ifdef _DICHOTIC_
	if (ctx.use_et1) {
		sprintf(tmp_arg, " -et1shmid %d -et1cnt %d", ctx.spikes_shmid, 
			ctx.nspikes);
		strcat(tmp, tmp_arg);
	}
#endif /* _DICHOTIC_ */

	if (ctx.use_att) {
		sprintf(tmp_arg, " -useatt");
		strcat(tmp, tmp_arg);
	}

	if (ctx.split) {
		sprintf(tmp_arg, " -split");
		strcat(tmp, tmp_arg);
	}

	tdt_cmd = (char *) calloc(256, sizeof(char));
	assert(tdt_cmd != NULL);

	if (ctx.nice != -1000) 
		sprintf(tdt_cmd, "%s -nice %d", tmp, ctx.nice);
	else
		strcpy(tdt_cmd, tmp);

	DEBUG(stderr, "tdt_client: using following string for tdtproc:\n"
		"tdt_client: %s\n", tdt_cmd);

	return(tdt_cmd);
}

/* ------------------------------------------------------------------------
   launch_tdtproc
   ------------------------------------------------------------------------ */
static int launch_tdtproc(void)
{
	char *tdt_cmd = NULL;

	tdt_cmd = format_cmdstring();
	assert(tdt_cmd != NULL);

	if ((ctx.tdtproc_fp = popen(tdt_cmd, "r")) == NULL) {
		perror("tdt_client: popen");
		return(FAIL);
	}

	free(tdt_cmd);

	sleep(1);

	fscanf(ctx.tdtproc_fp, "%d", &ctx.tdt_pid);
	if (ctx.tdt_pid == -1) {
		fprintf(stderr, "tdt_client: couldn't read pid from tdtproc\n");
		return(FAIL);
	}
	DEBUG(stderr, "tdt_client: tdtproc pid = %d\n", ctx.tdt_pid);

	fscanf(ctx.tdtproc_fp, "%d", &ctx.fc_actual);
	DEBUG(stderr, "tdt_client: tdtproc real fc = %d\n", ctx.fc_actual);

	DEBUG(stderr, "tdt_client: tdtproc launched succssfully\n");

	return(SUCCESS);
}

/* -----------------------------------------------------------------------
   register_signal_handlers
   ----------------------------------------------------------------------- */
static void register_signal_handlers(void)
{
    signal(SIGUSR2, tdt_ready_handler);
    signal(SIGCHLD, sigchld_handler);
}

/* -----------------------------------------------------------------------
   register_io_signal_handlers
   ----------------------------------------------------------------------- */
static void register_io_signal_handlers(void)
{
	alarm(5 * (ctx.duration / 1000 + 1)); 
	signal(SIGUSR1, tdt_done_handler);
	signal(SIGALRM, tdt_timeout_handler);
}

/* ------------------------------------------------------------------------
   perform_trial
   ------------------------------------------------------------------------ */
#define DEBUG_RETVAL(s) DEBUG(stderr, "tdt_client: tdtproc returns %s\n", s)
#define suspend(err)		{sigemptyset(&mask); \
				if (sigsuspend(&mask) == -1) \
					if (errno != EINTR) \
											perror(err);}

static void perform_trial(void)
{
	sigset_t	mask;

	while (!tdt_ready) 
		suspend("tdt_client: sigsuspend");

	DEBUG(stderr, "tdt_client: received tdt_ready signal\n");
	fflush(stderr);


	if (ctx.use_att) {
		ctx.control->att[0] = (float) ctx.latten;
#ifdef _DICHOTIC_
		ctx.control->att[1] = (float) ctx.ratten;
#endif /* _DICHOTIC_ */
	}

	ctx.control->channel = ctx.channel;

	register_io_signal_handlers();

	tdt_running = 1;
	tdt_timeout	= 0;

	if (!ctx.split) {
		DEBUG(stderr, "tdt_client: sending go signal (pid=%d)\n",ctx.tdt_pid);
		fflush(stderr);

		kill(ctx.tdt_pid, SIGHUP);

		while (tdt_running && !tdt_timeout) 
			suspend("tdt_client: sigsuspend");

		alarm(0);

		if (tdt_timeout == 1)
			fprintf(stderr, "tdt_client: tdtproc timed out!\n");
		else {
			DEBUG(stderr, "tdt_client: recieved done signal\n");
			fflush(stderr);

			check_retval();
		}

	} else {
		DEBUG(stderr, "tdt_client: sending load signal (pid=%d)\n",
			ctx.tdt_pid);
		fflush(stderr);

		kill(ctx.tdt_pid, SIGHUP);

		while (tdt_running && !tdt_timeout) 
			suspend("tdt_client: sigsuspend");

		alarm(0);

		if (tdt_timeout == 1)
			fprintf(stderr, "tdt_client: tdtproc timed out!\n");
		else {
			DEBUG(stderr, "tdt_client: (load) recieved done signal\n");
			fflush(stderr);

			check_retval();
		}

		register_io_signal_handlers();

		DEBUG(stderr, "tdt_client: sending play signal (pid=%d)\n",
			ctx.tdt_pid);
		fflush(stderr);

		kill(ctx.tdt_pid, SIGUSR1);

		while (tdt_running && !tdt_timeout) 
			suspend("tdt_client: sigsuspend");

		alarm(0);

		if (tdt_timeout == 1)
			fprintf(stderr, "tdt_client: tdtproc timed out!\n");
		else {
			DEBUG(stderr, "tdt_client: (play) recieved done signal\n");
			fflush(stderr);

			check_retval();
		}

		register_io_signal_handlers();

		DEBUG(stderr, "tdt_client: sending rec signal (pid=%d)\n",
			ctx.tdt_pid);
		fflush(stderr);

		kill(ctx.tdt_pid, SIGUSR2);

		while (tdt_running && !tdt_timeout) 
			suspend("tdt_client: sigsuspend");

		alarm(0);

		if (tdt_timeout == 1)
			fprintf(stderr, "tdt_client: tdtproc timed out!\n");
		else {
			DEBUG(stderr, "tdt_client: (rec) recieved done signal\n");
			fflush(stderr);

			check_retval();
		}
	}

	display_waveforms();
}

#undef suspend
#undef DEBUG_RETVAL

/* ------------------------------------------------------------------------
   check_retval
   ------------------------------------------------------------------------ */
static void check_retval(void)
{
	int		retval;

	fscanf(ctx.tdtproc_fp, "%d", &retval);

	switch(retval) {
		case IS_TRANS_OK:
			DEBUG(stderr, "tdt_client: tdtproc returns IS_TRANS_OK");
			break;
		case IS_TRANS_MISSED:
			DEBUG(stderr, "tdt_client: tdtproc returns IS_TRANS_MISSED");
			break;
	}
}

/* ------------------------------------------------------------------------
   display_waveforms
   ------------------------------------------------------------------------ */
static void display_waveforms(void)
{
	static short *left_recbuf = NULL;
#ifdef _DICHOTIC_
	static short *right_recbuf = NULL;
#endif /* _DICHOTIC_ */

	if (ctx.use_graphics) {
		if (left_recbuf == NULL) {
			left_recbuf = (short *) calloc(ctx.nsamples/2, sizeof(short));
			assert(left_recbuf != NULL);
		} else 
			left_recbuf = memset(left_recbuf, 0, ctx.nsamples/2);

#ifdef _DICHOTIC_
		if (right_recbuf == NULL) {
			right_recbuf = (short *) calloc(ctx.nsamples/2, sizeof(short));
			assert(right_recbuf != NULL);
		} else
			right_recbuf = memset(right_recbuf, 0, ctx.nsamples/2);
#endif /* _DICHOTIC_ */

		deinterlace_recbuf(left_recbuf, LEFT);
#ifdef _DICHOTIC_
		deinterlace_recbuf(right_recbuf, RIGHT);
#endif /* _DICHOTIC_ */

#ifdef _DICHOTIC_
		if (ctx.use_et1)
			tdtplotbufferet1(&ctx.p0, left_recbuf, ctx.spikes, ctx.nsamples/2, 
				ctx.nspikes);
		else {
			tdtplotbuffer(&ctx.p0, left_recbuf, ctx.nsamples/2);
			tdtplotbuffer2(&ctx.p0, right_recbuf, ctx.nsamples/2);
		}
#endif /* _DICHOTIC_ */
#ifdef _FREE_FIELD_
		tdtplotbuffer(&ctx.p0, left_recbuf, ctx.nsamples/2);
#endif /* _FREE_FIELD_ */
	}
}

/* ------------------------------------------------------------------ */
/* deinterlace_recbuf                                                  */
/* ------------------------------------------------------------------ */

static void deinterlace_recbuf(
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

	for (i=offset; i<ctx.nsamples; i+=2, j++) 
		chanbuf[j] = ctx.rec_sound[i];
}

