/*
**	This file is part of XDowl
**	Copyright (c) 1994 Jamie Mazer
**	California Institute of Technology
**	<mazer@asterix.cns.caltech.edu>
*/

static char *rcsid =
  "@(#) $Id: reset_audio.c,v 1.1 1995/06/05 22:02:11 cmalek Exp $";

/******************************************************************
**  RCSID: $Id: reset_audio.c,v 1.1 1995/06/05 22:02:11 cmalek Exp $
** Program: dowl
**  Module: saudio.c
**  Author: mazer
** Descrip: for playing SHORT sounds on sparc internal speaker
**
** Revision History (most recent last)
**
** Tue Mar 17 19:09:02 1992 mazer
**  added sparc_audio_play_buffer() to play an in memory buffer
**
** Mon Oct 10 22:06:55 1994 mazer
**  integrated into drpmproc to provide sync pulses..
**
*******************************************************************/

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/stropts.h>
#include <sun/audioio.h>
#include <unistd.h>
#include <strings.h>

#include "/usr/demo/SOUND/multimedia/libaudio.h"
#include "/usr/demo/SOUND/multimedia/audio_device.h"
#include "/usr/demo/SOUND/multimedia/ulaw2linear.h"

#include "saudio.h"

extern int audio_read_filehdr(), audio_drain(); 
extern int ioctl(), close(), read(), write();
extern void perror();


main(ac, av)
     int ac;
     char **av;
{
  int		audio_fd;
  audio_info_t 	adata;

  if ((audio_fd = open("/dev/audio", O_WRONLY)) < 0) {
    if ((errno != EINTR) && (errno != EBUSY))
      perror("/dev/audio");
    return (0);
  }
  AUDIO_INITINFO(&adata);
  adata.play.port = AUDIO_SPEAKER;
  if (ioctl(audio_fd, AUDIO_SETINFO, &adata) < 0)
    perror("Set audio->speaker");
  close(audio_fd);
  exit(0);
}
