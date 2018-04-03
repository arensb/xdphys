/*
**	Th file is part of XDowl
**	Copyright (c) 1994 Jamie Mazer
**	California Institute of Technology
**	<mazer@asterix.cns.caltech.edu>
*/

/******************************************************************
**  RCSID: $Id: iserver.h,v 2.55 2001/01/04 22:06:52 cmalek Exp $
** Program: xdphys
**  Module: iserver.h
**  Author: mazer
** Descrip: interface file for iserver.c
**
** Revision History (most recent last)
**
** Tue Oct 13 12:55:55 1992 mazer
**  stripped out synth stuff..
**
** Tue Feb  9 23:12:21 1993 mazer
**  stripped out all the is_getRaster() stuff
**
*******************************************************************/

#ifndef _ISERVER_H_
#define _ISERVER_H_

#include "daq.h"

/* ------------------------------------------------------------------ 
   Public Macros
   ------------------------------------------------------------------ */
#define iserver_started() (is_inited)

#define IS_LAST (-2.0)		/* is-drpm only right now */
#define IS_BEST (-1.0)		/* use best sampling freq */
#define IS_NONE (0.0)		/* don't sample */

#define X_DA	(1 << 0)
#define X_AD	(1 << 1)
#define X_LOADONLY (1 << 2)
#define X_EVT	(1 << 3)
#define X_TRIG	(1 << 4)
#define X_LEFT (1 << 5)
#define X_RIGHT (1 << 6)
#define X_RECONLY (1 << 7)

#ifdef __tdtproc__
#define AD_MAX_NCHANS 4
#define DA_MAX_NCHANS 2
#endif /* __tdtproc__ */

/* -------------------------------------------------------------------
   Typedefs
   ------------------------------------------------------------------- */
typedef int (*IDLEFN)(void);
typedef void (*FCSETFN)(int,int,int);
typedef void (*MSGFN)(char *, ...);
typedef int (*YESNOFN)(char *, char *, char *, char *);

/* ------------------------------------------------------------------ 
   Public Variables
   ------------------------------------------------------------------ */
extern int 	is_testmode;

extern int	is_inited;

extern int 	is_daFc;
extern int	is_adFc;
#ifdef __drpmproc__
extern int	is_evtFc;
#endif /* __drpmproc__ */
#ifdef DRPMPROC
extern int	is_evtFc;
#endif /* DRPMPROC */

extern int	is_syncPulse;

extern int  is_report_errors;


/* ------------------------------------------------------------------ 
   Public Functions
   ------------------------------------------------------------------ */
extern int is_init(float,float,float,int,int);
extern int is_load(int);
extern int is_sample(int, int, double *);
extern int is_record(void);
extern int is_shutdown(void);

#if(0)
extern double is_getRad(void);
extern double is_getRadWithArgs(double,double);
#endif
extern int is_getDAnchans(void);
extern int is_getADnchans(void);
extern xword *is_getDAbuffer(int*);
extern xword *is_getADbuffer(int*);
extern int is_getDAmslength(void);
extern int is_getADmslength(void);
extern int is_clearOutput(int);
extern int is_clearInput(int);

extern void is_setFcSetFn(FCSETFN);
extern void is_setYesNoFn(YESNOFN);
extern void is_setAlertFn(MSGFN);
extern void is_setNotifyFn(MSGFN);

#ifdef __tdtproc__
extern unsigned long *is_getSpikebuffer(int *);
extern void    is_clearSpikes(void);
extern void    is_setAtten(float, float);
extern void	is_led_enable(void);
extern void	is_led_disable(void);
extern void	is_setLedDur(int);
extern void	is_setLedDelay(int);
extern void	is_et1_enable(void);
extern void	is_et1_disable(void);
#endif /* __tdtproc__ */

#ifdef __drpmproc__
extern void isdrpm_set_ppDelays(int, int);
#endif

void is_setNice(int);
int	is_getNice(void);
void is_setSyncPulse(int);
int	is_getSyncPulse(void);
void is_setMaxSpikes(int);
int is_getMaxSpikes(void);

#ifdef __test__
void is_setUseAtten(int);
int	is_getUseAtten(void);
void is_setUseEq(int);
int	is_getUseEq(void);
#endif /* __test__ */

#endif /* _ISERVER_H_ */
