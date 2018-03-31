/******************************************************************
**  RCSID: $Id: daq.h,v 1.5 2000/05/26 23:08:04 cmalek Exp $
** Program: iserver
**  Module: daq.h
**  Author: cmalek
** Descrip: Generic stuff for daq programs in iserver
**
*******************************************************************/

#ifndef _DAQ_H_
#define _DAQ_H_

/* -------------------------------------------------------------------
   Macros
   ------------------------------------------------------------------- */
#define DEBUG		if (debugflag) fprintf

#define IS_OK		1
#define IS_ERROR	0
#define IS_TRANS_OK		0
#define IS_TRANS_MISSED	1
#define IS_TRANS_FAIL	2

#define IS_LEFT		(1 << 0)
#define IS_RIGHT	(1 << 1)
#define IS_BOTH		(IS_LEFT | IS_RIGHT)
#define IS_NEITHER (IS_LEFT & IS_RIGHT)

/* -------------------------------------------------------------------
   Typedefs
   ------------------------------------------------------------------- */
#ifdef __tdtproc__
typedef struct tdt_control {
	float	att[2];
	int		channel;
	int		use_led;
	int		led_dur;
	int		led_delay;
	double	timestamp;
} TDTcontrol;
#endif /* __tdtproc__ */

#ifdef __drpmproc__
extern int sparc_audio_play_click(int);
#endif /* __drpmproc__ */

#endif /* _DAQ_H_ */

