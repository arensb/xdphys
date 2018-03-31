#include <stdlib.h>
#include <math.h>
#ifndef __linux__
#include <bios.h>
#endif __linux__
#include <stdio.h>
#include <dos.h>
#include <string.h>
#include "xbdrv.h"

#define TXPORT			0
#define RXPORT			0
#define BAUDLOW			0
#define BAUDHIGH		1
#define INTENABLE	1
#define INTID			2
#define LINECONT		3
#define MODEMCONT	4
#define LINESTAT		5
#define MODEMSTAT		6

#define B300 384
#define B1200 96
#define B2400 48
#define B4800 24
#define B9600 12
#define B19200 6
#define B38400 3

#define upper(x) ((x & 0xff00)>>8)
#define lower(x) (x & 0xff)

void usageerr(void)
{
	printf("\n\nUsage:  XBCOMINI <ioport> <baudrate>\n\n");
	printf("    ioport => APa, APb, COM1, COM2, COM3 or COM4.\n");
	printf("  baudrate => 1200, 2400, 4800, 9600, 19200 or 38400.\n\n");
	exit(0);
}

int linecontrolport, baudhiport, baudloport, linestatusport;
int intenableport, modemcontrolport;

#ifndef __linux__
/* --------------------------------------------------------------------- */
/* Linux note: This section must be adapted to use the /dev/cua[0-3]     */
/*				devices; see setserial(8) code for ideas on how to       */
/*				set the baud rate for the UART of the COM port           */
/*              You could also use tcsetattr(2).                         */
/* --------------------------------------------------------------------- */
/*************************************************
**		comset(baud,comport)			  **
**		does initialization of specified comport  **
**		parity is fixed to NONE, word size is	  **
**		8 bits, with 1 stop bit.		  **
**						**
**		parameters:				  **
**		baud -- any of the following baud rate	  **
**		values are legal.  1200,2400,4800,	  **
**		9600,19200,38400.			  **
**		comport -- 1 - 4 are recognized for COM1  **
**		through COM4				  **
*************************************************/
void comset(long baud)
{
	char commandbyte;
	int baudval;
	unsigned char temp;


	if ((baud != 1200) && (baud != 2400) &&
		(baud != 4800) && (baud != 9600) && (baud != 19200) && (baud != 38400))
	{
		printf("\n\n\n	Illegal Baud Rate Specified.");
		usageerr();
	}

	if ((cc < 1) || (cc > 4))
	{
		printf("\n\n\n	Illegal COM Port Specified.");
		usageerr();
	}

	switch(cc)
	{
		case 1:
					cpa[0] = COM1base;
					break;
		case 2:
					cpa[0] = COM2base;
					break;
		case 3:
					cpa[0] = COM3base;
					break;
		case 4:
					cpa[0] = COM4base;
					break;
	}

	switch(baud)
	{
		case 1200:
					baudval = B1200;
					break;

		case 2400:
					baudval = B2400;
					break;

		case 4800:
					baudval = B4800;
					break;

		case 9600:
					baudval = B9600;
					break;

		case 19200:
					baudval = B19200;
					break;

		case 38400:
					baudval = B38400;
					break;
		default:
					usageerr();
	}

	linecontrolport = cpa[0] + LINECONT;
	baudhiport = cpa[0] + BAUDHIGH;
	baudloport = cpa[0] + BAUDLOW;
	linestatusport = cpa[0] + LINESTAT;
	intenableport = cpa[0] + INTENABLE;
	modemcontrolport = cpa[0] + MODEMCONT;

	/* Linux note: bioscom(cmd, byte, port) sets no parity, 9 bits, 1 
	   stop bit and 9600 baud rate of the UART for the apropriate COM port, 
	   I think */
	commandbyte = 0xE3;	/* n-8-1 9600 */
	bioscom(0,commandbyte,cc-1);

	outportb(intenableport,0x00);
	temp = inportb(linecontrolport);
	temp = temp | 0x80;
	outportb(linecontrolport,temp);
	outportb(baudloport, lower(baudval));
	outportb(baudhiport, upper(baudval));
	temp = temp & 0x7f;
	outportb(linecontrolport,temp);
}
#endif /* __linux__ */



void apcomset(void)
{
	switch(cc)
	{
		case COM_APa:
		{
			cc = COM_APa;
			cpa[0] = 0x230;
			break;
		}
		case COM_APb:
		{
			cc = COM_APb;
			cpa[0] = 0x250;
			break;
		}
		default:
		{
			printf("\n\n\n	Illegal COM Port Specified.");
			printf("\n	Use:  COM1, COM2, COM3, COM4, APa or APb.");
			printf("\n\n\n");
			exit(0);
		}
	}
	io_init(cpa[0]);
	outportb(cpa[0],3);
	outportb(cpa[0],3);
	outportb(cpa[0],3);
	outportb(cpa[0],0x15);
	outportb(cpa[0],0x15);
	outportb(cpa[0],0x15);
}


void main(int argc, char *argv[], char *env[])
{
	 char *s;
	 long baudrate;
	 int i,j;
	 int xx;

	 xbtimeout = (long)XB_TIMEOUT * 100;

	 cc = COM_APa;

	 s = getenv("XBCOM");
	 if (s != NULL) {
#ifndef __linux__
		 if (!strncmp(s,"COM1", 4) || !strncmp(s,"com1"), 4)
			 cc = COM1;
		 if (!strncmp(s,"COM2", 4) || !strncmp(s,"com2"), 4)
			 cc = COM2;
		 if (!strncmp(s,"COM3", 4) || !strncmp(s,"com3"), 4)
			 cc = COM3;
		 if (!strncmp(s,"COM4", 4) || !strncmp(s,"com4"), 4)
			 cc = COM4;
#endif /* __linux__ */
		 if (!strncmp(s,"APb", 3) || !strncmp(s,"apb", 3))
			 cc = COM_APb;
	 }

	 baudrate = 38400;
	 xx = 0;

	 if (argc > 1) {
		 if (!strncmp(argv[1],"APa",3) || !strncmp(argv[1],"apa",3) || 
			 !strncmp(argv[1],"APA",3))
			 xx = COM_APa;
		 if (!strncmp(argv[1],"APb",3) || !strncmp(argv[1],"apb",3) || 
			 !strncmp(argv[1],"APB",3))
			 xx = COM_APb;
#ifndef __linux__
		 if (!strncmp(argv[1],"COM1",4) || !strncmp(argv[1],"com1",4))
			 xx = COM1;
		 if (!strncmp(argv[1],"COM2",4) || !strncmp(argv[1],"com2",4))
			 xx = COM2;
		 if (!strncmp(argv[1],"COM3",4) || !strncmp(argv[1],"com3",4))
			 xx = COM3;
		 if (!strncmp(argv[1],"COM4",4) || !strncmp(argv[1],"com4",4))
			 xx = COM4;
#endif /* __linux__ */

		 if (!xx)
			 sscanf(argv[1],"%ld",&baudrate);
		 else
			 cc = xx;
	 }

	 if (argc==3)
	 {
		 if (!xx)
		 {
			 printf("\n\nIllegal argument specified.");
			 usageerr();
		 }
		 sscanf(argv[2],"%ld",&baudrate);
	 }

#ifndef __linux__
	 if (cc<COM_APa)
		 comset(baudrate);
	 else
#endif /*__linux__*/
		 apcomset();

	 for(i=0; i<100; i++)
	 {
		 iosend(BAUD_LOCK);
		 for(j=0; j<10000; j++);
	 }
}

