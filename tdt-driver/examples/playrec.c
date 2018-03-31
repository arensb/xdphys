/******************************************************************
**   RCSID: $Id: playrec.c,v 1.3 2000/07/12 22:55:04 cmalek Exp $
** RCSName: $Name:  $
** Program: playrec
**    File: playrec.c
**  Author: cmalek
** Descrip:
**
**	Simultaneous play/record with one D/A channel and one A/D channel.
**	Records from A/D input 1, and plays from D/A output 1.  Plots
**	recorded data.  This example requires either a signal generator as input 
**	for A/D input 1, or that D/A output 1 be connected to A/D input 1.
**
*******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "xbdrv.h"
#include "apos.h"
#include "tdtplot.h"
#include <unistd.h>


#ifdef NCURSES
#include <curses.h>
#define clrscr()	clear()
#define	gotoxy(x,y)	move((y)-1,(x)-1)
#define kbhit()	        (getch() != ERR)
#define out(a)		addstr(a),refresh()
#endif


#define DURATION    100				/* Playback duration in milliseconds */
#define STIMFREQ   1000				/* Playback duration in milliseconds */
#define SRATE    20.0				/* Sampling period in mircoseconds */

#define PLAY1       1     	/* Setup logical name for play buffer */
#define REC1      2     	/* Setup logical name for rec buffer */


void main()
{
	long  npts;
	char  c;
	int   i = 0;
	char  s[80];
	plotinfo p0;

	initplotinfo(&p0);              /* plotting parameters for signal  */

	p0.ymin=-32767.0;   p0.ymax=32768.0;
	p0.xmin=0.0;     p0.xmax=(float)DURATION;
	p0.samp_period = SRATE;
	p0.skip = 4;
	strcpy(p0.lab_x,"Milliseconds");
	strcpy(p0.lab_y," ");
	strcpy(p0.title,"Signal From ADC1 and DAC2 of DD1");


	/* Initialize Hardware (AP2 and XBUS */
	if (!apinit(APa))
	{
		printf("\n\n  Error initializing AP2 !!! \n\n");
		exit(0);
	}
	if  (!XB1init(USE_DOS))
	{
		printf("\n\n  Error initializing XBUS !!! \n\n");
		exit(0);
	}

#ifdef NCURSES
	initscr();
	cbreak();
	noecho();
	/*nodelay(stdscr, TRUE);*/
#endif

	clrscr();


	/* Calculate number of points needed for  signal.  */
	/* Note, the factor of 1000.0 is because DURATION is in */
	/* milliseconds while SRATE is in microseconds.        */
	npts = (int)(1000.0 * DURATION / SRATE);

	/* Probably don't have to do this, but ... */
	trash();
	dropall();

	/* Allocate a DAMA buffers to hold rec data */
	allot16(PLAY1,npts);
	allot16(REC1,npts);

	gotoxy(37,3);
	out("PLAYREC\n");
	gotoxy(1,4);
	out("      ---------------------------------------------------------------\n");
	gotoxy(1,5);
	out("      This example demonstrates simultaneous playback and recording\n");
	out("      using one D/A channel and one A/D channel of a DD1 Stereo Analog\n"); 
	out("      Interface, with AP2 Array Processing card. \n\n");


	gon();

	/* Create a tone to play */
	/* --------------------- */
	dpush(npts);
	tone(STIMFREQ, SRATE);
	scale(32000.0);
	qpop16(PLAY1);

	DD1clear(1);
	/* Program the DD1 with desired parameters */
	/* --------------------------------------- */
	DD1clear(1);
	/* DD1speriod() returns the sampling period the 
	   DD1 is actually using; DD1srate() doesn't */
	sprintf(s, "      Sampling period is %f us.\n", DD1speriod(1,SRATE));
	out(s);
	sprintf(s, "      Playing and recording %d ms (%ld samples) each keypress.\n", 
		DURATION, npts);
	out(s);
	DD1mode(1,DAC1+ADC1);
	DD1npts(1,npts);
	DD1strig(1);
	out("      ---------------------------------------------------------------\n");

	gotoxy(22,16);
	out("Press any key to play/rec, [X] to quit...\n");

	for (;;) {
		gotoxy(28,13);
		sprintf(s, "==>  Iteration: %d  <==",i );
		out(s);

		/* You don't have to do the following 3 lines; I did it just to 
		   clear the WAVE1 DAMA buffer before each A/D conversion, to
		   see if I was getting a full <DURATION> ms of data */
		dpush(npts);
		value(0.0);
		qpop16(REC1);


		/* play() and record() must be called before each A/D conversion */
		play(PLAY1);
		record(REC1);

	    DD1arm(1);

	    DD1go(1);

		/* The following do/while loop is very important! It causes
		   the program to wait until the DD1 has finished taking data
		   before retrieving it from the AP2 DAMA buffer.  If you use
		   it in your program, use it precisely like this! */
			do {} while (DD1status(1) != 0);


	    tdtplot1(&p0, PLAY1);
	    tdtplot2(&p0, REC1);

	    do {
			c = getch();
	    } while (c == ERR);

	    if (c == 'x' || c == 'X') 
			break;
		i++;

		sleep(1);
	}

	DD1stop(1);
	DD1clear(1);

	goff();

#ifdef NCURSES
	endwin();
#endif

}
