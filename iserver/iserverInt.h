/******************************************************************
**  RCSID: $Id: iserverInt.h,v 1.6 1998/11/24 19:56:45 cmalek Exp $
** Program: iserver
**  Module: iserverInt.h
**  Author: mazer, cmalek
** Descrip: Internal header file for iserver
**
*******************************************************************/

#ifndef _ISERVERINT_H_
#define _ISERVERINT_H_

/* -------------------------------------------------------------------
   Macros
   ------------------------------------------------------------------- */
#define IS_OK		1
#define IS_ERROR	0
#define IS_TRANS_OK		0
#define IS_TRANS_MISSED	1
#ifdef MATLAB
#define calloc mxCalloc
#endif /* MATLAB */

/* -------------------------------------------------------------------
   Variables
   ------------------------------------------------------------------- */
extern int da_nchans;
extern int ad_nchans;

extern xword *is_inputBuffer;
extern int is_inputSize;	    /* "sample-pairs" in is_inputBuffer */
extern xword *is_outputBuffer;
extern int is_outputSize;	    /* "sample-pairs" in is_outputBuffer */

#ifdef __tdtproc__
extern float *is_attBuffer;
extern int is_attSize;
extern unsigned long *is_spikeBuffer;
extern int is_spikeSize;
extern int is_useET1;
#endif /* __tdtproc__ */

extern FCSETFN is_fcSetFn;
extern MSGFN is_alert;
extern MSGFN is_notify;

#endif /* _ISERVERINT_H_ */


