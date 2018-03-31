/******************************************************************
**  RCSID: $Id: att.h,v 2.45 1998/11/24 19:56:45 cmalek Exp $
** Program: iserver
**  Module: att.h
**  Author: mazer
** Descrip: dig-atten controller interface file
**
** Revision History (most recent last)
**  
*******************************************************************/

#ifndef _ATT_H_
#define _ATT_H_

extern float attMaxAtten;
extern int setRack(int, float, float);
extern void getRack(double *, double *);

#endif /* _ATT_H_ */
