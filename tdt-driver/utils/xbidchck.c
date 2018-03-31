#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <dos.h>
#include <assert.h>

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

void main(int argc, char *argv[])
{
	int i,j,rn,pn,xbl;

	int 	cc;
	char s[20];
	int vflag,v;


	vflag = 0;
	cc = USE_DOS;

	if (argc > 1) {
		strncpy(s,argv[1], 19);

		if (!strncmp(s,"COM1",4) || !strncmp(s,"com1",4))
			cc = COM1;

		if (!strncmp(s,"COM2",4) || !strncmp(s,"com2",4))
			cc = COM2;

		if (!strncmp(s,"COM3",4) || !strncmp(s,"com3",4))
			cc = COM3;

		if (!strncmp(s,"COM4",4) || !strncmp(s,"com4",4))
			cc = COM4;

		if (!strncmp(s,"APa",3) || !strncmp(s,"apa",3) || !strncmp(s,"APA",3))
			cc = COM_APa;

		if (!strncmp(s,"APb",3) || !strncmp(s,"apb",3) || !strncmp(s,"APB",3))
			cc = COM_APb;


		if ((argc==2) && (!strncmp(argv[1],"V",1) || !strncmp(argv[1],"v",1))) 
			 vflag = 1;
		else 
			if ((argc==3) && (!strncmp(argv[2],"V",1) || !strncmp(argv[2],"v",1)))
				vflag = 1;


		if ((cc==USE_DOS) && (vflag == 0)) {
			printf("\n\n  Illegal argument specified. ");
			printf("\n\n	    Usage:  XBIDCHCK {port_spec} {v}");
			printf("\n	    port_spec = APa, APb, COM1, COM2, COM3 or COM4");
			printf("\n		    v = include microcode version information");

			exit(0);
		}


	}


#ifdef NCURSES

    initscr();
    cbreak();
    noecho();
    /* nodelay(stdscr, TRUE); */

#endif
	clrscr();

	out("XBUS Device Identification Check-Out Program: \n\n");


	XB1init(cc);
	out("    Rack			       Position No.\n");
	out("     No.	 - 1 -		  - 2 -		   - 3 -	    - 4 -\n");
	out("  ----------------------------------------------------------------------------");


	for (i=1; i<=XB_MAX_DEV_TYPES; i++)
	{
		for (j=1; j<=XB_MAX_OF_EACH_TYPE; j++)
		{
			xbl = xbcode[i][j][xbsel];
			if (xbl != 0)
			{
				rn = xbl >> 2;
				pn = xbl & 3;

				gotoxy(5,rn+5);
				sprintf(s,"%d ->",rn);
				out(s);

				if (!vflag)
				{
					gotoxy((pn-1)*17+30,rn+5);
					sprintf(s, "  { %s_(%d) }",xbname[i],j);
					out(s);
				}
				else
				{
					v = XB1version(i,j);
					gotoxy((pn-1)*17+30,rn+5);
					sprintf(s, "{ %s_(%d) v%d }",xbname[i],j,v);
					out(s);
				}
			}
		}
	}


  gotoxy(15,20);
  out("Press any key to continue...\n");
  while (!kbhit());


  endwin();
}
