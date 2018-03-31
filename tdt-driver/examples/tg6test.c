/******************************************************************
**   RCSID: $Id: tg6test.c,v 1.4 2000/07/12 22:55:05 cmalek Exp $
** RCSName: $Name:  $
** Program: tg6test
**    File: tg6test.c
**  Author: cmalek
** Descrip: 
**
**	Simultaneous play/record with both D/A channels and both A/D channels.  
**	Records from A/D inputs 1 and 2, and plays from D/A outputs 1 and 2. 
**	Plots recorded data.  This example requires either a signal generator or 
**	two as inputs for A/D inputs 1 and 2, or that D/A outputs 1 and 2 be 
**	connected to A/D inputs 1 and 2.
**
**	Same as dualplayrec, really, except that instead of using a software 
**	trigger (i.e. DD1go(1);) to start D/A - A/D conversion, we set up
**	the TG6 to send a trigger pulse to start conversion.  To get this 
**	to example to work, connect TG6 output 1 to the TRIG input of the DD1.
**
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


#define DURATION    1000				/* Playback duration in milliseconds */
#define STIMFREQ1   1000			/* Tone freq for playback 1 */
#define STIMFREQ2   2000			/* Tone freq for playback 2 */
#define SRATE    20.0				/* Sampling period in mircoseconds */

#define PLAY1       1     	/* Setup logical name for play buffer 1 */
#define PLAY2       2     	/* Setup logical name for play buffer 2 */
#define REC1      3     	/* Setup logical name for rec buffer 1 */
#define REC2      4     	/* Setup logical name for rec buffer 2 */


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
	strcpy(p0.title,"Signal From ADC1 and ADC2 of DD1");


	/* Initialize Hardware (AP2 and XBUS */
	if (!apinit(APb) || !XB1init(USE_DOS))
	{
		printf("\n\n  Error initializing hardware !!! \n\n");
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
	allot16(PLAY2,npts);
	allot16(REC1,npts);
	allot16(REC2,npts);

	gotoxy(37,3);
	out("TG6TEST\n");
	gotoxy(1,4);
	out("      ---------------------------------------------------------------\n");
	gotoxy(1,5);
	out("      This example demonstrates simultaneous playback and recording\n");
	out("      using both D/A channels and both A/D channels of a DD1 Stereo Analog\n"); 
	out("      Interface, with AP2 Array Processing card.  Conversion will be\n\n");
	out("      triggered by the TG6 Timing Generator.\n\n");


	gon();

	/* Create tones to play */
	/* --------------------- */
	dpush(npts);
	tone(STIMFREQ1, SRATE);
	scale(32000.0);
	qpop16(PLAY1);

	dpush(npts);
	tone(STIMFREQ2, SRATE);
	scale(32000.0);
	qpop16(PLAY2);

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
	DD1mode(1,ALL);
	DD1npts(1,npts);
	DD1strig(1);
	out("      ---------------------------------------------------------------\n");

	/* Program the TG6 with desired parameters */
	/* --------------------------------------- */
	TG6clear(1);
	TG6baserate(1, _1ms);
	TG6new(1, 0, 1020, 0x00); /* New timing sequence #0, 20 ms long */
	TG6high(1, 0, 0, 1000, 0x01); /* Set output #1 high for 0-10ms */
	TG6low(1, 0, 1000, 1020, 0x01); /* Set output #1 high for 0-10ms */

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
		qdup();
		qpop16(REC1);
		qpop16(REC2);


		/* dplay() and drecord() must be called before each A/D conversion */
		dplay(PLAY1, PLAY2);
		drecord(REC1, REC2);

	    DD1arm(1);

		TG6arm(1,0);

		TG6go(1);

		do {} while (TG6status(1) != 0);
		/* The following do/while loop is very important! It causes
		   the program to wait until the DD1 has finished taking data
		   before retrieving it from the AP2 DAMA buffer.  If you use
		   it in your program, use it precisely like this! */
		do {} while (DD1status(1) != 0);


	    tdtplot1(&p0, REC1);
	    tdtplot2(&p0, REC2);

	    do {
			c = getch();
	    } while (c == ERR);

	    if (c == 'x' || c == 'X') 
			break;
		i++;

	}

	TG6clear(1);

	DD1stop(1);
	DD1clear(1);


	goff();

#ifdef NCURSES
	endwin();
#endif

}
