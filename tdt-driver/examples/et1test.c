/******************************************************************
**   RCSID: $Id: et1test.c,v 1.3 2000/07/12 22:55:04 cmalek Exp $
** RCSName: $Name:  $
** Program: et1test
**    File: et1test.c
**  Author: cmalek
** Descrip: 
**
**	Test program for ET1/SD1 with TG6.  For a test signal for input to 
**  the SD1, use a 1 KHz, +/-5V sine wave, and set the hoops for the SD1 
**  as follows:
**
**       Hoop #1: Slope: RISE 
**                Upper: 1.0
**       Hoop #2: Slope: PEAK 
**                Delay: 0.13
**                Width: 0.151
**                Upper: 6.0
**                Lower: 4.0
**
**	Connect your setup as follows:
**
**			TG6 output 1 -> middle input of ET1
**			Signal Generator output -> SD1 "Input" input
**			ET1 In-1 -> SD1 "First" input
**			ET1 In-2 -> SD1 "Last" input
**
**	This program does one spike gathering cycle, and reports the number
**  of spikes gathered.
**
*******************************************************************/
#define XBDRV_SCHED

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sched.h>
#include <unistd.h>
#include <asm/system.h>
#include <assert.h>
#include "xbdrv.h"
#include "apos.h"


/* ------------------------------------------------
 * Static function prototypes
 * ------------------------------------------------ */
static void init_hardware(void);
static void program_tg6(void);
static void program_et1(void);
static void trigger(void);
static void cleanup(void);
static void get_spikes(unsigned long *);

int main()
{
	long *times = NULL;

	init_hardware();
	program_tg6();
	program_et1();

	trigger();

	do {} while (TG6status(1) != 0);

	ET1stop(1);

	do {} while (ET1active(1) != 0);

	get_spikes(times);

	cleanup();

	return(0);

}


/* ------------------------------------------------------------------------
	init_hardware
   ------------------------------------------------------------------------ */
static void init_hardware(void)
{
	/* Initialize Hardware (AP2 and XBUS */
	if (!apinit(APb) || !XB1init(USE_DOS))
	{
		printf("\n\n  Error initializing hardware !!! \n\n");
		exit(0);
	}
}

/* ------------------------------------------------------------------------
	program_tg6
   ------------------------------------------------------------------------ */
static void program_tg6(void)
{
	TG6clear(1);
	TG6baserate(1, _1ms);
	TG6new(1, 0, 200, 0x00); /* New timing sequence #0, 200 ms long */
	TG6high(1, 0, 0, 100, 0x02); /* Set output #2 high for 0-100ms */
	TG6low(1, 0, 100, 200, 0x02); /* Set output #2 low for 100-200ms */
}

/* ------------------------------------------------------------------------
	program_et1
   ------------------------------------------------------------------------ */
static void program_et1(void)
{
	ET1clear(1);
}

/* ------------------------------------------------------------------------
	trigger
   ------------------------------------------------------------------------ */
static void trigger(void)
{
	ET1go(1);            /* Enable ET1 */
	TG6arm(1,0);
	TG6go(1);
}

/* ------------------------------------------------------------------------
	cleanup
   ------------------------------------------------------------------------ */
static void cleanup(void)
{
	ET1clear(1);
	TG6clear(1);
}

/* ------------------------------------------------------------------------
	get_spikes
   ------------------------------------------------------------------------ */
static void get_spikes(
	unsigned long *times)
{
	int  ntimes;

	ntimes = ET1readall32(1, &times);

	fprintf(stderr, "et1test: nspikes=%d\n", ntimes);

}

