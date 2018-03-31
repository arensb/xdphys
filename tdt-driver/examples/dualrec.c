/******************************************************************
**   RCSID: $Id: dualrec.c,v 1.3 1997/09/08 23:00:29 cmalek Exp $
** RCSName: $Name:  $
** Program: dualrec
**    File: dualrec.c
**  Author: cmalek
** Descrip: 
**
**	Two channel recording with the DD1.  Records data from A/D inputs 1
**	and 2 simultaneously, and plots the recorded data.  This example 
**	requires one or two signal generators, as inputs for A/D inputs 1 and 2.
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
#include <curses.h>

#define clrscr()	clear()
#define	gotoxy(x,y)	move((y)-1,(x)-1)
#define kbhit()	        (getch() != ERR)
#define out(a)		addstr(a),refresh()


#define STIMDUR    100				/* Record duration in milliseconds */
#define SRATE    20.0				/* Sampling period in mircoseconds */

#define WAVE1       1     	/* Setup logical name for rec buffer 1 */
#define WAVE2       2     	/* Setup logical name for rec buffer 2 */


void main()
{
	long  npts;
	char  c;
	int   i = 0;
	char  s[80];
	plotinfo p0;

	initplotinfo(&p0);              /* plotting parameters for signal  */

	p0.ymin=-32767.0;   p0.ymax=32768.0;
	p0.xmin=0.0;     p0.xmax=(float)STIMDUR;
	p0.samp_period = SRATE;
	p0.skip = 4;
	strcpy(p0.lab_x,"Milliseconds");
	strcpy(p0.lab_y," ");
	strcpy(p0.title,"Signals From ADC1 and ADC2 of DD1");


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
	/* Note, the factor of 1000.0 is because STIMDUR is in */
	/* milliseconds while SRATE is in microseconds.        */
	npts = (int)(1000.0 * STIMDUR / SRATE);

	trash();
	dropall();

	/* Allocate a DAMA buffer to hold rec data */
	allot16(WAVE1,npts);
	allot16(WAVE2,npts);

	gotoxy(37,3);
	out("DUALREC\n");
	gotoxy(1,4);
	out("      ---------------------------------------------------------------\n");
	gotoxy(1,5);
	out("      This example demonstrates dual channel recording using the. \n");
	out("      A/D channels on a DD1 Stereo Analog Interface and AP2 Array \n");
	out("      Processing card. \n\n");


	gon();

	/* Program the DD1 with desired parameters */
	/* --------------------------------------- */
	DD1clear(1);
	/* DD1speriod() returns the sampling period the 
	   DD1 is actually using; DD1srate() doesn't */
	sprintf(s, "      Sampling period is %f us.\n", DD1speriod(1,SRATE));
	out(s);
	sprintf(s, "      Recording %d ms (%ld samples) each keypress.\n", 
		STIMDUR, npts);
	out(s);
	DD1mode(1,DUALADC);
	DD1npts(1,npts);
	DD1strig(1);
	out("      ---------------------------------------------------------------\n");

	gotoxy(22,16);
	out("Press any key to rec, [X] to quit...\n");

	for (;;) {
		gotoxy(28,13);
		sprintf(s, "==>  Iteration: %d  <==",i );
		out(s);

		/* You don't have to do the following 3 lines; I did it just to 
		   clear the WAVE1 DAMA buffer before each A/D conversion, to
		   see if I was getting a full <STIMDUR> ms of data */
		dpush(npts);
		value(0.0);
		qpop16(WAVE1);


		/* drecord() must be called before each A/D conversion */
		drecord(WAVE1, WAVE2);

	    DD1arm(1);

	    DD1go(1);

		/* The following do/while loop is very important! It causes
		   the program to wait until the DD1 has finished taking data
		   before retrieving it from the AP2 DAMA buffer.  If you use
		   it in your program, use it precisely like this! */
			do {} while (DD1status(1) != 0);


	    tdtplot1(&p0, WAVE1);
	    tdtplot2(&p0, WAVE2);

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

	endwin();

}
