/*
**	This file is part of XDowl
**	Copyright (c) 1994 Jamie Mazer
**	California Institute of Technology
**	<mazer@asterix.cns.caltech.edu>
*/

/******************************************************************
**  RCSID: $Id: saudio.h,v 1.1 1995/06/05 22:02:17 cmalek Exp $
** Program: dowl
**  Module: saudio.h
**  Author: mazer
** Descrip: interface for saudio.c
**
** Revision History (most recent last)
**
*******************************************************************/


extern void sparc_audio_shutdown();
extern int sparc_audio_play_buffer();
extern int sparc_audio_click();

