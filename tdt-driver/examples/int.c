
static char *rcsid = "@(#)$Id: int.c,v 1.1 1996/12/10 23:41:27 cmalek Exp $";
static char *rcsrev = "$Revision: 1.1 $";
/* -------------------------------------------------------------------------
   RCSID: $Id: int.c,v 1.1 1996/12/10 23:41:27 cmalek Exp $ 
   Filename:  		   dev_coils.c
   Author:			   cmalek
   Description:        Device driver for John Power's Hemholtz coil system. 
   Notes:			   make device special file with :

							mknod /dev/coil c 31 0

						    (major # 31, minor # 0)
 
   Bjorn Ekwall added ugly typecast to avoid the dreaded "__moddi3" message...
   ------------------------------------------------------------------------- */


/* Kernel Includes */
#include <linux/config.h>
#ifdef MODULE
#include <linux/module.h>
#include <linux/version.h> 
#endif
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/major.h>
#include <asm/segment.h>
#include <linux/kernel.h> 
#include <linux/signal.h>
#include <asm/io.h>
#include <linux/delay.h>

/* Defines */
#define FUNCTION
#define MAX_RET_STRING			32
#define POWERS_CARD_PORT		0x300

/* External function prototypes */
extern int printk(const char* fmt, ...);

/* nice and high, but it is tunable: insmod drv_coils major=30 */
static int major = 31;

/* -------------------------------------------------------------------------
   coil_read:		Function called when device is read: reads azimuth and 
   					elevation from the coils, and returns string containing 
   					azimuth and elevation.
   ------------------------------------------------------------------------- */

FUNCTION static int coil_read(
	struct inode * node,
	struct file * file,
	char * buf,
	int count)
	{
	static char 	coil_reply[MAX_RET_STRING];
    short			reading = 0;
    unsigned char	readinglo; 
    unsigned char	readinghi; 
    int				azimuth = 0;
    int 			elevation = 0;
	
	int 			left;
	int 			coil_pos;
	int				i;

	/* read azimuth */
	outw(1, POWERS_CARD_PORT);
	reading = inw(POWERS_CARD_PORT);
	readinghi = (reading & 0x03); 
	readinglo = (reading & 0xff00) >> 8; 
	azimuth = ((float) readinghi) * 90.0 + ((float) readinglo) /2.0;

	/* read elevation */
	outw(0, POWERS_CARD_PORT);
	reading = inw(POWERS_CARD_PORT);
	readinghi = (reading & 0x03);  
	readinglo = (reading & 0xfe00) >> 8;
	elevation = ((float) readinghi) * 90.0 + ((float) readinglo)/2.0;

	sprintf(coil_reply, "%d %d\n", azimuth, elevation);
					
	coil_pos = ((int)file->f_pos) % (sizeof(coil_reply) - 1);
		   /* ^^^^^ Sorry (bj0rn faked this, but it works for f_pos < 2^32) */

	for (left = count; left > 0; left--) 
		{
		put_fs_byte(coil_reply[coil_pos++], buf);
		buf++;
		if (coil_pos == MAX_RET_STRING - 1)
			coil_pos = 0;
		}
	file->f_pos+=count;

	for (i=0; i<1000;i++) 
		outw(0, 0x80);
	/* udelay((unsigned long) 10000); */

	return count;
	}

/* -------------------------------------------------------------------------
   Support seeks on the device 
   ------------------------------------------------------------------------- */
 
FUNCTION static int coil_lseek(
	struct inode * inode, 
	struct file * file, 
	off_t offset, 
	int orig)
	{
	switch (orig) 
		{
		case 0:
			file->f_pos = offset;
			return file->f_pos;
		case 1:
			file->f_pos += offset;
			return file->f_pos;

		default: /* how does the device have an end? */
			return -EINVAL;
		}
	}


/* -------------------------------------------------------------------------
   coil_open:		Function called when device is opened: MOD_INC_USE_COUNT 
   					makes sure that the driver memory is not freed while the 
   					device is in use.
   ------------------------------------------------------------------------- */

FUNCTION static int coil_open( 
	struct inode* ino, 
	struct file* filep)
	{
	MOD_INC_USE_COUNT;
	return 0;   
	printk("Coil driver delay is 10000.\n");
	}

/* -------------------------------------------------------------------------
   coil_open:		Function called when device is closed: MOD_DEC_USE_COUNT 
   					so that driver memory can be freed. 
   ------------------------------------------------------------------------- */
FUNCTION static void coil_close( 
	struct inode* ino, 
	struct file* filep)
	{
	MOD_DEC_USE_COUNT;
	}

/* -------------------------------------------------------------------------
   File operations supported by this driver:
   ------------------------------------------------------------------------- */

static struct file_operations coil_fops = {
	coil_lseek,
	coil_read,
	NULL,
	NULL,		/* coil_readdir */
	NULL,		/* coil_select */
	NULL,		/* coil_ioctl */
	NULL,
	coil_open,
	coil_close,
	NULL		/* fsync */
};


/* -----------------------------------------------------------------
   Kernel interface code
   ----------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

FUNCTION int init_module(void)
	{
	printk( "drv_coils.c:  init_module called\n");

	if (register_chrdev(major, "hw", &coil_fops))
		{
		printk("register_chrdev failed: goodbye world :-(\n");
		return -EIO;
		}
	else
		printk("Hemholtz Coil Driver %s installed.\n", rcsrev);

	return 0;
	}

FUNCTION void cleanup_module(void)
	{
	printk("drv_coils.c:  cleanup_module called\n");

	if (MOD_IN_USE)
		printk("hw: device busy, remove delayed\n");

	if (unregister_chrdev(major, "hw") != 0)
		printk("cleanup_module failed\n");
	else
		printk("cleanup_module succeeded\n");
	}

#ifdef __cplusplus
}
#endif
