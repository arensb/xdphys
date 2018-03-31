#ifndef _conio_h
#define _conio_h

#include <curses.h>

#define clrscr()	clear()
#define	gotoxy(x,y)	move((y)-1,(x)-1)
#define kbhit()	        (getch() != ERR)
#define printf(a)	addstr(a),refresh()

#endif
