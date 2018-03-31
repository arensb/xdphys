#include <stdlib.h>
#include <stdio.h>
#include <dos.h>
#include <math.h>
#include <string.h>

#ifdef MSDOS
#include <conio.h>
#define out(a)		fputs(a, stdout)
#endif

#ifdef NCURSES
#include <curses.h>
#define clrscr()	clear()
#define	gotoxy(x,y)	move((y)-1,(x)-1)
#define kbhit()	        (getch() != ERR)
#define out(a)		addstr(a),refresh()
#endif

#include "xbdrv.h"

void main(int argc, char *argv[], char *env[])
{
  int i,racknum;
	int cc;
	char s[80];

	cc = USE_DOS;
	if (argc>1)
	{
		strcpy(s,argv[1]);
		if (!strcmp(s,"COM1") || !strcmp(s,"com1"))
			cc = COM1;
		if (!strcmp(s,"COM2") || !strcmp(s,"com2"))
			cc = COM2;
		if (!strcmp(s,"COM3") || !strcmp(s,"com3"))
			cc = COM3;
		if (!strcmp(s,"COM4") || !strcmp(s,"com4"))
			cc = COM4;
		if (!strcmp(s,"APa") || !strcmp(s,"apa") || !strcmp(s,"APA"))
			cc = COM_APa;
		if (!strcmp(s,"APb") || !strcmp(s,"apb") || !strcmp(s,"APB"))
			cc = COM_APb;

		if (cc==USE_DOS)
		{
			out("\n\n  Illegal COM Port specified.  Use: COM1..4, APa or APb.\n\n");
			exit(0);
		}
	}


  if (!XB1init(cc)) {
  	out(" Error initializing XB1!\n\n");
	exit(0);
	}

  XB1flush();

  racknum=1;

#ifdef NCURSES
  initscr();
  cbreak();
  noecho();
  /* nodelay(stdscr, TRUE); */
#endif

  clrscr();
  out(">>> XBUS Rack Identification Program.\n\n");
  out("	All XB1s connected to XBUS should now have both the ACTIVITY\n");
  out("	and ERROR indicators illuminated.  Press the ID button on the \n");
  out("	first (logical) XB1 in your system.  When the ID button is pressed\n");
  out("	the ERROR indicator will go off the and the XB1's newly\n");
  out("	assigned rack-ID-number will be displayed below.  Continue this\n");
  out("	process until all racks in the system have been IDed.  Press\n");
  out("	any key on the PC keyboard to terminate this program.	   \n");
  gotoxy(1,13);
  out("	    Pinging...\n\n");


  while(kbhit()) (void)getch();

  do
  {
    iosend(ARB_ID);
#ifndef __linux__
    delay(500);
#else 
	usleep(500);
#endif
    i=iorec();
    if (i==ARB_ACK) {
		sprintf(s, "		  Rack - %d   IDed.\n",racknum);
		out(s);
		iosend(racknum);
		racknum++;
    }
  }while(!kbhit());
  (void)getch();

  iosend(ARB_RST);
  iosend(ARB_RST);
  iosend(ARB_RST);

  endwin();
}
