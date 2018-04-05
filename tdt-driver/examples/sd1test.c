/******************************************************************
**   RCSID: $Id: sd1test.c,v 1.4 2000/07/12 22:55:04 cmalek Exp $
** RCSName: $Name:  $
** Program: sd1test
**    File: sd1test.c
**  Author: cmalek
** Descrip: 
**
**	Simultaneous play/record with both D/A channels and both A/D channels.  
**	Records from A/D inputs 1 and 2, and plays from D/A outputs 1 and 2. 
**	Plots recorded data.  
**
**	For this example program, connect your setup as follows:
**
**			DD1 D/A output 1 -> IN of PA4 #1
**			DD1 D/A output 2 -> IN of PA4 #2
**			OUT of PA4 #1 -> DD1 A/D input 1
**			OUT of PA4 #2 -> DD1 A/D input 2
**			TG6 output 1 -> TRIG input of DD1
**
**	This program does 10 play/record cycles, at each step increasing 
**	the attenuation by 9.9 dB, starting from 0 dB.  You should see the 
**	recorded data decrease in amplitude with each cycle.
**
*******************************************************************/
#undef USE_GRAPHS
#define CONTINUOUS

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sched.h>
#include <unistd.h>
/*#include <asm/system.h>*/
#include "xbdrv.h"
#include "apos.h"
#ifdef USE_GRAPHS
#include "tdtplot.h"
#endif /*USE_GRAPHS */



#ifdef NCURSES
#include <curses.h>
#define clrscr()	clear()
#define	gotoxy(x,y)	move((y)-1,(x)-1)
#define kbhit()	        (getch() != ERR)
#define out(a)		addstr(a),refresh()
#endif


#define DURATION    100				/* Playback duration in milliseconds */
#define SRATE    20.0				/* Sampling period in mircoseconds */

#define REC1      1     	/* Setup logical name for rec buffer */


/* ------------------------------------------------
 * Static function prototypes
 * ------------------------------------------------ */
static void set_sched_priority(int, int);
static void lock_memory(void);

int main()
{
	long times[1000];
	int  ntimes;
	long  npts;
	char  c;
	int   j, i = 0;
	char  s[80];
	unsigned long flags;
#ifdef USE_GRAPHS
	plotinfo p0;
#endif /* USE_GRAPHS */

#ifdef USE_GRAPHS
	initplotinfo(&p0);              /* plotting parameters for signal  */

	p0.ymin=-32767.0;   p0.ymax=32768.0;
	p0.xmin=0.0;     p0.xmax=100.0;
	p0.samp_period = SRATE;
	p0.skip = 1;
	strcpy(p0.lab_x,"Milliseconds");
	strcpy(p0.lab_y," ");
	strcpy(p0.title,"Signal From ADC1 of DD1 and spikes from ET1");
#endif /* USE_GRAPHS */


	/* First real-time preparation: lock all pages into RAM */
	lock_memory();

	for (j=0; j<1000; j++) 
		times[j] = 0;

	/* Initialize Hardware (AP2 and XBUS */
	if (!apinit(APa) || !XB1init(USE_DOS))
	{
		printf("\n\n  Error initializing hardware !!! \n\n");
		exit(0);
	}

	initscr();
	cbreak();
	noecho();
	/* nodelay(stdscr, TRUE); */

	clrscr();


	/* Calculate number of points needed for  signal.  */
	/* Note, the factor of 1000.0 is because DURATION is in */
	/* milliseconds while SRATE is in microseconds.        */
	npts = (int)(1000.0 * DURATION / SRATE);

	/* Probably don't have to do this, but ... */
	trash();
	dropall();

	/* Allocate a DAMA buffers to hold rec data */
	allot16(REC1,npts);

	gotoxy(37,3);
	out("SD1TEST\n");
	gotoxy(1,4);
	out("      ---------------------------------------------------------------\n");
	gotoxy(1,5);
	out("      This example demonstrates spike discrimination with the SD1 Spike\n");
	out("      Discriminator and ET1 Event Timer.  If a 1 KHz, +/-5V sine wave is \n"); 
	out("      is used as input to the SD1, with appropriate settings for \n");
	out("      Hoop #2, a spike is be detected for each peak of the sine wave\n");
	out("      ADC1 of DD1 records the waveform, and ADC1 output is plotted vs. \n");
	out("      with detected spikes.  A TG6 Timing Generator is used to start\n");
	out("      the A/D conversion and spike discrimination. \n\n");


#ifdef USE_GRAPHS
	gon();
#endif /* USE_GRAPHS */

	/* Program the DD1 with desired parameters */
	/* --------------------------------------- */
	DD1clear(1);
	/* DD1speriod() returns the sampling period the 
	   DD1 is actually using; DD1srate() doesn't */
	sprintf(s, "      Sampling period is %.1f us.\n", DD1speriod(1,SRATE));
	out(s);
	sprintf(s, "      Recording %d ms (%ld samples) each iteration.\n", 
		DURATION, npts);
	out(s);
	out("      ---------------------------------------------------------------\n");
	out("\n");
	out("\n");
	DD1mode(1,ADC1);
	DD1npts(1,npts);
	DD1strig(1);
	out("      ---------------------------------------------------------------\n");

	/* Program the TG6 with desired parameters */
	/* --------------------------------------- */
	TG6clear(1);
	TG6baserate(1, _1ms);
	TG6new(1, 0, 200, 0x00); /* New timing sequence #0, 100 ms long */
	TG6high(1, 0, 0, 100, 0x03); /* Set outputs #1 and #3 high for 0-100ms */
	TG6low(1, 0, 100, 200, 0x03); /* Set outputs #1 and #3 low for 100-200ms */

	/* Program the ET1 with desired parameters */
	/* --------------------------------------- */
	ET1clear(1);

	/* We don't have to do anything for the SD1 */

	gotoxy(18,22);
	out("Press any key to look for spikes, [X] to quit...\n");

	for (;;) {
		gotoxy(23,19);
		sprintf(s, "==>  Iteration: %d  <==",i );
		out(s);

		/* You don't have to do the following 3 lines; I did it just to 
		   clear the WAVE1 DAMA buffer before each A/D conversion, to
		   see if I was getting a full <DURATION> ms of data */
		/*
		dpush(npts);
		value(0.0);
		qdup();
		qpop16(REC1);
		*/


		/* record() must be called before each A/D conversion */
		record(REC1);

	    DD1arm(1);

		ET1drop(1);          /* Clear ET1 FIFO */
		ET1go(1);            /* Enable ET1 */

		TG6arm(1,0);

		TG6go(1);


		do {} while (TG6status(1) != 0);
		/* The following do/while loop is very important! It causes
		   the program to wait until the DD1 has finished taking data
		   before retrieving it from the AP2 DAMA buffer.  If you use
		   it in your program, use it precisely like this! */
		do {} while (DD1status(1) != 0);

		ET1stop(1);

		/* We have to get the spikes from the ET1 one at a time */

		do {} while (ET1active(1) != 0);


		gotoxy(25,20);
		out("==>  Nspikes:    <==");

#ifdef _POSIX_PRIORITY_SCHEDULING
		set_sched_priority(SCHED_FIFO, 98);
#endif /* _POSIX_PRIORITY_SCHEDULING */

		ntimes = ET1report(1);

		if (ntimes > 0) {
			if (ntimes > 1000) ntimes = 1000;
			for (j=0; j<ntimes; j++) {
				times[j] = ET1read32(1);
				if (times[j] == -1) {
					ntimes = j;
					break;
				}
			}
		}

#ifdef _POSIX_PRIORITY_SCHEDULING
		set_sched_priority(SCHED_OTHER, 0);
#endif /* _POSIX_PRIORITY_SCHEDULING */

		gotoxy(25,20);
		sprintf(s, "==>  Nspikes: %d <==", ntimes);
		out(s);

#ifdef USE_GRAPHS
	    tdtplotet1(&p0, REC1, times, ntimes);
#endif /* USE_GRAPHS */

#ifndef CONTINUOUS
	    do {
			c = getch();
	    } while (c == ERR);
#else
		sleep(1);

		c=getch();
#endif /* CONTINUOUS */


	    if (c == 'x' || c == 'X') 
			break;
		i++;
	}

	ET1clear(1);
	TG6clear(1);

	DD1stop(1);
	DD1clear(1);


#ifdef USE_GRAPHS
	goff();
#endif /* USE_GRAPHS */

#ifdef NCURSES
	endwin();
#endif

	return(0);

}

/* ------------------------------------------------------------------------

	lock_memory

	  First real-time preparation: lock all pages into RAM.

	Returns:
		
		Nothing.

	Side Effects:

   ------------------------------------------------------------------------ */


static void lock_memory(void)
{
#ifdef _POSIX_MEMLOCK
	if (mlockall(MCL_CURRENT | MCL_FUTURE)) {
		perror("mlockall(MCL_CURRENT | MCL_FUTURE)");
	}
#else
	printf("Warning: memory locking not available, get a new kernel!\n");
#endif
}

/* ------------------------------------------------------------------------

	set_sched_priority

		Set the process scheduling priority and policy.  We will use this
		to make Linux switch between normal scheduling and real-time 
		scheduling.

	Returns:
		
		Nothing.

	Side Effects:

   ------------------------------------------------------------------------ */


static void set_sched_priority(
	int		policy,
	int		priority)
{
	int	  minp,maxp;
	int   retval;
	char  s[80];
#ifdef _POSIX_PRIORITY_SCHEDULING
	struct sched_param my_priority;
#endif

#ifdef _POSIX_PRIORITY_SCHEDULING
	minp = sched_get_priority_min(policy);
	if (minp < 0) {
		perror("sched_get_priority_min");
		exit(1);
	}
	maxp = sched_get_priority_max(policy);
	if (maxp < 0) {
		perror("sched_get_priority_max");
		exit(1);
	}

	/*
	switch (policy) {
		case SCHED_FIFO: 
			printf("      SCHED_FIFO priority range: (%d ... %d)\n", 
				minp, maxp);
			break;
		case SCHED_RR: 
			printf("      SCHED_RR priority range: (%d ... %d)\n", 
				minp, maxp);
			break;
		case SCHED_OTHER: 
			printf("       SCHED_OTHER priority range: (%d ... %d)\n", 
				minp, maxp);
			break;
	}
	out(s);
	*/

	if (priority < minp)
		priority = minp;
	if (priority > maxp)
		priority = maxp-1;

	my_priority.sched_priority = priority;

	/*
	printf("Trying to set priority to %d.\n", my_priority.sched_priority);
	*/

	if (my_priority.sched_priority < sched_get_priority_min(policy))
		my_priority.sched_priority++;
	if (sched_setscheduler(getpid(), policy, &my_priority)) {
		perror("sched_setscheduler");
		exit(1);
	}

	/* Let's check the priority and scheduling policy 
	   to see if we actually changed them */
	my_priority.sched_priority = 4711;

	retval = sched_getparam(0, &my_priority);

	if (retval < 0)
		perror("sched_getparam(0, ...)");
	else {
		gotoxy(1,16);
		sprintf(s, "      Set priority of this process to %d.\n", 
			my_priority.sched_priority);
		out(s);
	}

	retval = sched_getscheduler(0);
	if (retval < 0)
		perror("sched_getscheduler");
	else {
		gotoxy(1,17);
		switch (retval) {
			case SCHED_FIFO:
				sprintf(s, "      Scheduler for this process is SCHED_FIFO.\n");
				break;
			case SCHED_RR:
				sprintf(s, "      Scheduler for this process is SCHED_RR.\n");
				break;
			case SCHED_OTHER:
				sprintf(s, "      Scheduler for this process is SCHED_OTHER.\n");
				break;
		}
		out(s);
	}

#else
	gotoxy(1,16);
	out("      WARNING: real-time scheduling not available, get a new kernel!\n");
#endif
}
