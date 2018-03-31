/*
 * setpriority.c -- a tool to set scheduling priorities and policies
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sched.h>
#include <fcntl.h>
#include <linux/unistd.h>     /* for _syscallX macros/related stuff */

/* ------------------------------------------------
   Typedefs
   ------------------------------------------------ */
static struct ctx_struct {
	int	scheduler;
	int	priority;
	int pid;
	int	verbose;
} ctx;

/* ------------------------------------------------
   Global variables
   ------------------------------------------------ */
int bss_debugflag;

/* ------------------------------------------------
 * Static function prototypes
 * ------------------------------------------------ */
static void parse_cmdline(int, char **);
static void command_info(void);


/* ------------------------------------------------
 * Static function prototypes
 * ------------------------------------------------ */

int main(int argc, char **argv)
{
  int 	i; 
  int	minp;
  int	maxp;
  struct sched_param my_priority;

  if (geteuid() != 0) {
	  fprintf(stderr, "You must be root to use setpriority.\n");
	  exit(1);
  }

  ctx.pid = -1;
  ctx.priority = -1;
  ctx.scheduler = SCHED_FIFO;
  ctx.verbose = 0;

  parse_cmdline(argc, argv);

  minp = sched_get_priority_min(ctx.scheduler);
  if (minp < 0) {
    perror("sched_get_priority_min");
    exit(1);
  }
  maxp = sched_get_priority_max(ctx.scheduler);
  if (maxp < 0) {
    perror("sched_get_priority_max");
    exit(1);
  }

  if (ctx.verbose)
	switch (ctx.scheduler) {
		case SCHED_FIFO: 
			printf("Available SCHED_FIFO priority range: (%d ... %d)\n", 
				minp, maxp);
			break;
		case SCHED_RR: 
			printf("Available SCHED_RR priority range: (%d ... %d)\n", 
				minp, maxp);
			break;
		case SCHED_OTHER: 
			printf("Available SCHED_OTHER priority range: (%d ... %d)\n", 
				minp, maxp);
			break;
	}

  /* Make sure the priority chosen is valid, and choose priority if
	 no scheduling priority was specified on the command line */
  if (ctx.priority == -1) {
	ctx.priority = maxp;
  } else {
	  if (ctx.priority < minp)
		  ctx.priority = minp;
	  if (ctx.priority > maxp)
		  ctx.priority = maxp;
  }

  my_priority.sched_priority = ctx.priority;

  if (ctx.verbose)
	  printf("Trying to set priority of process #%d to %d.\n", 
		  ctx.pid, my_priority.sched_priority);

  if (my_priority.sched_priority < sched_get_priority_min(ctx.scheduler))
    my_priority.sched_priority++;
  if (sched_setscheduler(ctx.pid, ctx.scheduler, &my_priority)) {
     perror("sched_setscheduler");
     exit(1);
  }

  /* Lets make sure we actually did something */
  my_priority.sched_priority = 4711;

  i = sched_getparam(ctx.pid, &my_priority);
  if (i < 0)
    perror("sched_getparam");
  else
	  if (ctx.verbose)
		printf("Set priority of process #%d to %d.\n", ctx.pid, 
			my_priority.sched_priority);

  i = sched_getscheduler(ctx.pid);
  if (i < 0)
    perror("sched_getscheduler");
  else {
	  if (ctx.verbose) {
		  switch (i) {
			case SCHED_FIFO:
				printf("Scheduler for process #%d is SCHED_FIFO.\n", ctx.pid);
				break;
			case SCHED_RR:
				printf("Scheduler for process #%d is SCHED_RR.\n", ctx.pid);
				break;
			case SCHED_OTHER:
				printf("Scheduler for process #%d is SCHED_OTHER.\n", ctx.pid);
				break;
		  }
	  }
  }

  return(0);
}

/* ------------------------------------------------------------------------

	parse_cmdline

	Returns:

	Side Effects:

   ------------------------------------------------------------------------ */


static void parse_cmdline(
	int		ac,
	char	**av)
{
	char		c;

	if (ac == 1) {
		command_info();
		exit(0);
	} 

	while (1) {

		if ((c = getopt(ac, av, "fop:rv")) == -1)
			break;

		switch (c) {

			case 'f':
				ctx.scheduler = SCHED_FIFO;
				break;
			case 'o':
				ctx.scheduler = SCHED_OTHER;
				break;
			case 'p':
				ctx.priority = atoi(optarg);
				break;
			case 'r':
				ctx.scheduler = SCHED_RR;
				break;
			case 'v':
				ctx.verbose = 1; 
				break;
			
			case '?': 
				break;
		}
	}

	if (optind < ac) 
		ctx.pid = (int) atoi(av[optind]);
	else {
		fprintf(stderr, "setpriority: no pid specified\n");
		exit(1);
	}

	if (ctx.scheduler == SCHED_OTHER && ctx.priority > 0) {
		fprintf(stderr,"setpriority: only allowed priority for SCHED_OTHER is 0.\n");
		exit(1);
	}


}




/* ------------------------------------------------------------------------
    command_info:    

	Returns:

	Side Effects:

   ------------------------------------------------------------------------ */

static void command_info(void)
{
	fprintf(stderr, "usage:  setpriority <options> pid\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "   options are:\n");
	fprintf(stderr, "      -f            set scheduler to SCHED_FIFO\n");
	fprintf(stderr, "      -o            set scheduler to SCHED_OTHER\n");
	fprintf(stderr, "      -p <priority> set priority to <priority>\n");
	fprintf(stderr, "      -r <msec>     set scheduler to SCHED_RR\n");
	fprintf(stderr, "      -v            verbose mode\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "You must be root to use this program.\n");

}

