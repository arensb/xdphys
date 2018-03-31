/******************************************************************
**  RCSID: $Id: att.c,v 2.50 1999/05/12 18:22:23 cmalek Exp $
** Program: iserver
**  Module: att.c
**  Author: mazer
** Descrip: interace to programmable attenuators
**
** Revision History (most recent last)
**
** Wed Mar  9 15:53:36 1994 mazer
**  creation date
**
** 98.11 bjarthur
**  changed attMaxAtten to 90.0 b/c PA4s aren't accurate beyond that
**
*******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#ifndef __linux__
#include <fcntl.h>
#include <malloc.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#endif /* __linux__ */
#include <assert.h>
#include "iserver.h"
#include "iserverInt.h"
#include "port.h"


/* ------------------------------------------------
   Macros
   ------------------------------------------------ */
#ifndef __tdtproc__
#define PA4_1_ID 0x04
#define PA4_2_ID 0x05

#define TTYDEV		"/dev/ttya"

/* BAUDRATE can be B9600, B19200, B38400 etc.. */
#define BAUDRATE	B38400
#define BAUDRATE_STR	"38400"
#endif /* __tdtproc__ */


/* ------------------------------------------------
   Public Variables
   ------------------------------------------------ */
float attMaxAtten = 90.0;

/* ------------------------------------------------
   Private Variables
   ------------------------------------------------ */
static float att_latten = 0.0;
static float att_ratten = 0.0;

/* ------------------------------------------------
   Prototypes
   ------------------------------------------------ */
#ifndef __tdtproc__
static void xb1_sendstr(int, char*);
static void PA4atten(int, float);
#endif /* __tdtproc__ */

/* ---------------------------------------------------------------------- */

#ifndef __tdtproc__
static void xb1_sendstr(int n, char *buf)
{
	int fd;
	static struct termios *t = NULL;

	if ((fd = open(TTYDEV, O_RDWR, 0777)) < 0) {
		perror(TTYDEV);
		exit(1);
	}
	if (t == NULL) {
		t = (struct termios *) calloc(1,sizeof(struct termios));
	}
	tcgetattr(fd, t);
	t->c_oflag &= ~(OPOST | OCRNL | XTABS);	 	/* clear */
	t->c_cflag &= ~(CBAUD | CSIZE | PARENB);		/* clear */
	t->c_cflag |= (BAUDRATE | CS8);			/* set   */
	tcsetattr(fd, TCSANOW, t);
	if (write(fd, buf, n) < 0)  {
		perror(TTYDEV);
		exit(1);
	}
	close(fd);
}

static void PA4atten(int id, float atten)
{
	char buf[6];
	int bitpat;

	bitpat = (int)(atten * 10.0 + 0.05);

	buf[0] = 0x7f & id;		/* device address */
	buf[1] = 0x44;		/* instruction length */
	buf[2] = 0x20;		/* PA4_ATT op code */
	buf[3] = 0xff & (bitpat >> 8);
	buf[4] = 0xff & bitpat;
	buf[5] = 0xff & (buf[2] + buf[3] + buf[4]);
	xb1_sendstr(6, buf);
}
#endif /* __tdtproc__ */

/* ---------------------------------------------------------------------
   setRack

   		CPM: freq is never used.
   --------------------------------------------------------------------- */
int setRack(
	int freq, 
	float latten, 
	float ratten)
{
	static int disable = -1;

	if (disable < 0) {
		if (getenv("ATTOFF") != NULL) {
			disable = 1;
			fprintf(stderr, "setRack: attenuator's disabled\n");
		} else {
			disable = 0;
		}
	}

	if (disable) {
#ifdef __tdtproc__
		is_setAtten(attMaxAtten,attMaxAtten);
#else /* __tdtproc__ */
		PA4atten(PA4_1_ID, attMaxAtten);
		PA4atten(PA4_2_ID, attMaxAtten);
#endif /* __tdtproc__ */
	} else {
		if (latten < 0.0 || latten < 0.0)
			return(1);

		if (freq != 0) {
			fprintf(stderr, "\007setRack: can't set frequency!!\n");
			return(0);
		}
		if (latten > attMaxAtten) {
			fprintf(stderr, "\007setRack: %4.1fdb on LEFT too big, using %4.1fdB\n",
				latten, attMaxAtten);
			latten = attMaxAtten;
		}
		if (ratten > attMaxAtten) {
			fprintf(stderr, "\007setRack: %4.1fdb on RIGHT too big, using %4.1fdB\n",
				ratten, attMaxAtten);
			ratten = attMaxAtten;
		}


		if (attMaxAtten == 98.0) {
			/* 03-Apr-94
			   something's wrong with 0db atten -- adding one to everything
			   this could be an imped. problem..
			   */
			latten += 1.0;
			ratten += 1.0;
		}
    
#ifdef __tdtproc__
		is_setAtten(latten,ratten);
#else /* __tdtproc__ */
		PA4atten(PA4_1_ID, latten);
		PA4atten(PA4_2_ID, ratten);
#endif /* __tdtproc__ */
		att_latten = latten;
		att_ratten = ratten;
	}

#ifndef __tdtproc__
	usleep(5000);			/* ~5ms settle time */
#endif /* __tdtproc__ */
	return(1);
}

void getRack(
	double *latten, 
	double *ratten)
{
	*latten = att_latten;
	*ratten = att_ratten;
}
