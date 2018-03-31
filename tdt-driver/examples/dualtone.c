 /******************************************************************
**   RCSID: $Id: dualtone.c,v 1.4 1997/09/08 23:00:29 cmalek Exp $
** RCSName: $Name:  $
** Program: dualtone
**    File: dualtone.c
**  Author: cmalek, TDT Inc.
** Descrip: 
**
**	Two channel playback with the DD1.  A tone is synthesized on
**	the AP2 array processor, and simultaneously played from both D/A 
**	outputs 1 and 2 of the DD1.
**
**	Based on EX3-1-1.C from the Tucker Davis driver.
**
*******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "xbdrv.h"
#include "apos.h"
#include <unistd.h>
#include <curses.h>

#define clrscr()	clear()
#define	gotoxy(x,y)	move((y)-1,(x)-1)
#define kbhit()	        (getch() != ERR)
#define out(a)		addstr(a),refresh()

#define STIMDUR    100				/* Tone blip duration in milliseconds */
#define SRATE    10.0				/* Sampling period in mircoseconds    */
#define FREQ     1000.0				/* Test tone frequency */
#define NREPS    10				    /* Play the test tones this many times */

#define WAVE1       1     	/* Setup logical name for play buffer 1 */
#define WAVE2       2     	/* Setup logical name for play buffer 2 */


void main() {

	long  npts;
	char  c;
	char  s[80];

	initscr();
	cbreak();
	noecho();
	/* nodelay(stdscr, TRUE); */

	clrscr();

	/* Print display header */
	gotoxy(1,1);
	out("   Tucker-Davis Technologies          Gainesville, Florida        \n");

	/* Inialize Hardware (AP2 and XBUS) */
	if (!apinit(APa) || !XB1init(USE_DOS)) {
		out("\n\n  Error initializing hardware !!! \n\n");
		exit(0);
	}

	/* Calculate number of points needed for 50ms signal.  */
	/* Note, the factor of 1000.0 is because STIMDUR is in */
	/* milliseconds while SRATE is in microseconds.        */
	npts = (int)(1000.0 * STIMDUR / SRATE);

	/* Allocate two DAMA buffers to hold play data */
	allot16(WAVE1,npts);
	allot16(WAVE2,npts);

	gotoxy(1,3);
	out("Dual channel playback test program\n\n");
	out("This program demonstrates dual channel playback with the DD1.\n");

	/* Program the DD1 with desired parameters */
	DD1clear(1);
	DD1npts(1,npts);
	sprintf(s, "==> Sampling period is %f\n", DD1speriod(1,SRATE));
	out(s);
	DD1mode(1,DUALDAC);
	DD1mtrig(1);
	DD1reps(1,NREPS); /* Play the test sounds NREPS times */

	/* Generate tones for DAC1 and DAC2 */
	dpush(2*npts);      	  /* Get memory for 2 waveforms on top of 
								 the stack */
	tone(FREQ,SRATE); 	     /* Fill buffer with tone of spec'ed freq */
	qwind(2.0,SRATE); 	     /* Apply Cos2 window with 2ms r/f time  */
	scale(10.0);
	scale(3276.7);    	     /* Scale for playing from 16 bit DAC    */
	qdup();
	qpop16(WAVE1);     	     /* Move data to DAMA area 1 */
	qpop16(WAVE2);     	     /* Move data to DAMA area 2 */

	sprintf(s, "==> Test tone freq is %f\n", FREQ);
	out(s);
	sprintf(s, "==> Test tones are %d ms long\n", STIMDUR);
	out(s);
	sprintf(s, "==> Playing test tones %d times.\n", NREPS);
	out(s);
	out("Press any key to play, [X] to quit...\n");


	dplay(WAVE1, WAVE2);
	DD1arm(1);               /* Arm DD1 */
	do {
		if (DD1status(1) == 1){
			out("DD1 go!");
			DD1go(1);  /* If the DD1 is armed       */
		}

		/* This aborts program if [x] key is pressed during the play */

		if (kbhit()) {

			c=getch();
			if (c=='x' || c=='X')
			{
				/* stop and clear the DD1 */

				DD1stop(1);
				DD1clear(1);
				gotoxy(10,20);
				out("Program stopped by user.\n\n");
				exit(0);
			}
		}
	} while(DD1status(1));

	/* stop and clear the DD1 */
	out("\n\n         Press any key to end...\n");
	getch();
	DD1stop(1);
	DD1clear(1);

	endwin();
}
