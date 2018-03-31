/******************************************************************
**  RCSID: $Id: saudio.c,v 1.2 1998/11/24 19:59:21 cmalek Exp $
** Program: iserver
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
#include <malloc.h>
#include <strings.h>

#include "/usr/demo/SOUND/multimedia/libaudio.h"
#include "/usr/demo/SOUND/multimedia/audio_device.h"
#include "/usr/demo/SOUND/multimedia/ulaw2linear.h"

#include "saudio.h"
#include "port.h"

extern int audio_read_filehdr(), audio_drain(); 
extern int ioctl(), close(), read(), write();
extern void perror();

static int audio_fd = -1;
static audio_info_t adata;
static unsigned save_audio_gain;
static unsigned save_audio_port;

void sparc_audio_shutdown()
{
  if (audio_fd >= 0) {
    audio_drain(audio_fd, 0);	/* block until audio's done, non-int'ble */
    adata.play.port = save_audio_port;
    adata.play.gain = save_audio_gain;
    if (ioctl(audio_fd, AUDIO_SETINFO, &adata) < 0)
      perror("Restore audio params");
    close(audio_fd);
    audio_fd = -1;
  }
}
  
int sparc_audio_play_buffer(buffer, nsamps, gain)
     unsigned char *buffer;
     int nsamps;
     unsigned gain;
{
  if (audio_fd >= 0)
    sparc_audio_shutdown();

  if ((audio_fd = open("/dev/audio", O_WRONLY)) < 0) {
    if ((errno != EINTR) && (errno != EBUSY))
      perror("/dev/audio");
    return (0);
  }

  AUDIO_INITINFO(&adata);
  if (ioctl(audio_fd, AUDIO_GETINFO, &adata) < 0)
    perror("Read audio params");

  save_audio_gain = adata.play.gain;
  save_audio_port = adata.play.port;

  adata.play.gain = gain;
  adata.play.port = AUDIO_HEADPHONE;

  if (ioctl(audio_fd, AUDIO_SETINFO, &adata) < 0)
    perror("Set audio params");

  if (write(audio_fd, buffer, nsamps) < 0) {
    perror("audio write");
    close(audio_fd);
    return (0);
  }
  audio_drain(audio_fd, 0);
  return(1);
}

int sparc_audio_play_click(ms_dur)
     int ms_dur;
{
  int i;
  int nsamps = 8000 * ms_dur / 1000;
  unsigned char *buf;

  if (nsamps < 3)
    nsamps = 3;

  buf = (unsigned char *) calloc(nsamps * 4, sizeof(unsigned char));

  buf[0] = 0x0;
  for (i = 1; i < nsamps; i++)
    buf[i] = 0x80;
  for (--i; i < nsamps * 4; i++)
    buf[i] = 0x0;
  i = sparc_audio_play_buffer(buf, nsamps * 4, (unsigned)100);
  free(buf);
  return(i);
}
