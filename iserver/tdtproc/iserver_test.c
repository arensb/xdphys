/******************************************************************
**  RCSID: $Id: iserver_test.c,v 1.4 2001/02/27 05:56:43 cmalek Exp $
** Program: xdphys
**  Module: iserver_test.c
**  Author: cmalek
** Descrip: test program for the iserver library
**
** Environment Variables:
**
**      ISERVERTEST_DEBUG:		if defined, turn on debug messages
**
*******************************************************************/

#define _IS_STANDALONE_
#define _USE_TIMESTAMP_

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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <values.h>
  
#ifdef __GLIBC__
#include <curses.h>
#else
#include <ncurses/curses.h>
#endif
#include <tdtplot.h>

#include "att.h"
#include "iserver.h"
#include "iserverInt.h"

/* ------------------------------------------------
   Macros
   ------------------------------------------------ */
#define SUCCESS 0
#define FAIL -1
#define DEBUG	if (debugflag) fprintf
#define SHMEM_PERMS		0666

#define LEFT		0
#define RIGHT		1

#define AD_LEFT_FILENAME "ileft_ad.out"
#define DA_LEFT_FILENAME "ileft_da.out"
#define AD_RIGHT_FILENAME "iright_ad.out"
#define DA_RIGHT_FILENAME "iright_da.out"

#ifndef COMPILE_DATE
#define COMPILE_DATE			" "
#endif /*COMPILE_DATE*/
#ifndef OSINFO
#define OSINFO				" "
#endif /*OSINFO*/

#define VERSION 					"1.2"
#define USER_STEP
#define _DICHOTIC_

/* ------------------------------------------------
   Typedefs
   ------------------------------------------------ */
static struct ctx_struct {
	short 	*play_sound;				
	short 	*rec_sound;				
#ifndef _FREE_FIELD_
	unsigned long 	*spikes;				
#endif /* _FREE_FIELD_ */

#ifdef _DICHOTIC_
	int		use_et1;
#endif /* _DICHOTIC_ */
	int 	nsamples;		

	int		latten;
#ifdef _DICHOTIC_
	int		ratten;
#endif /* _DICHOTIC_ */
	int		use_att;

#ifdef _DICHOTIC_
	int 	nspikes;		
#endif /* _DICHOTIC_ */
	int 	fc;					/* sampling frequency in Hz */
	int		duration;			/* tone duration in msec */
	int		freq; 				/* tone freq in Hz */

	int 	rise;
	int		fall;

	plotinfo p0;
	int		use_graphics;
	int		use_strace;
	int		split;
	int		channel;

	int 	inited;
	int 	dump;
} ctx;


/* ------------------------------------------------
   Global variables
   ------------------------------------------------ */
int debugflag;

/* ------------------------------------------------
 * Static variables
 * ------------------------------------------------ */
static char *rcs_revision = "$Revision: 1.4 $";
static char *rcs_name = "$Name:  $";
static char *rcs_date = "$Date: 2001/02/27 05:56:43 $";
static char *rcs_path = "$Source: /home/konishi11/cvsroot/iserver/tdtproc/iserver_test.c,v $";

/* ------------------------------------------------
 * Static function prototypes
 * ------------------------------------------------ */
static void initialize_ctx(void);
static void parse_env(void);
static void parse_cmdline(int, char **);
static void setup_graphics(void);
static void cleanup_graphics(void);
static int init_daq(void);
static void startup(int, char **);
static void cleanup(void);
static void command_info(void);
static void synth_sound(void);
static void perform_trial(void);
static void deinterlace_playbuf(short *, int);
static void display_waveforms(void);
static void dump_dabufs_to_file(void);

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

	fprintf(stderr,"Instructions:\n");
	fprintf(stderr,"  Hit 'l' to play only the left side.\n");
	fprintf(stderr,"  Hit 'r' to play only the right side.\n");
	fprintf(stderr,"  Hit 'b' to play both sides.\n\n");
	fprintf(stderr,"  Hit any key to play the tone.:\n\n");
	fprintf(stderr,"  Hit 'x' to exit.\n");
	fflush(stderr);
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
			ctx.channel = X_LEFT;
			continue;
		}

		if (c == 'r' || c == 'R') {
			fprintf(stderr, "tdt_client: playing only right channel\n");
			fflush(stderr);
			ctx.channel = X_RIGHT;
			continue;
		}

		if (c == 'b' || c == 'B') {
			fprintf(stderr, "tdt_client: playing both channels\n");
			fflush(stderr);
			ctx.channel = X_LEFT | X_RIGHT;
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


	fprintf(stderr, "iserver_test: cleaning up ...\n");

	cleanup();

	fprintf(stderr, "iserver_test: exiting ...\n");

	return(SUCCESS);
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

	setup_graphics();
	if (init_daq() == FAIL) 
		cleanup();
	synth_sound();

}


/* ------------------------------------------------------------------------
   cleanup:    Cleanup files and shared memory before exiting.
   ------------------------------------------------------------------------ */

static void cleanup(void)
{
	DEBUG(stderr, "iserver_test: cleaning up\n");

	endwin();

	cleanup_graphics();

	if (ctx.inited) {
		is_shutdown();
		ctx.inited = 0;
	}

	DEBUG(stderr, "iserver_test: finished cleanup\n");
}

/* ------------------------------------------------------------------------
	initialize_ctx
   ------------------------------------------------------------------------ */
static void initialize_ctx(void) 
{
	ctx.play_sound = NULL;
	ctx.rec_sound = NULL;
#ifdef _DICHOTIC_
	ctx.spikes	= NULL;
	ctx.use_et1 = 0;
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
	ctx.use_strace = 0;
	ctx.inited = 0;
	ctx.split = 0;
	ctx.channel = IS_BOTH;
	ctx.dump = 0;
}

/* ------------------------------------------------------------------------
	parse_env
   ------------------------------------------------------------------------ */
static void parse_env(void) 
{
	if (getenv("ISERVERTEST_DEBUG") != NULL)
		debugflag = 1;
	else
		debugflag = 0;

	if (getenv("ISERVERTEST_STRACE") != NULL)
		ctx.use_strace = 1;
}

/* ------------------------------------------------------------------------
	parse_cmdline
   ------------------------------------------------------------------------ */
#define DEBUG_SWITCH(arg, label, val) DEBUG(stderr, \
	"iserver_test: parsed %s switch: %s=%d\n", arg, label, val)

static void parse_cmdline(
	int		ac,
	char	**av)
{
	char		c;
	static struct option longopts[] = {
		{"att", 0, 0, 'a'},
		{"duration", 1, 0, 'd'},
		{"spikes", 0, 0, 'e'},
		{"tonefreq", 1, 0, 'f'},
		{"fc", 1, 0, 'F'},
		{"graph", 0, 0, 'g'},
		{"latten", 1, 0, 'l'},
		{"split", 0, 0, 'p'},
		{"ratten", 1, 0, 'r'},
		{"nspikes", 1, 0, 's'},
		{"strace", 1, 0, 'S'},
		{"version", 1, 0, 'v'},
		{"help", 1, 0, 'h'},
		{"dump", 0, 0, 'D'},
		{0, 0, 0, 0}};
	int options_index = 0;

	while (1) {

		if ((c = getopt_long_only(ac, av, "ad:DeF:f:gl:pr:s:Svh?",
			longopts, &options_index)) == -1)
			break;

		switch (c) {
			case 'a':
				ctx.use_att = 1;
				DEBUG_SWITCH("-a", "use_att", ctx.use_att);
				break;
			case 'd':
				ctx.duration = atoi(optarg);
				DEBUG_SWITCH("-d", "duration", ctx.duration);
				break;
			case 'D':
				ctx.dump = 1;
				DEBUG_SWITCH("-D", "dump", ctx.dump);
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
				ctx.use_att = 1;
				ctx.latten = atoi(optarg);
				DEBUG_SWITCH("-l", "latten", ctx.latten);
				break;
			case 'p':
				ctx.split = 1;
				DEBUG_SWITCH("-p", "split", ctx.split);
				break;
			case 'r':
#ifdef _DICHOTIC_
				ctx.use_att = 1;
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
			case 'v':
#ifdef _DICHOTIC_
				printf("\niserver_test Version %s (%s rig)\n", VERSION,
					"DICHOTIC");
#endif /* _DICHOTIC_ */
#ifdef _FREE_FIELD_
				printf("\niserver_test Version %s (%s rig)\n", VERSION,
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
			strcpy(ctx.p0.title,"Output Signals to DD1 and spikes from ET1");
		else 
			strcpy(ctx.p0.title,"Output Signals to DD1");
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
	init_daq
   ------------------------------------------------------------------------ */
static int init_daq(void)
{
	float split_ops;

	if (ctx.split)
		split_ops = -1.0;
	else
		split_ops = 0.0;

	if (is_init((float) ctx.fc, (float) ctx.fc, split_ops, ctx.duration, 
		ctx.duration) != IS_OK) {
		return(FAIL);
	}

	ctx.inited = 1;

	fprintf(stderr, "iserver_test: Actual daFc=%d\n", is_daFc);
	fprintf(stderr, "iserver_test: Actual adFc=%d\n", is_adFc);
	fflush(stderr);

	ctx.play_sound = is_getDAbuffer(&ctx.nsamples);
	ctx.rec_sound = is_getADbuffer(NULL);

	return(SUCCESS);

}


/* ------------------------------------------------------------------------
	synth_sound
   ------------------------------------------------------------------------ */
static void synth_sound(void)
{
	int			i, j, envsamples;
	double		slope, ratio;

	ratio = ((double) ctx.freq) / ((double) (ctx.fc));

	fprintf(stderr,"ctx.nsamples=%d\n",ctx.nsamples);
	fflush(stderr);
	for (i=0,j=0; i<ctx.nsamples; i++,j+=2) {
		ctx.play_sound[j] = (short) ((double) (MAXSHORT/2) * (sin(2.0*M_PI*
			ratio * (double) i)));
#ifdef _DICHOTIC_
		ctx.play_sound[j+1] = ctx.play_sound[j];
#endif /* _DICHOTIC_ */
	}

#if(0)
	envsamples = (int) (((double) ctx.rise)/1000.0 * (double) ctx.fc);
	slope = 1.0 / ((double) envsamples);

	for (i=0, j=0; i<envsamples; i++,j+=2) {
		ctx.play_sound[j] = (short) (((double) ctx.play_sound[j]) * 
			slope*((double) i));
#ifdef _DICHOTIC_
		ctx.play_sound[j+1] = (short) (((double) ctx.play_sound[j+1]) * 
			slope*((double) i));
#endif /* _DICHOTIC_ */
	}

	envsamples = (int) (((double) ctx.fall)/1000.0 * (double) ctx.fc);
	slope = 1.0 / ((double) envsamples);

	for (i=ctx.nsamples-2, j=(2*ctx.nsamples-4); i>(ctx.nsamples - envsamples); i--,j-=2) {
		ctx.play_sound[j] = (short) (((double) ctx.play_sound[j]) * 
			slope*((double) i));
#ifdef _DICHOTIC_
		ctx.play_sound[j+1] = (short) (((double) ctx.play_sound[j+1]) * 
			1.0-slope*((double) i));
#endif /* _DICHOTIC_ */
	}
#endif /* 0 */

}



/* ------------------------------------------------------------------------
    command_info:    
   ------------------------------------------------------------------------ */

static void command_info(void)
{
	fprintf(stderr, "usage:  iserver_test <options>\n");
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
#ifdef _DICHOTIC_
	fprintf(stderr, "      -s <spikes>   # spikes in spike shm buffer\n");
#endif /* _DICHOTIC_ */
	fprintf(stderr, "      -S            use strace to run tdtproc\n");
	fprintf(stderr, "      -v            version\n");

}

/* ------------------------------------------------------------------------
   perform_trial
   ------------------------------------------------------------------------ */
static void perform_trial(void)
{
	double 	timestamp;

	if (ctx.use_att) {
#ifdef _FREE_FIELD_
		setRack(0, ctx.latten, 0.0);
#endif /* _FREE_FIELD_ */
#ifdef _DICHOTIC_
		setRack(0, ctx.latten, ctx.ratten);
#endif /* _DICHOTIC_ */
	}

	if (ctx.dump)
		dump_dabufs_to_file(); 

	if (ctx.split) {
		is_load(0);
		is_sample(ctx.channel, ctx.duration, &timestamp);
		is_record();
	} else {
		is_sample(ctx.channel, ctx.duration, &timestamp);
	}

	display_waveforms(); 
}

/* ------------------------------------------------------------------------
   display_waveforms
   ------------------------------------------------------------------------ */
static void display_waveforms(void)
{
	static short *left_playbuf = NULL;
#ifdef _DICHOTIC_
	static short *right_playbuf = NULL;
#endif /* _DICHOTIC_ */

	if (ctx.use_graphics) {
		if (left_playbuf == NULL) {
			left_playbuf = (short *) calloc(ctx.nsamples, sizeof(short));
			assert(left_playbuf != NULL);
		} else 
			left_playbuf = memset(left_playbuf, 0, ctx.nsamples);

#ifdef _DICHOTIC_
		if (right_playbuf == NULL) {
			right_playbuf = (short *) calloc(ctx.nsamples, sizeof(short));
			assert(right_playbuf != NULL);
		} else
			right_playbuf = memset(right_playbuf, 0, ctx.nsamples);
#endif /* _DICHOTIC_ */

		deinterlace_playbuf(left_playbuf, LEFT);
#ifdef _DICHOTIC_
		deinterlace_playbuf(right_playbuf, RIGHT);
#endif /* _DICHOTIC_ */

#ifdef _DICHOTIC_
		if (ctx.use_et1)
			tdtplotbufferet1(&ctx.p0, left_playbuf, ctx.spikes, ctx.nsamples, 
				ctx.nspikes);
		else {
			tdtplotbuffer(&ctx.p0, left_playbuf, ctx.nsamples);
			tdtplotbuffer2(&ctx.p0, right_playbuf, ctx.nsamples);
		}
#endif /* _DICHOTIC_ */
#ifdef _FREE_FIELD_
		tdtplotbuffer(&ctx.p0, left_playbuf, ctx.nsamples);
#endif /* _FREE_FIELD_ */
	}
}

/* ------------------------------------------------------------------ */
/* deinterlace_playbuf                                                  */
/* ------------------------------------------------------------------ */

static void deinterlace_playbuf(
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

	for (i=0,j=offset; i<ctx.nsamples; j+=2, i++) 
		chanbuf[i] = ctx.play_sound[j];
}

/* ------------------------------------------------------------------ */
/* dump_dabufs_to_file                                                  */
/* ------------------------------------------------------------------ */

static void dump_dabufs_to_file(void)
{
	int fd;
	static short *left_playbuf = NULL;
#ifdef _DICHOTIC_
	static short *right_playbuf = NULL;
#endif /* _DICHOTIC_ */

	if (left_playbuf == NULL) {
		left_playbuf = (short *) calloc(ctx.nsamples, sizeof(short));
		assert(left_playbuf != NULL);
	} else 
		left_playbuf = memset(left_playbuf, 0, ctx.nsamples);


	deinterlace_playbuf(left_playbuf, LEFT);

	if ((fd = creat(DA_LEFT_FILENAME,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH))==-1) {
		perror("opening left outfile");
		exit(1);
	} else {
		if (write(fd,left_playbuf,ctx.nsamples*sizeof(short))!=(ctx.nsamples*sizeof(short))) {
			perror("writing to left outfile");
			exit(1);
		}
		if (close(fd) == -1) {
			perror("closing left outfile");
			exit(1);
		}
	}

#ifdef _DICHOTIC_
	if (right_playbuf == NULL) {
		right_playbuf = (short *) calloc(ctx.nsamples, sizeof(short));
		assert(right_playbuf != NULL);
	} else
		right_playbuf = memset(right_playbuf, 0, ctx.nsamples);

	deinterlace_playbuf(right_playbuf, RIGHT);

	if ((fd = creat(DA_RIGHT_FILENAME,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH))==-1) {
		perror("opening right outfile");
		exit(1);
	} else {
		if (write(fd,right_playbuf,ctx.nsamples*sizeof(short))!=(ctx.nsamples*sizeof(short))) {
			perror("writing to right outfile");
			exit(1);
		}
		if (close(fd) == -1) {
			perror("closing right outfile");
			exit(1);
		}
	}
#endif /* _DICHOTIC_ */
}
