#ifndef __dos_h
#define __dos_h

#include <unistd.h>
#include <asm/io.h>

#define outport(port, word) 	outw((word), (port))
#define outportb(port, byte)	outb((byte), (port))
#define inport(port)		inw((port))
#define inportb(port)		inb((port))

/* ifndef XBDRV_RS232 */
#define io_init(base)					\
{							\
  if (ioperm (base, 0x19, 1) || ioperm(0x80,1, 1)) {	\
    fprintf (stderr, "\n\nMust be run as root!\n");	\
    exit (1);						\
  }							\
}
#define io_disable(base)					\
{							\
  if (ioperm (base, 0x19, 0) || ioperm(0x80,1, 0)) {	\
    fprintf (stderr, "\n\nMust be run as root!\n");	\
    exit (1);						\
  }							\
}
/* endif  */
#endif /* __dos_h */
