/******************************************************************
**  RCSID: $Id: iserver.c,v 2.59 2001/01/04 22:06:52 cmalek Exp $
**  Module: iserver
**  Author: mazer,cmalek
** Descrip: machine independent I.S. (Integral Server) code
**
** Revision History (most recent last)
**
** Sat Feb 15 14:31:41 1992 mazer
**   bit of the old clean-up .. nothing significant
**
** Tue Feb 18 01:03:59 1992 mazer
**   added handles for figuring rep number -- is_trial()
**
** Tue Oct 13 12:54:37 1992 mazer
**  striped out synth stuff into synth.c
**
** Mon Jun  6 10:54:08 1994 mazer
**  moved is_postsample_hook and is_sample() here.
**
** Wed Oct 12 17:21:09 1994 mazer
**  added is_sync_pulse flag
**
** 97.3 bjarthur
**  added is_getRad() to centralize its computation
**
** 98.11 cmalek
**  split libiserver.a off into a truly independant library
**
*******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdarg.h>
#include "iserver.h"
#include "iserverInt.h"
#include "port.h"


/* --------------------------------------------------------------------
   Public Variables
   -------------------------------------------------------------------- */
int		is_inited = 0;

extern int debugflag;

int 	is_syncPulse = 1; 
int 	is_testmode = 0; 

int 	is_daFc = -1;	/* public da conversion frequency */
int		is_adFc = -1;	/* public ad conversion frequency */
#ifdef __drpmproc__
int		is_evtFc = -1;	/* public event timer sampling freq */
#endif


#if(0)
int 	is_repnum = 0;	/* rep number of current step (0..nreps-1) */
int 	is_nreps  = 1;	/* total number of reps in this set */
int		is_depvar = -1;	/* current dependent var, if specified */
#endif

int		is_nice = -10;

int 	is_report_errors = 1;  /* notify with alert/notify boxes */

/* --------------------------------------------------------------------
   Private iserver Variables
   -------------------------------------------------------------------- */
xword   *is_inputBuffer = NULL;
int		is_inputSize;	    /* # samples per channel of is_inputBuffer */
xword   *is_outputBuffer = NULL;
int		is_outputSize;	    /* # samples per channel of is_outputBuffer */
int		ad_nchans = 2;
int		da_nchans = 2;

/* --------------------------------------------------------------------
   Prototypes
   -------------------------------------------------------------------- */
static void is_defaultFcSetFn(int, int, int);
static void is_defaultAlert(char *fmt, ...);
static void is_defaultNotify(char *fmt, ...);
static int is_rnd2int(float f);

FCSETFN	is_fcSetFn = is_defaultFcSetFn;
#ifndef MATLAB
MSGFN	is_alert = is_defaultAlert;
MSGFN	is_notify = is_defaultNotify;
#else
MSGFN	is_alert = mexPrintf;
MSGFN	is_notify = mexPrintf;
#endif
YESNOFN	is_yesno = NULL;

/* -------------------------------------------------------------------- */

void is_setYesNoFn(YESNOFN yesnofn)
{
	is_yesno = yesnofn;
}

void is_setFcSetFn(FCSETFN fcSetFn)
{
	is_fcSetFn = fcSetFn;
}

void is_setAlertFn(MSGFN alertfn)
{
	is_alert = alertfn;
}

void is_setNotifyFn(MSGFN notifyfn)
{
	is_notify = notifyfn;
}

#if(0)
double is_getRad(void)
{
	return(is_getRadWithArgs((double)is_repnum,(double)is_nreps));
}

double is_getRadWithArgs(double repnum, double nreps)
{
	return(2.0 * M_PI * (double)repnum / (double) nreps);
}
#endif

int is_getDAnchans(void)
{
	return(da_nchans);
}

int is_getADnchans(void)
{
	return(ad_nchans);
}

void is_setNice(int	nice)
{
	is_nice = nice;
}

int	is_getNice(void)
{
	return(is_nice);
}

void is_setSyncPulse(int syncpulse)	
{
	is_syncPulse = syncpulse;
}

int	is_getSyncPulse(void)	
{
	return(is_syncPulse);
}

xword *is_getDAbuffer(int *nsamps)
{
	if (nsamps != NULL)
		*nsamps = is_outputSize; /* # samples per channel in is_outputBuffer */
	return(is_outputBuffer);
}

int is_getDAmslength(void)
{
	return((int) (((float) (is_outputSize) / (float) is_daFc) * 1000.0));
}

int is_getADmslength(void)
{
	return((int) (((float) (is_inputSize) / (float) is_adFc) * 1000.0));
}

xword *is_getADbuffer(int *nsamps)
{
	if (nsamps != NULL )
		*nsamps = is_inputSize;
	return(is_inputBuffer);
}

int is_clearOutput(int lr)
{
	register int i;
	register xword *buf;

	if (lr & IS_LEFT) {
		buf = is_outputBuffer;
		for (i = is_outputSize; i; i--, buf += da_nchans)
			*buf = 0;
	}
	if (lr & IS_RIGHT) {
		buf = is_outputBuffer + 1;
		for (i = is_outputSize; i; i--, buf += da_nchans)
			*buf = 0;
	}
	return(IS_OK);
}

int is_clearInput(int lr)
{
	register int i;
	register xword *buf;

	if (lr & IS_LEFT) {
		buf = is_inputBuffer;
		for (i = is_inputSize; i; i--, buf += ad_nchans)
			*buf = 0;
	}
	if (lr & IS_RIGHT) {
		buf = is_outputBuffer + 1;
		for (i = is_inputSize; i; i--, buf += ad_nchans)
			*buf = 0;
	}
	return(IS_OK);
}

static int is_rnd2int(float f)
{
  if (f < 0)
    return((int)(f - 0.5));
  else
    return((int)(f + 0.5));
}

static void is_defaultFcSetFn(int adFc, int daFc, int evtFc)
{
	DEBUG(stderr, "iserver: Actual dafc=%d\n", daFc);
	DEBUG(stderr, "iserver: Actual adfc=%d\n", adFc);
#ifdef __drpmproc__
	DEBUG(stderr, "iserver: Actual evtfc=%d\n", evtFc);
#endif
}

static void is_defaultAlert(char *fmt, ...)
{
	va_list ap;
	char *p,*sval;
	int	ival;
	double	fval;

	va_start(ap, fmt);
	fprintf(stderr,"alert: ");
	for (p = fmt; *p; p++) {
		if (*p != '%') {
			fputc(*p, stderr);
			continue;
		}
		switch (*++p) {
			case 'd':
				ival = va_arg(ap, int);
				fprintf(stderr, "%d", ival);
				break;
			case 'f':
				fval = va_arg(ap, double);
				fprintf(stderr, "%f", fval);
				break;
			case 's':
				for (sval = va_arg(ap, char *); *sval; sval++)
					fputc(*sval, stderr);
				break;
			default:
				fputc(*p, stderr);
		}
	}
	fprintf(stderr,"\n");
	fflush(stderr);
	va_end(ap);
}

static void is_defaultNotify(char *fmt, ...)
{
	va_list ap;
	char *p,*sval;
	int	ival;
	double	fval;

	va_start(ap, fmt);
	fprintf(stderr,"notify: ");
	for (p = fmt; *p; p++) {
		if (*p != '%') {
			fputc(*p, stderr);
			continue;
		}
		switch (*++p) {
			case 'd':
				ival = va_arg(ap, int);
				fprintf(stderr, "%d", ival);
				break;
			case 'f':
				fval = va_arg(ap, double);
				fprintf(stderr, "%f", fval);
				break;
			case 's':
				for (sval = va_arg(ap, char *); *sval; sval++)
					fputc(*sval, stderr);
				break;
			default:
				fputc(*p, stderr);
		}
	}
	fprintf(stderr,"\n");
	fflush(stderr);
	va_end(ap);
}



#ifdef __drpmproc__
#endif /* __drpmproc__ */

#ifdef __drpmproc__
# include "is-drpm.c"
#endif /* __drpmrpoc__ */

#ifdef __tdtproc__
# include "is-tdt.c"
#endif /* __tdtproc__ */

