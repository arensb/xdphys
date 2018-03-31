/******************************************************************
**  RCSID: $Id: init_test.c,v 1.5 2000/03/01 23:06:55 cmalek Exp $
** Program: dowl
**  Module: init_test.c
**  Author: cmalek
** Descrip: test program for is_init
**
** Environment Variables:
**
**      TEST_DEBUG:		if defined, turn on debug messages
**
*******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <stdarg.h>
#include <curses.h>

#include "dowlio.h"
#include "svars.h"
#include "iserver.h"
#include "iserverInt.h"
#include "tdtproc.h"
#include <tdtplot.h>

extern int is_init(float, float, float, int, int, int, IDLEFN);

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

#define VERSION 					"1.0"

/* ------------------------------------------------
   Typedefs
   ------------------------------------------------ */
static struct ctx_struct {
	int duration;
	int fc;
} ctx;


/* ------------------------------------------------
   Global variables
   ------------------------------------------------ */
int 	debugflag;

int		is_inited = 0;	/* num of "sample-pairs" in is_inputBuffer */
int 	is_testmode = 0;

int 	is_daFc = -1;	/* public da conversion frequency */
int		is_adFc = -1;	/* public ad conversion frequency */
int		is_evtFc = -1;	

xword   *is_outputBuffer = NULL;
int		is_outputSize;	/* numb of "sample-pairs" in is_outputBuffer */
xword   *is_inputBuffer = NULL;
int		is_inputSize;	/* num of "sample-pairs" in is_inputBuffer */

int		ad_nchans = 2;
int		da_nchans = 2;

int 	is_report_errors = 1;

SVAR_TABLE *global_svar_table = NULL;
int 	LastVarUndefined = 0; 

/* ------------------------------------------------
 * Static variables
 * ------------------------------------------------ */
static char *rcs_revision = "$Revision: 1.5 $";
static char *rcs_name = "$Name:  $";
static char *rcs_date = "$Date: 2000/03/01 23:06:55 $";
static char *rcs_path = "$Source: /home/konishi11/cvsroot/iserver/tdtproc/init_test.c,v $";

/* ------------------------------------------------
 * Static function prototypes
 * ------------------------------------------------ */
static void initialize_ctx(void);
static void parse_env(void);
static void parse_cmdline(int, char **);
static void startup(int, char **);
static void cleanup(void);
static void command_info(void);
static void launch_tdtproc(void);

/* ------------------------------------------------
   main
   ------------------------------------------------ */
int main(
	int	argc,
	char **argv)
{
	char 	c;

	fprintf(stderr, "init_test: starting ...\n");

	startup(argc, argv);
	fprintf(stderr, "---------------------------------------------------\n");
	fprintf(stderr, "init_test: First launch...\n");
	fprintf(stderr, "---------------------------------------------------\n");
	launch_tdtproc();
	sleep(2);
	fprintf(stderr, "---------------------------------------------------\n");
	fprintf(stderr, "init_test: last test launch...\n");
	fprintf(stderr, "---------------------------------------------------\n");
	launch_tdtproc_last();
	sleep(2);
	fprintf(stderr, "---------------------------------------------------\n");
	fprintf(stderr, "init_test: diffargs test launch...\n");
	fprintf(stderr, "---------------------------------------------------\n");
	launch_tdtproc_diffargs();

	cleanup();

	fprintf(stderr, "init_test: exiting ...\n");

	return(SUCCESS);
}


/* ------------------------------------------------------------------------
	view_output: simulate linking with libdowl.a
   ------------------------------------------------------------------------ */
void view_output(void)
{
}

/* ------------------------------------------------------------------------
	view_input: simulate linking with libdowl.a
   ------------------------------------------------------------------------ */
void view_input(void)
{
}

/* ------------------------------------------------------------------------
	dloop_empty: simulate linking with libdowl.a
   ------------------------------------------------------------------------ */
void dloop_empty(void)
{
}

/* ------------------------------------------------------------------------
	single_step: simulate linking with libdowl.a
   ------------------------------------------------------------------------ */
int single_step(int ret)
{
	fprintf(stderr, "init_test: called single step with ret=%d\n", ret);
}


/* ------------------------------------------------------------------------
	getvar_int: simulate linking with libdowl.a
   ------------------------------------------------------------------------ */
int getvar_int(char *p, SVAR_TABLE *hashtab)
{
	LastVarUndefined = 1;

	return 0;
}

/* ------------------------------------------------------------------------
	idle_set: simulate linking with libdowl.a
   ------------------------------------------------------------------------ */
int rnd2int(
	float f)
{
  if (f < 0.0)
    return((int)(f - 0.5));
  else
    return((int)(f + 0.5));
}

/* ------------------------------------------------------------------------
	idle_set: simulate linking with libdowl.a
   ------------------------------------------------------------------------ */
void idle_set(
	IDLEFN idlefn) 
{
}

/* ------------------------------------------------------------------------
	forbid_detach: simulate linking with libdowl.a
   ------------------------------------------------------------------------ */
void forbid_detach(
	IDLEFN idlefn) 
{
}

/* ------------------------------------------------------------------------
	allow_detach: simulate linking with libdowl.a
   ------------------------------------------------------------------------ */
void allow_detach(void) 
{
}

/* ------------------------------------------------------------------------
	set_led: simulate linking with libdowl.a
   ------------------------------------------------------------------------ */
void set_led(
	int a,
	int b) 
{
}


/* --------------------------------------------------------------------
	alert: simulate linking with libdowl.a
   ------------------------------------------------------------------------ */
void alert(char *buf, ...)
{
	va_list pvar;
}

/* --------------------------------------------------------------------
	notify: simulate linking with libdowl.a
   ------------------------------------------------------------------------ */
void notify(char *buf, ...)
{
	va_list pvar;

	va_start(pvar, buf);

	fprintf(stderr, "notify: %s", buf);
}


/* ------------------------------------------------------------------------
	forbid_detach: simulate linking with libdowl.a
   ------------------------------------------------------------------------ */
void pop_box(
	char *a,
	char *b,
	char *c,
	char *d)
{
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

}


/* ------------------------------------------------------------------------
   cleanup:    Cleanup files and shared memory before exiting.
   ------------------------------------------------------------------------ */

static void cleanup(void)
{
	DEBUG(stderr, "init_test: cleaning up\n");

	is_shutdown();

	DEBUG(stderr, "init_test: finished cleanup\n");
}

/* ------------------------------------------------------------------------
	initialize_ctx
   ------------------------------------------------------------------------ */
static void initialize_ctx(void) 
{
	ctx.duration = 1000;
	ctx.fc = 32.0;
}

/* ------------------------------------------------------------------------
	parse_env
   ------------------------------------------------------------------------ */
static void parse_env(void) 
{
	if (getenv("TEST_DEBUG") != NULL)
		debugflag = 1;
	else
		debugflag = 0;
}

/* ------------------------------------------------------------------------
	parse_cmdline
   ------------------------------------------------------------------------ */
#define DEBUG_SWITCH(arg, label, val) DEBUG(stderr, \
	"init_test: parsed %s switch: %s=%d\n", arg, label, val)

static void parse_cmdline(
	int		ac,
	char	**av)
{
	char		c;
	static struct option longopts[] = {
		{"duration", 1, 0, 'd'},
		{"fc", 1, 0, 'F'},
		{"version", 1, 0, 'v'},
		{"help", 1, 0, 'h'},
		{0, 0, 0, 0}};
	int options_index = 0;

	while (1) {

		if ((c = getopt_long_only(ac, av, "d:F:vh",
			longopts, &options_index)) == -1)
			break;

		switch (c) {
			case 'd':
				ctx.duration = atoi(optarg);
				DEBUG_SWITCH("-duration", "duration", ctx.duration);
				break;
			case 'F':
				ctx.fc = atoi(optarg);
				DEBUG_SWITCH("-fc", "fc", ctx.fc);
				break;
			case 'v':
				printf("\ninit_test Version %s\n", VERSION);
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
				cleanup();
				command_info();
				exit(SUCCESS);
				break;
		}
	}
}
#undef DEBUG_SWITCH



/* ------------------------------------------------------------------------
    command_info:    
   ------------------------------------------------------------------------ */
static void command_info(void)
{
	fprintf(stderr, "usage:  init_test <options>\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "   options are:\n");
	fprintf(stderr, "      -d <msec>     tone duration\n");
	fprintf(stderr, "      -f <Hz>       sampling freq\n");
	fprintf(stderr, "      -v            version\n");

}

/* ------------------------------------------------------------------------
   launch_tdtproc
   ------------------------------------------------------------------------ */
static void launch_tdtproc(void)
{
	int status;

	status = is_init((float) ctx.fc, (float) ctx.fc, 0.0, ctx.duration, 
		ctx.duration, 0, NULL);

	tdtproc_retval(status);
}

/* ------------------------------------------------------------------------
   relaunch_tdtproc
   ------------------------------------------------------------------------ */
static void launch_tdtproc_last(void)
{
	int status;

	status = is_init((float) IS_LAST, (float) IS_LAST, 0.0, ctx.duration, 
		ctx.duration, 0, NULL);

	tdtproc_retval(status);

}

/* ------------------------------------------------------------------------
   launch_tdtproc_
   ------------------------------------------------------------------------ */
static void launch_tdtproc_diffargs(void)
{
	int status;

	status = is_init((float) 48.0, (float) 48.0, 0.0, ctx.duration, 
		ctx.duration, 0, NULL);

	tdtproc_retval(status);

}

/* ------------------------------------------------------------------------
   tdtproc_retval
   ------------------------------------------------------------------------ */
static void tdtproc_retval(
	int status)
{
	switch (status) {
		case IS_OK:
			fprintf(stderr, "init_test: is_init returns IS_OK\n");
			break;
		case IS_ERROR:
			fprintf(stderr, "init_test: is_init returns IS_ERROR\n");
			break;
		default:
			fprintf(stderr, "init_test: is_init returns %d\n", status);
			break;
	}
}
