/******************************************************************
**   RCSID: $Id: tonetest.c,v 1.6 2000/07/12 22:55:05 cmalek Exp $
** RCSName: $Name:  $
** Program: tonetest
**    File: tonetest.c
**  Author: cmalek, TDT Inc.
** Descrip: 
**
**	Simple one channel playback with the DD1.  A tone is synthesized on
**	the AP2 array processor, and played from D/A output 1 of the DD1.
**
**	Based on EX3-1-1.C from the Tucker Davis driver.
**
**	Though written for Linux, this may work under MS-DOS, as well.
**
*******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifdef MSDOS
#include <graphics.h>
#include <conio.h>
#include "c:\tdt\drivers\xbdrv\c\xbdrv.h"
#include "c:\tdt\drivers\apos\c\apos.h"
#include "c:\tdt\drivers\examples\tdtplot.h"
#define out(a)		fputs(a, stdout)
#endif

#ifdef __linux__
#include "xbdrv.h"
#include "apos.h"
#include <unistd.h>
#endif


#ifdef NCURSES
#include <curses.h>
#define clrscr()	clear()
#define	gotoxy(x,y)	move((y)-1,(x)-1)
#define kbhit()	        (getch() != ERR)
#define out(a)		addstr(a),refresh()
#endif


#define STIMDUR    1000				/* Tone blip duration in milliseconds */
#define SRATE    5.0				/* Sampling period in mircoseconds    */

#define WAVE1       1     	/* Setup logical name for play buffer */


void main()
{

#ifdef MSDOS
	int   first=1;
#endif
	long  npts;
	float freq;
	char  c;
	char  s[80];
#ifdef MSDOS
	plotinfo p0;
#endif

#ifdef NCURSES
  initscr();
  cbreak();
  noecho();
  /* nodelay(stdscr, TRUE); */
#endif

	clrscr();

#ifdef MSDOS
	initplotinfo(&p0);              /* plotting parameters for signal  */
	p0.xx1=100;      p0.yy1=50;
	p0.xx2=450;      p0.yy2=200;
	p0.ymin=-10.0;   p0.ymax=10.0;
	p0.xmin=0.0;     p0.xmax=(float)STIMDUR;
	strcpy(p0.lab_x,"Milliseconds");
	strcpy(p0.lab_y,"Volts");
	strcpy(p0.title,"Signal From APOS tone Command");
#endif


/* Print display header */
#ifndef NCURSES
	textbackground(1);
	textcolor(15);
#endif 
	gotoxy(1,1);
	out("   Tucker-Davis Technologies          Gainesville, Florida        \n");
#ifndef NCURSES
	textbackground(0);
#endif 

/* Inialize Hardware (AP2 and XBUS */
	if (!apinit(APa)) {
		out("\n\n  Error initializing APa!!! \n\n");
		endwin();
		exit(0);
	}
	if	(!XB1init(USE_DOS)) {
		out("\n\n  Error initializing XB1 !!! \n\n");
		endwin();
		exit(0);
	}

/* Calculate number of points needed for 50ms signal.  */
/* Note, the factor of 1000.0 is because STIMDUR is in */
/* milliseconds while SRATE is in microseconds.        */
	npts = (int)(1000.0 * STIMDUR / SRATE);

/* Allocate a DAMA buffer to hold play data */
	allot16(WAVE1,npts);

	gotoxy(1,3);
	out("Chapter 3: Example-1, Method-1\n\n");
	out("This example demonstrates the APOS \"tone\" command.  The tone\n");
	out("command has the advantage of highly accurate frequency placement,\n");
	out("however, there is no control over the starting phase.\n\n");
	out("A simple tone of the same frequency will be played 10 times\n");
	out("and then the frequency will be incremented by 10%.  The starting\n");
	out("frequency is 1000 and the program will terminate at 5000 Hz.\n\n");
	out("Press any key to play, [X] to quit...\n");

/* Program the DD1 with desired parameters */
	DD1clear(1);
	DD1npts(1,npts);
	/* DD1srate(1,SRATE); */
	sprintf(s, "Sampling period is %f\n", DD1speriod(1,SRATE));
	out(s);
	DD1mode(1,DAC1);
	DD1mtrig(1);
	DD1reps(1,1);

/* This is main loop that steps through frequencies */
	freq = 1000.0;
	c=getch();
	if (c=='x' || c=='X')
	{
/* When [x] is entered, stop and clear DD1 */
		DD1stop(1);
		DD1clear(1);
		gotoxy(10,20);
		out("Program stopped by user.\n\n");
		exit(0);
	}

	/*clrscr();*/
#ifdef MSDOS
	gon();
#endif 
	do
	{
/* Generate tone buffer using APOS */
		dpush(npts);      	     /* Get empty buffer on top of the stack */
		tone(freq,SRATE); 	     /* Fill buffer with tone of speced freq */
		qwind(2.0,SRATE); 	     /* Apply Cos2 window with 2ms r/f time  */
		scale(10.0);

#ifdef MSDOS
		tdtplot1(&p0);                        /* plot the generated data */
		if (first)  {
			 tdtplot2(&p0,LIGHTGREEN,START);    /* for first time plotting */
			 first = 0;
		}
		else
			 tdtplot2(&p0,LIGHTGREEN,REFRESH);  /* for refreshing the plot */
#endif 


		scale(3276.7);    	     /* Scale for playing from 16 bit DAC    */
		qpop16(WAVE1);     	     /* Move data to DAMA area               */

		play(WAVE1);             /* Tell AP2 to play from WAVE1 */
		DD1arm(1);               /* Arm DD1 */

		gotoxy(10,20);
		sprintf(s, "Play frequency: %7.1f,  Press [x] to quit.   ",freq);
		out(s);

/* Wait for DD1 to play tone buffer 2 times and stop */
		do
		{
			if (DD1status(1) == 1)  DD1go(1);  /* If the DD1 is armed       */

/* This aborts program if [x] key is pressed during the play */
			if (kbhit())
			{

				c=getch();
				if (c=='x' || c=='X')
				{
/* stop and clear the DD1 and the graphics */
#ifdef MSDOS
					goff();
#endif
					DD1stop(1);
					DD1clear(1);
					gotoxy(10,20);
					out("Program stopped by user.\n\n");
					exit(0);
				}
			}
		} while(DD1status(1));

/* Step to next frequency */
		freq = freq * 1.1;
#ifndef __linux__
		delay(100);
#else 
		usleep(500);
#endif

	} while(freq<5055.0);

/* stop and clear the DD1 and the graphics */
	out("\n\n         Press any key to end...\n");
	getch();
#ifdef MSDOS
	goff();
#endif
	DD1stop(1);
	DD1clear(1);

#ifdef NCURSES
  endwin();
#endif

}
