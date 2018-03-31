#include <stdlib.h>
#include <stdio.h>
#include <dos.h>
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


#include "apos.h"
 
#define word unsigned int

char  str[1024];
float buf[0x2000];
int *tbuf;
 
void showbits(long adr);
 
void main(int argc, char *argv[], char *env[])
{
  long npts;
  int i,z,apn;
  long m,x;
  long sr,dr = 0;
  long m1,m2;
  float serr1,derr1;
 
  tbuf = (int *)buf;


#ifdef NCURSES

  initscr();
  cbreak();
  noecho();
  /* nodelay(stdscr, TRUE); */

#endif

  clrscr();
  m1 = 0;
  m2 = 0;
  out("AP2 Diagnostic Program...                ( Press any key to abort )\n");
 
  apn = APa;
  if(argc==2)
    {
      if(!strcmp(argv[1],"B") || !strcmp(argv[1],"b"))
	apn = APb;
      if(!strcmp(argv[1],"APb") || !strcmp(argv[1],"apb"))
	apn= APb;
    }
 
  if(!apinit(apn))
    {
      out("\n\n\nSpecified AP2 not responding on bus.\n\n");
      out("Program usage:  APTST {APa} {APb}  (default = APb)\n\n\n");
      exit(0);
    }
 
  m=freewords();
  out("\n\n  Functioning Memory Detected: \n\n");
  out("      128K Program SRAM\n");
  sr = 32768;

  switch((int)(m >> 8))
    {
    case 0x80:
      {
	out("      128K Data SRAM\n");
	dr = 0;
	sr = 0x8000;
	break;
      }
    case 0x200:
      {
	out("      512K Data SRAM\n");
	dr = 0;
	sr = 0x20000;
	break;
      }
    case 0x1200:
      {
	out("      512K Data SRAM\n");
	out("      4Meg Data DRAM\n");
	dr = 0x100000;
	sr = 0x020000;
	m1 = 0x610000;
	break;
      }
    case 0x2080:
      {
	out("      128K Data SRAM\n");
	out("      8Meg Data DRAM\n");
	dr = 0x200000;
	sr = 0x008000;
	m1 = 0x610000;
	m2 = 0xa10000;
	break;
      }
    case 0x1080:
      {
	out("      128K Data SRAM\n");
	out("      4Meg Data DRAM\n");
	dr = 0x100000;
	sr = 0x008000;
	m1 = 0x610000;
	break;
      }
    case 0x2200:
      {
	out("      512K Data SRAM\n");
	out("      8Meg Data DRAM\n");
	dr = 0x200000;
	sr = 0x020000;
	m1 = 0x610000;
	m2 = 0xa10000;
	break;
      }
    }
 
  gotoxy(10,13);
 
  out("Bit-wise Testing:\n\n");
 
  gotoxy(10,15);
  out("    Data SRAM...      ");
  showbits(0x5e0000);
 
  if(m1)
    {
      gotoxy(10,15);
      out("    LOWER DRAM...      ");
      showbits(0x610000);
    }
 
  if(m2)
    {
      gotoxy(10,15);
      out("    UPPER DRAM...      ");
      showbits(0xa10000);
    }
 
  gotoxy(1,10);
  for(i=0; i<12; i++)
    out("                                                                          \n");
 
  npts = 0x1000;
  serr1=0;
  gotoxy(10,12);
  out("Testing Data SRAM...");
 
  for(i=0; i<10; i++)
    {
      gotoxy(40,12);
      sprintf(str,"Pass =>  %d        ",i);
      out(str);
      dropall();
      dpush(10);
      x=0;
      z=0;
      do
	{
	  dpush(npts);
	  tone(10.0,1000.0);
	  popf(buf);
	  pushf(buf,npts);
	  rfft();
	  rift();
	  pushf(buf,npts);
	  subtract();
	  serr1+=average();
	  if(serr1>1e3) serr1 = 999;
	  if(-serr1>1e3) serr1 = -999;
	  gotoxy(15,14);
	  sprintf(str,"%d:   %f   ",z,serr1);
	  out (str);
	  x+=npts << 1;
	  z++;
	  drop();
	  drop();
	  dpush(x);
	  if(kbhit())
	    x=3e6;
	}while(x+(npts<<1) < sr);
      if(kbhit())
	{
	  (void)getch();
	  break;
	}
    }
 
  if(dr>0)
    {
      npts = 0x1000;
      derr1=0;
      gotoxy(10,18);
      out("Testing Data DRAM...");
      for(i=0; i<10; i++)
	{
	  gotoxy(40,18);
	  sprintf(str,"Pass =>  %d        ",i);
	  out (str);
	  dropall();
	  dpush(sr);
	  dpush(10);
	  x=0;
	  z=0;
	  do
	    {
	      dpush(npts);
	      tone(10.0,1000.0);
	      qdup();
	      rfft();
	      rift();
	      subtract();
	      derr1+=average();
	      if(derr1>1e3) derr1 = 999;
	      if(-derr1>1e3) derr1 = -999;
	      drop();
	      drop();
	      x+=(npts<<1);
	      z++;
	      dpush(x);
	      gotoxy(15,20);
	      sprintf(str,"%d:   %6.6f    ",z,derr1);
	      out (str);
	      if(kbhit())
		x=3e6;
	    }while(x+(npts << 1) < dr);
	  if(kbhit())
	    {
	      (void)getch();
	      break;
	    }
	}
    }

#ifdef CURSES

  endwin();

#endif
}
 
 
void showbits(long adr)
{
  int j,i;
  long m;
 
  for(i=0; i<0x2000; i++)
    tbuf[i] = i;
  pushpoptest(tbuf,adr,0x2000);
  gotoxy(20,19);
  printf("Errors:     [ None ]                                         ");
  for(i=0; i<0x2000; i++)
    {
      if((tbuf[i]^i)!=0)
	{
	  m = (long)(tbuf[i]^i);
	  gotoxy(30,19);
	  if(!(i & 1))
	    out("00000000 00000000 ");
	  for(j=15; j>=0; j--)
	    {
	      if((m >> j) & 1)
		out("1");
	      else
		out("0");
	      if(j==8) out(" ");
	    }
	  if(i & 1)
	    out(" 00000000 00000000");
	}
    }
  gotoxy(20,22);
  out("...Done (press any key to continue)");
  (void)getch();
  gotoxy(20,22);
  out("                                   ");

  endwin();
}
