#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <dos.h>


#ifdef MSC
#include <malloc.h>
#include <conio.h>
#include "xbdrv.h"
#define outportb(a,b) _outp(a,b)
#define outport(a,b) _outpw(a,b)
#define inportb(a) _inp(a)
#define inport(a) _inpw(a)
#define enable() _enable()
#define disable() _disable()
#else /* MSC */

#ifdef __linux__

#define XBDRV_SCHED

#include <dos.h>
/* system.h has the priority scheduling stuff in it */
#include <sched.h>
#include <sys/mman.h>
//#include <asm/system.h>
#include <assert.h>
#include "xbdrv.h"


#define FP_OFF(fp)      ((unsigned short)((unsigned long)fp & 0xffff))
#define FP_SEG(fp)      ((unsigned short)((unsigned long)(fp) >> 16))
#define _MK_FP(seg,ofs) ((void *) \
			(((unsigned long)(seg) << 16)|(unsigned short)(ofs)))

#define disable() ;
#define enable() ;

#ifdef XBDRV_SCHED
static unsigned long ET1read32_int(qint);
static qint ET1read16_int(qint);
static long ET1report_int(qint din);
static qint SD1count_int(qint din);
static void set_sched_priority(int, int);
static void check_sched_params(int, int);
static int get_valid_priority(int, int);
static void lock_memory(void);
static void unlock_memory(void);
static qint PI2gettime_int( qint din, qint bitn);
#endif /* XBDRV_SCHED */

/* ----------------------------------------------------------------------- */
#else /* __linux__ */

#include <alloc.h>
#include <io.h>
#include "xbdrv.h"
unsigned char inportb(unsigned int addr);
unsigned int  inport(unsigned int addr);
void outportb(unsigned int addr, unsigned char val);
void outport(unsigned int addr, unsigned int val);


#define FP_OFF(fp)      ((unsigned)(fp))
#define FP_SEG(fp)      ((unsigned)((unsigned long)(fp) >> 16))
#define MK_FP(seg,ofs)  ((void far *) \
			   (((unsigned long)(seg) << 16) | (unsigned)(ofs)))

#endif /* __linux__ */

#endif /* MSC */

#define DEBUG if (xbdrv_debug) fprintf

qint cc;
qint cpa[2];
qint xln,xbsel;
unsigned char cmd[100];
char *xcaller;
unsigned char xbcode[XB_MAX_DEV_TYPES+1][XB_MAX_OF_EACH_TYPE+1][2];
xbstring xbname[XB_MAX_DEV_TYPES+1];
long xbtimeout;
qint xb_eflag,xb_emode;
char xb_err[10][50];
qint dadev,addev;
int  xbdrv_debug = 0;
int  xbdelay = 200;


#ifdef __linux__
/********************************************************************
  Linux specific routines
 ********************************************************************/
#ifdef XBDRV_SCHED
/* ------------------------------------------------------------------------
   ET1readall32
	
       The <times> array will be allocated or resized to hold the number
	   of times returned from the ET1.

   ------------------------------------------------------------------------ */

int ET1readall32(
	qint	din,
	unsigned long	**times)
{
	int	ntimes;
	int	i;

	set_sched_priority(SCHED_FIFO, 98);

	ntimes = ET1report_int(din);

	if (ntimes == 0) {
		DEBUG(stderr, "  xbdrv: ET1readall32, ntimes=0\n");
		fflush(stderr);
		if (*times != NULL) {
			free(*times);
			*times = NULL;
		}
	} else {
		DEBUG(stderr, "  xbdrv: ET1readall32, ntimes=%d\n", ntimes);

		if (*times == NULL) {
			*times = (unsigned long *) calloc(ntimes, sizeof(unsigned long));
		} else {
			*times = (unsigned long *) realloc(*times, 
				ntimes * sizeof(unsigned long));
		}
		assert(*times != NULL);

		/* The following is some hocus-pocus to convince
		   mlockall() that we want this memory */

		for(i=0; i<ntimes; i++)
			(*times)[i] = 0;

		lock_memory(); 

		if (ntimes > 0) {
			DEBUG(stderr, "  xbdrv: ET1readall32, getting spike times\n");
			for (i=0; i<ntimes; i++) 
				(*times)[i] = ET1read32_int(din);
			DEBUG(stderr, "  xbdrv: ET1readll32, done reading spike times\n");
		}

		unlock_memory(); 
	}

	ET1clear(din);

	set_sched_priority(SCHED_OTHER, 0);

	return(ntimes);
}

/* ------------------------------------------------------------------------
   ET1readall16

       The <times> array will be allocated or resized to hold the number
	   of times returned from the ET1.

   ------------------------------------------------------------------------ */

int ET1readall16(
	qint	din,
	qint	**times)
{
	int	ntimes;
	int i;

	assert(times != NULL);

	set_sched_priority(SCHED_FIFO, 98);

	ntimes = ET1report_int(din);

	if (ntimes == 0) {
		DEBUG(stderr, "  xbdrv: ET1readall16, ntimes=0\n");
		fflush(stderr);
		if (*times != NULL) {
			free(*times);
			*times = NULL;
		}
	} else {
		DEBUG(stderr, "  xbdrv: ET1readall16, ntimes=%d\n", ntimes);

		if (*times == NULL) {
			*times = (qint *) calloc(ntimes, sizeof(qint));
		} else {
			*times = (qint *) realloc(*times, ntimes * sizeof(qint));
		}
		assert(*times != NULL);

		/* The following is some hocus-pocus to convince
		   mlockall() that we want this memory */

		for(i=0; i<ntimes; i++)
			(*times)[i] = 0;

		lock_memory();

		if (ntimes > 0) {
			for (i=0; i<ntimes; i++) 
				(*times)[i] = ET1read16_int(din);
			DEBUG(stderr, "  xbdrv: ET1readll16, done reading spike times\n");
		}

		unlock_memory();
	}

	ET1clear(din);

	set_sched_priority(SCHED_OTHER, 0);
	return(ntimes);
}


/* ------------------------------------------------------------------------
   PI2gettime

       This is just a wrapper for PI2gettime_int() (used to be PI2gettime),
	   which uses the priority scheduling calls.

   ------------------------------------------------------------------------ */

qint PI2gettime( qint din, qint bitn)
{
	qint	val;

#ifdef _POSIX_PRIORITY_SCHEDULING
	set_sched_priority(SCHED_FIFO, 98);
#endif /* _POSIX_PRIORITY_SCHEDULING */

	val = PI2gettime_int(din, bitn);

#ifdef _POSIX_PRIORITY_SCHEDULING
	set_sched_priority(SCHED_OTHER, 0);
#endif /* _POSIX_PRIORITY_SCHEDULING */

	return(val);
}

/* ------------------------------------------------------------------------
   SD1count

       This is just a wrapper for SD1count_int() (used to be SD1count),
	   which uses the priority scheduling calls.

   ------------------------------------------------------------------------ */

qint SD1count(qint din)
{
	qint	count;

#ifdef _POSIX_PRIORITY_SCHEDULING
	set_sched_priority(SCHED_FIFO, 98);
#endif /* _POSIX_PRIORITY_SCHEDULING */

	count = SD1count_int(din);

#ifdef _POSIX_PRIORITY_SCHEDULING
	set_sched_priority(SCHED_OTHER, 0);
#endif /* _POSIX_PRIORITY_SCHEDULING */

	return(count);
}

/* ------------------------------------------------------------------------
   ET1report

       This is just a wrapper for ET1report_int() (used to be ET1report),
	   which uses the priority scheduling calls.

   ------------------------------------------------------------------------ */

long ET1report(qint din)
{
	long	ntimes;

#ifdef _POSIX_PRIORITY_SCHEDULING
	set_sched_priority(SCHED_FIFO, 98);
#endif /* _POSIX_PRIORITY_SCHEDULING */

	ntimes = ET1report_int(din);

#ifdef _POSIX_PRIORITY_SCHEDULING
	set_sched_priority(SCHED_OTHER, 0);
#endif /* _POSIX_PRIORITY_SCHEDULING */

	return(ntimes);
}

/* ------------------------------------------------------------------------
   ET1read32

       This is just a wrapper for ET1read32_int() (used to be ET1read32),
	   which uses the priority scheduling calls.

   ------------------------------------------------------------------------ */

unsigned long ET1read32(qint din)
{
	unsigned long	time;

#ifdef _POSIX_PRIORITY_SCHEDULING
	set_sched_priority(SCHED_FIFO, 98);
#endif /* _POSIX_PRIORITY_SCHEDULING */

	time = ET1read32_int(din);

#ifdef _POSIX_PRIORITY_SCHEDULING
	set_sched_priority(SCHED_OTHER, 0);
#endif /* _POSIX_PRIORITY_SCHEDULING */

	return(time);
}

/* ------------------------------------------------------------------------
   ET1read16

       This is just a wrapper for ET1read16_int() (used to be ET1read16),
	   which uses the priority scheduling calls.

   ------------------------------------------------------------------------ */

qint ET1read16(qint din)
{
	qint	time;

#ifdef _POSIX_PRIORITY_SCHEDULING
	set_sched_priority(SCHED_FIFO, 98);
#endif /* _POSIX_PRIORITY_SCHEDULING */

	time = ET1read16_int(din);

#ifdef _POSIX_PRIORITY_SCHEDULING
	set_sched_priority(SCHED_OTHER, 0);
#endif /* _POSIX_PRIORITY_SCHEDULING */

	return(time);
}


/* ------------------------------------------------------------------------
	lock_memory
   ------------------------------------------------------------------------ */
static void lock_memory(void)
{
#ifdef _POSIX_MEMLOCK
	if (mlockall(MCL_CURRENT | MCL_FUTURE)) {
		perror("  xbrdv: mlockall(MCL_CURRENT | MCL_FUTURE)");
	}
#else
	printf("  xbdrv: Warning: memory locking not available, get a new kernel!\n");
#endif
}

/* ------------------------------------------------------------------------
	unlock_memory
   ------------------------------------------------------------------------ */
static void unlock_memory(void)
{
#ifdef _POSIX_MEMLOCK
	if (munlockall()) {
		perror("  xbrdv: munlockall(MCL_CURRENT | MCL_FUTURE)");
	}
#else
	printf("  xbdrv: Warning: memory locking not available, get a new kernel!\n");
#endif
}

/* ------------------------------------------------------------------------
	set_sched_priority
   ------------------------------------------------------------------------ */
static void set_sched_priority(
	int		policy,
	int		priority)
{
#ifdef _POSIX_PRIORITY_SCHEDULING
	struct sched_param prio;
#endif

#ifdef _POSIX_PRIORITY_SCHEDULING

	prio.sched_priority = get_valid_priority(policy, priority);

	DEBUG(stderr, "Trying to set priority to %d.\n", prio.sched_priority);

	if (sched_setscheduler(getpid(), policy, &prio)) {
		perror("  xbdrv: sched_setscheduler");
		exit(1);
	}

	check_sched_params(policy, priority);

#else
	DEBUG(stderr, 
		"  xbdrv: WARNING: real-time scheduling not available,"
		" get a new kernel!\n");
#endif
}


#ifdef _POSIX_PRIORITY_SCHEDULING
/* ------------------------------------------------------------------------
	validate_priority
   ------------------------------------------------------------------------ */
static int get_valid_priority(
	int	policy,
	int priority)
{
	int	  minp,maxp;

	minp = sched_get_priority_min(policy);
	if (minp < 0) {
		perror("  xbdrv: sched_get_priority_min");
		exit(1);
	}
	maxp = sched_get_priority_max(policy);
	if (maxp < 0) {
		perror("  xbdrv: sched_get_priority_max");
		exit(1);
	}

	switch (policy) {
		case SCHED_FIFO: 
			DEBUG(stderr, 
				"  xbdrv: SCHED_FIFO priority range: (%d ... %d)\n", 
				minp, maxp);
			break;
		case SCHED_RR: 
			DEBUG(stderr, 
				"  xbdrv: SCHED_RR priority range: (%d ... %d)\n", 
				minp, maxp);
			break;
		case SCHED_OTHER: 
			DEBUG(stderr, 
				"  xbdrv: SCHED_OTHER priority range: (%d ... %d)\n", 
				minp, maxp);
			break;
	}

	if (priority < minp)
		priority = minp;
	if (priority > maxp)
		priority = maxp-1;

	return(priority);
}

/* ------------------------------------------------------------------------
	check_sched_params
   ------------------------------------------------------------------------ */
static void check_sched_params(
	int	policy,
	int priority)
{
	int   retval;
	struct sched_param prio;

	prio.sched_priority = 4711;

	if (sched_getparam(0, &prio) < 0)
		perror("  xbdrv: sched_getparam(0, ...)");
	else {
		if (prio.sched_priority != priority) {
			DEBUG(stderr, "  xbdrv: couldn't set priority to %d\n", 
				priority);
		}
		DEBUG(stderr, "  xbdrv: priority is %d.\n", 
			prio.sched_priority);
	}

	retval = sched_getscheduler(0);
	if (retval < 0)
		perror("  xbdrv: sched_getscheduler");
	else {
		switch (retval) {
			case SCHED_FIFO:
				DEBUG(stderr, 
					"  xbdrv: scheduler is SCHED_FIFO.\n");
				break;
			case SCHED_RR:
				DEBUG(stderr, 
					"  xbdrv: scheduler is SCHED_RR.\n");
				break;
			case SCHED_OTHER:
				DEBUG(stderr, 
					"  xbdrv: scheduler is SCHED_OTHER.\n");
				break;
		}
	}
}
#endif /* _POSIX_PRIORITY_SCHEDULING */
#endif /* XBDRV_SCHED */


#endif /* __linux__ */


void tdtdisab( void)
{
  if (!xb_eflag)
    disable();
}

/********************************************************************
  Physical Drivers.
 ********************************************************************/
void iosend(qint datum)
{
    long ii;

    if (!xb_eflag)
	{
	    if (cc<COM_APa)
		{
		    ii=0;
		    do
			{
			    ii++;
			    if (ii>xbtimeout)
				{
				    showerr("Transmission Time-Out on COM Port.");
				    break;
				}
			}
		    while(!(inportb(cpa[xbsel]+5) & 0x20));
		    outportb(cpa[xbsel],datum);
		}
	    else
		{
		    ii=0;
		    do
			{
			    ii++;
			    if (ii>xbtimeout)
				{
				    showerr("Transmission Time-Out on COM Port.");
				    break;
				}
			}
		    while(!(inportb(cpa[xbsel]) & 0x02));
		    outportb(cpa[xbsel]+1,datum);
		}
	}
}


qint iorec( void )
{
  if (xb_eflag)
    return(0);

  if (cc<COM_APa)
  {
    if (inportb(cpa[xbsel]+5) & 0x01)
    {
      return(inportb(cpa[xbsel]));
    }
    else
    {
      return(-1);
    }
  }
  else
  {
    if (inportb(cpa[xbsel]) & 0x01)
    {
			return(inportb(cpa[xbsel]+1));
    }
    else
    {
      return(-1);
    }
  }
}


/********************************************************************
  Error Handling procedures.
 ********************************************************************/
void showerr( char eee[])
{
  qint rn,pn,i;

  enable();
  rn = xln >> 2;
  pn = (xln & 3) + 1;
  sprintf(xb_err[0]," ");
  sprintf(xb_err[1],"XBUS Error:    Rack(%d),  Position(%d).",rn,pn);
  sprintf(xb_err[2],"	 %s",xcaller);
  sprintf(xb_err[3],"	 %s",eee);
  sprintf(xb_err[4]," ");
  sprintf(xb_err[5]," ");
  xb_eflag = 1;

#ifndef WINDOWS
  if (!xb_emode)
  {
    i = 0;
    do
    {
			printf("%s \n",xb_err[i]);
      i++;
    }while(xb_err[i][0]!=0);
    exit(0);
  }
#endif

}


void processerr( qint li, qint ne)
{
  long x;
  qint i,j;
  char sss[100];

  switch(li)
  {
    case ARB_ERR:
    {
      if (ne>XB_MAX_TRYS)
      {
	XB1flush();
	showerr("Communication errors.");
	break;
      }
      else
      {
	XB1flush();
      }
      break;
    }

    case SLAVE_ERR:
		{
      tdtdisab();
      j=0;
      x=0;
      do
      {
	i=iorec();
	if (i>0)
	{
	  sss[j]=(i);
    j++;
    x=0;
  }
  x++;
      }while(i!=0 && j<200 && x<xbtimeout);
      sss[j]=0;
      enable();
      showerr(sss);
      break;
    }

    default:
    {
      if (ne>XB_MAX_TRYS)
	showerr("No XBUS device resonding at XBL accessed.");
      break;
    }
  }
}


/********************************************************************
  Form Transfer Procedures.
 ********************************************************************/
void shortform(qint val)
{
  qint i,j;
  long k;

  if (!xb_eflag)
  {
    j=0;
    do
    {
      cmd[0] = val;
      j++;
      iosend(xln & 0x7f);
      iosend(val);
      k=0;
      do
      {
  i=iorec();
  k++;
      }while(i<0 && k < xbtimeout);

      if (i!=SLAVE_ACK)
  processerr(i,j);

    }while(i!=SLAVE_ACK && j<=XB_MAX_TRYS && !xb_eflag);
  }
}


void standform( unsigned char bufp[], qint n)
{
	qint i,j,x,cs;
	long k;

	if (!xb_eflag)
	{
		if (n>63)
		{
      showerr("Too many for Stand-Form.");
    }
    else
    {
      j=0;
      n++;
      do
      {
  j++;
  cs = 0;
  iosend(xln & 0x7f);
  iosend(0x40 | n);
  for(x=0; x<(n-1); x++)
  {
    iosend(bufp[x]);
    cs+=(qint)bufp[x];
  }
  iosend(cs);

  k=0;
  do
  {
    i=iorec();
    k++;
  }while(i<0 && k < xbtimeout);

  if (i!=SLAVE_ACK)
    processerr(i,j);

      }while(i!=SLAVE_ACK && j<=XB_MAX_TRYS && !xb_eflag);
		}
  }
}


/********************************************************************
  Data Receive Procedures.
 ********************************************************************/
unsigned long get32(void)
{
  qint i,bn;
  unsigned long k;
  unsigned long *rv;
  unsigned char x[4];

  if (xb_eflag)
    return(0);

  rv = (unsigned long *)x;
  k=0;
  for(bn=0; bn<4; bn++)
  {
    do
    {
      i=iorec();
      k++;
    }while(i<0 && k < xbtimeout);
    if (i>=0)
    {
      x[bn] = i;
      k=0;
    }
    else
    {
			showerr("Device not resp. after SLAVE_ACK.");
      *rv = 0;
		}
  }
  return(*rv);
}


qint get16(void)
{
  qint i,bn;
  long k;
  qint *rv;
  unsigned char x[2];

  if (xb_eflag)
    return(0);

  rv = (qint *)x;
  k=0;
  for(bn=0; bn<2; bn++)
  {
    do
    {
      i=iorec();
      k++;
    }while(i<0 && k < xbtimeout);
    if (i>=0)
    {
      x[bn] = i;
      k=0;
    }
    else
    {
			showerr("Device not resp. after SLAVE_ACK.");
      *rv = 0;
		}
  }
  return(*rv);
}

unsigned char get8(void)
{
  qint i;
  long k;

  if (xb_eflag)
    return(0);

  k=0;
  do
  {
    i=iorec();
    k++;
  }while(i<0 && k < xbtimeout);
  if (i<0)
  {
    showerr("Device not resp. after SLAVE_ACK.");
    i=0;
  }
  return((unsigned char)i);
}


void getxln(qint din, qint dtype, char descrip[])
{
  xln=xbcode[dtype][din][xbsel];
  xcaller = (char *)descrip;
	if (!xln && !xb_eflag)
  {
		showerr("  Non-present device referenced.");
  }
}


qint XB1init( qint cptr )
{
  char *s;
  qint i,j,k,rn,pn,flag;
  int af;

  if (getenv("XBDRV_DEBUG") != NULL)
	  xbdrv_debug = 1;
  else
	  xbdrv_debug = 0;

  dadev=addev=0;

  xb_eflag = 0;
  xb_emode = 0;

  xbsel = 0;

  if (cptr>0)
  {
    cc = cptr;
    if (cptr == 0x311)
      cc=COM_APa;
    if (cptr == 0x312)
      cc=COM_APb;
  }
  else
  {
    cc = COM_APa;
    s = getenv("XBCOM");
    if (s!=NULL) {
		if (!strcmp(s,"COM1") || !strcmp(s,"com1"))
			cc = COM1;
		if (!strcmp(s,"COM2") || !strcmp(s,"com2"))
			cc = COM2;
		if (!strcmp(s,"COM3") || !strcmp(s,"com3"))
			cc = COM3;
		if (!strcmp(s,"COM4") || !strcmp(s,"com4"))
			cc = COM4;
		if (!strcmp(s,"APb") || !strcmp(s,"apb"))
			cc = COM_APb;
	}
  }

  if (cc==COM_APb)
    xbsel = 1;

  s = getenv("XBDELAY");
  if (s!=NULL) {
    sscanf(s,"%d",&xbdelay);
    if (xbdelay<1) xbdelay = 1;
	printf("s= %s, XBDELAY = %d\n", s, xbdelay);
  }

  af = 100;

  s = getenv("SPFACT");
  if (s!=NULL) {
    sscanf(s,"%d",&af);
    if (af<1) af = 1;
    if (af>1000) af = 1000;
	printf("s=%s, SPFACT = %d\n", s, af);
  }

  xbtimeout = (long)XB_TIMEOUT * af;

  switch(cc)
  {
    case COM1:
	  cpa[xbsel] = COM1base;
	  break;
    case COM2:
	  cpa[xbsel] = COM2base;
	  break;
		case COM3:
	  cpa[xbsel] = COM3base;
					break;
    case COM4:
	  cpa[xbsel] = COM4base;
	  break;
    case COM_APa:
	  cpa[xbsel] = 0x230;
	  break;
    case COM_APb:
	  cpa[xbsel] = 0x250;
  }

  io_init(cpa[xbsel]);

  for(i=0; i<20; i++)
    iosend(ERR_ACK);
  for(i=0; i<20; i++)
    iosend(SNOP);

  i=1000;
  do
  {
    i--;
  }while(iorec()>=0 && i>0);

  if (i<=0)
  {
    return(0);
  }
  else
  {
    for(i=0; i<XB_MAX_DEV_TYPES; i++)
      for(j=0; j<XB_MAX_OF_EACH_TYPE; j++)
	xbcode[i][j][xbsel]=0;

		for(i=0; i<XB_MAX_DEV_TYPES; i++)
      strcpy(xbname[i],"NU ");

    strcpy(xbname[PA4_CODE],"PA4");
    strcpy(xbname[SW2_CODE],"SW2");
    strcpy(xbname[CG1_CODE],"CG1");
    strcpy(xbname[SD1_CODE],"SD1");
    strcpy(xbname[ET1_CODE],"ET1");
    strcpy(xbname[PI1_CODE],"PI1");
    strcpy(xbname[UI1_CODE],"UI1");
    strcpy(xbname[WG1_CODE],"WG1");
    strcpy(xbname[PF1_CODE],"PF1");
    strcpy(xbname[TG6_CODE],"TG6");
    strcpy(xbname[PI2_CODE],"PI2");
    strcpy(xbname[WG2_CODE],"WG2");
/*    strcpy(xbname[XXX_CODE],"XXX");  */
    strcpy(xbname[SS1_CODE],"SS1");
    strcpy(xbname[PM1_CODE],"PM1");
    strcpy(xbname[HTI_CODE],"HTI");

    strcpy(xbname[DA1_CODE],"DA1");
    strcpy(xbname[AD1_CODE],"AD1");
    strcpy(xbname[DD1_CODE],"DD1");
    strcpy(xbname[AD2_CODE],"AD2");
    strcpy(xbname[AD3_CODE],"AD3");
    strcpy(xbname[DA3_CODE],"DA3");
    strcpy(xbname[PD1_CODE],"PD1");

    rn=1;
    do
    {
      flag=0;
      for(pn=0; pn<4; pn++)
      {
	iosend((rn << 2) + pn);
	iosend(IDENT_REQUEST);
	for(i=0; i<XB_WAIT_FOR_ID; i++)
	  iosend(SNOP);
				j = iorec();
	if (j>0 && j<=XB_MAX_DEV_TYPES)
				{
	  flag=1;
	  k=1;
	  do
	  {
	    if (xbcode[j][k][xbsel])
	      k++;
	    else
	    {
	      xbcode[j][k][xbsel] = (rn << 2) + pn;
	      k=1000;
	    }
	  }while(k<=XB_MAX_OF_EACH_TYPE);

	  if (k!=1000)
	    showerr("Too many devices of single a type.");
	}
      }
      rn++;
    }while(rn<=XB_MAX_NUM_RACKS && flag);
    return(1);
  }
}


void XB1flush( void )
{
  qint i;

  for(i=0; i<20; i++)
    iosend(ERR_ACK);
  for(i=0; i<20; i++)
		iosend(SNOP);

	i=1000;
  do
  {
    i--;
    if (i<=0)
    {
      showerr("Unable to flush serial line.");
      break;
    }
  }while(iorec()>=0);
}


qint XB1device( qint devcode, qint dn)
{
  return(xbcode[devcode][dn][xbsel]);
}


void XB1gtrig( void )
{
  iosend(GTRIG);
}

void XB1ltrig( qint rn )
{
  iosend(LTRIG);
  iosend(rn);
}

qint XB1version( qint devcode, qint dn )
{
	qint rv;

	rv = 0;
  if (devcode<1)
  {
    iosend(ARB_VER_REQ);
    iosend(dn);
    rv = (qint)get8();
  }
  else
  {
    getxln(dn,devcode,"XB1version");
    tdtdisab();
    shortform(VER_REQUEST);
    rv = (qint)get8();
    enable();
  }
	return(rv);
}


void XB1select( qint cptr )
{
  xbsel = 0;
  if (cptr==COM_APb || cptr==0x312 || cptr==2)
    xbsel = 1;
}


/********************************************************************
  PA4 programming procedures
 ********************************************************************/
void PA4atten(qint din, float atten)
{
	qint v;

	getxln(din,PA4_CODE,"PA4atten");
  v = (qint)(atten*10.0+0.05);
  cmd[0] = PA4_ATT;
  cmd[1] = v >> 8;
  cmd[2] = v;
  standform(cmd, 3);
}

void PA4setup(qint din, float base, float step)
{
  qint bv,sv;

  getxln(din,PA4_CODE,"PA4setup");
  bv = (qint)(base*10.0+0.05);
  sv = (qint)(step*10.0+0.05);
  cmd[0] = PA4_SETUP;
	cmd[1] = bv >> 8;
  cmd[2] = bv;
  cmd[3] = sv >> 8;
  cmd[4] = sv;
  standform(cmd, 5);
}

void PA4mute(qint din)
{
  getxln(din,PA4_CODE,"PA4mute");
  shortform(PA4_MUTE);
}

void PA4nomute(qint din)
{
  getxln(din,PA4_CODE,"PA4nomute");
	shortform(PA4_NOMUTE);
}

void PA4ac(qint din)
{
  getxln(din,PA4_CODE,"PA4ac");
  shortform(PA4_AC);
}

void PA4dc(qint din)
{
  getxln(din,PA4_CODE,"PA4dc");
  shortform(PA4_DC);
}

void PA4man(qint din)
{
  getxln(din,PA4_CODE,"PA4man");
  shortform(PA4_MAN);
}

void PA4auto(qint din)
{
  getxln(din,PA4_CODE,"PA4auto");
  shortform(PA4_AUTO);
}

float PA4read(qint din)
{
	qint val;

	getxln(din,PA4_CODE,"PA4read");

	tdtdisab();
	shortform(PA4_READ);
	val = get8() << 8;
	val = val | get8();
	enable();
	return((float)val*0.1);
}


/********************************************************************
  ET1 Procedures
 ********************************************************************/
void ET1clear(qint din)
{
  getxln(din,ET1_CODE,"ET1clear");
  shortform(ET1_CLEAR);
}

void ET1mult(qint din)
{
  getxln(din,ET1_CODE,"ET1mult");
  shortform(ET1_MULT);
}

void ET1compare(qint din)
{
  getxln(din,ET1_CODE,"ET1compare");
  shortform(ET1_COMPARE);
}

void ET1evcount(qint din)
{
  getxln(din,ET1_CODE,"ET1evcount");
  shortform(ET1_EVCOUNT);
}

void ET1go(qint din)
{
  getxln(din,ET1_CODE,"ET1go");
  shortform(ET1_GO);
}

void ET1stop(qint din)
{
  getxln(din,ET1_CODE,"ET1stop");
  shortform(ET1_STOP);
}

qint ET1active(qint din)
{
  qint v;

  getxln(din,ET1_CODE,"ET1active");
  tdtdisab();
  shortform(ET1_ACTIVE);
  v=get8();
  enable();
  return(v);
}

void ET1drop(qint din)
{
  getxln(din,ET1_CODE,"ET1drop");
  shortform(ET1_DROP);
}

void ET1blocks(qint din, qint nblocks)
{
  getxln(din,ET1_CODE,"ET1blocks");
	cmd[0] = ET1_BLOCKS;
  cmd[1] = nblocks;
	cmd[2] = nblocks >> 8;
  standform(cmd, 3);
}

void ET1xlogic(qint din, qint lmask)
{
  getxln(din,ET1_CODE,"ET1xlogic");
  cmd[0] = ET1_XLOGIC;
  cmd[1] = lmask;
  standform(cmd, 2);
}

#ifdef XBDRV_SCHED
static long ET1report_int(qint din)
#else /* XBDRV_SCHED */
long ET1report(qint din)
#endif /* XBDRV_SCHED */
{
  long val;

  getxln(din,ET1_CODE,"ET1report(");
  tdtdisab();
  shortform(ET1_REPORT);
  val = get32();
  enable();
  return(val);
}

#ifdef XBDRV_SCHED
static unsigned long ET1read32_int(qint din)
#else /* XBDRV_SCHED */
unsigned long ET1read32(qint din)
#endif /* XBDRV_SCHED */
{
  unsigned long val;

  getxln(din,ET1_CODE,"ET1read32");
  tdtdisab();
  shortform(ET1_READ32);
  val = get32();
  enable();
  return(val);
}

#ifdef XBDRV_SCHED
static qint ET1read16_int(qint din)
#else /* XBDRV_SCHED */
qint ET1read16(qint din)
#endif /* XBDRV_SCHED */
{
  qint val;

  getxln(din,ET1_CODE,"ET1read16");
  tdtdisab();
  shortform(ET1_READ16);
  val = get16();
  enable();
  return(val);
}



/********************************************************************
  SD1 Procedures
 ********************************************************************/
void SD1go(qint din)
{
  getxln(din,SD1_CODE,"SD1go");
  shortform(SD1_GO);
}

void SD1stop(qint din)
{
  getxln(din,SD1_CODE,"SD1stop");
  shortform(SD1_STOP);
}

void SD1use_enable(qint din)
{
  getxln(din,SD1_CODE,"SD1use_enable");
	shortform(SD1_ENABLE);
}

void SD1no_enable(qint din)
{
  getxln(din,SD1_CODE,"SD1no_enable");
  shortform(SD1_NOENABLE);
}

/* Note all times are in milliseconds */
void SD1hoop(qint din, qint num,
		      qint slope, float dly, float width, float upper, float lower)
{
  getxln(din,SD1_CODE,"SD1hoop");
  shortform(SD1_STOP);
  cmd[0] = SD1_HOOP;
  cmd[1] = num;
  cmd[2] = slope;
  cmd[3] = (qint)((dly*38.4)+0.5);
  cmd[4] = (qint)((width*307.2)+0.5);
  cmd[5] = (qint)((upper*10.0)+128.5);
  cmd[6] = (qint)((lower*10.0)+128.5);
  standform(cmd, 7);
}


void SD1numhoops(qint din, qint num )
{
  getxln(din,SD1_CODE,"SD1numhoops");
  shortform(SD1_STOP);
  cmd[0] = SD1_NUMHOOPS;
  cmd[1] = num;
	standform(cmd, 2);
}


#ifdef XBDRV_SCHED
static qint SD1count_int(qint din)
#else /* XBDRV_SCHED */
qint SD1count(qint din)
#endif /* XBDRV_SCHED */

{
  qint val;

  getxln(din,SD1_CODE,"SD1count");
  tdtdisab();
  shortform(SD1_COUNT);
  val = get16();
  enable();
  return(val);
}

void SD1up(qint din, char cbuf[])
{
  qint i;

  getxln(din,SD1_CODE,"SD1up(");
  tdtdisab();
  shortform(SD1_UP);
  for(i=0; i<32; i++)
    cbuf[i] = get8();
  enable();
}

void SD1down(qint din, char cbuf[])
{
  qint i;

  getxln(din,SD1_CODE,"SD1down");
  cmd[0] = SD1_DOWN;
	for(i=0; i<32; i++)
    cmd[i+1]=cbuf[i];
	standform(cmd, 33);
}


/********************************************************************
  CG1 Procedures
 ********************************************************************/
void CG1go(qint din)
{
  getxln(din,CG1_CODE,"CG1go");
  shortform(CG1_GO);
}

void CG1stop(qint din)
{
  getxln(din,CG1_CODE,"CG1stop");
  shortform(CG1_STOP);
}

void CG1reps(qint din, qint reps )
{
  getxln(din,CG1_CODE,"CG1reps");
  cmd[0] = CG1_REPS;
  cmd[1] = reps;
  cmd[2] = reps >> 8;
  standform(cmd, 3);
}

void CG1trig(qint din, qint ttype )
{
  getxln(din,CG1_CODE,"CG1trig");
  cmd[0] = CG1_TRIG;
	cmd[1] = ttype;
  standform(cmd, 2);
}

void CG1period(qint din, float period )
{
  unsigned long p;

  getxln(din,CG1_CODE,"CG1period");
  p = (long)(period * 10.0);
  cmd[0] = CG1_PERIOD;
  cmd[1] = p;
  cmd[2] = p >> 8;
  cmd[3] = p >> 16;
  cmd[4] = p >> 24;
  standform(cmd, 5);
}

void CG1pulse(qint din, float on_t, float off_t )
{
  unsigned long p;

  getxln(din,CG1_CODE,"CG1pulse");
  p = (long)(on_t * 10.0);
  cmd[0] = CG1_PULSE;
  cmd[1] = p;
  cmd[2] = p >> 8;
  cmd[3] = p >> 16;
  cmd[4] = p >> 24;
  p = (long)(off_t * 10.0);
  cmd[5] = p;
  cmd[6] = p >> 8;
  cmd[7] = p >> 16;
  cmd[8] = p >> 24;
	standform(cmd, 9);
}

void CG1patch(qint din, qint pcode )
{
  getxln(din,CG1_CODE,"CG1patch");
  cmd[0] = CG1_PATCH;
  cmd[1] = pcode;
  standform(cmd, 2);
}

char CG1active(qint din)
{
  char val;

  getxln(din,CG1_CODE,"CG1active");
  tdtdisab();
  shortform(CG1_ACTIVE);
  val = get8();
  enable();
  return(val);
}

void CG1tgo(qint din)
{
  getxln(din,CG1_CODE,"CG1tgo");
  shortform(CG1_TGO);
}



/********************************************************************
  PI1 Procedures
 ********************************************************************/
void PI1clear(qint din)
{
	getxln(din,PI1_CODE,"PI1clear");
  shortform(PI1_CLEAR);
}

void PI1outs( qint din, qint omask)
{
  getxln(din,PI1_CODE,"PI1outs");
  cmd[0] = PI1_OUTS;
  cmd[1] = omask;
  standform(cmd, 2);
}

void PI1logic( qint din, qint logout, qint login)
{
  getxln(din,PI1_CODE,"PI1logic");
  cmd[0] = PI1_LOGIC;
  cmd[1] = logout;
  cmd[2] = login;
  standform(cmd, 3);
}

void PI1write( qint din, qint bitcode)
{
  getxln(din,PI1_CODE,"PI1write");
  cmd[0] = PI1_WRITE;
  cmd[1] = bitcode;
  standform(cmd, 2);
}

qint PI1read(qint din)
{
  char val;

  getxln(din,PI1_CODE,"PI1read");
	tdtdisab();
  shortform(PI1_READ);
  val = get8();
  enable();
  return(val);
}

void PI1autotime( qint din, qint atmask, float dur)
{
  qint val;

  getxln(din,PI1_CODE,"PI1autotime");
  cmd[0] = PI1_AUTOTIME;
  cmd[1] = atmask;
  val = (qint)(dur * 5.0);
  cmd[2] = val;
  cmd[3] = val >> 8;
  standform(cmd, 4);
}


void PI1map( qint din, qint incode, qint mapoutmask)
{
  getxln(din,PI1_CODE,"PI1map");
  cmd[0] = PI1_MAP;
  cmd[1] = incode;
  cmd[2] = mapoutmask;
  standform(cmd, 3);
}

void PI1latch( qint din, qint latchmask)
{
	getxln(din,PI1_CODE,"PI1latch");
  cmd[0] = PI1_LATCH;
	cmd[1] = latchmask;
  standform(cmd, 2);
}

void PI1debounce( qint din)
{
  getxln(din,PI1_CODE,"PI1debounce");
  shortform(PI1_DEBOUNCE);
}

void PI1strobe( qint din)
{
  getxln(din,PI1_CODE,"PI1strobe");
  shortform(PI1_STROBE);
}

qint PI1optread( qint din)
{
  char val;

  getxln(din,PI1_CODE,"PI1optread");
  tdtdisab();
  shortform(PI1_OPTREAD);
  val = get8();
  enable();
  return(val);
}

void PI1optwrite( qint din, qint bitcode)
{
  getxln(din,PI1_CODE,"PI1optwrite");
  cmd[0] = PI1_OPTWRITE;
	cmd[1] = bitcode;
  standform(cmd, 2);
}


/********************************************************************
  PI2 Procedures
 ********************************************************************/
void PI2clear(qint din)
{
  getxln(din,PI2_CODE,"PI2clear");
  shortform(PI2_CLEAR);
}

void PI2outs( qint din, qint omask)
{
  getxln(din,PI2_CODE,"PI2outs");
  cmd[0] = PI2_OUTS;
  cmd[1] = omask;
  standform(cmd, 2);
}

void PI2logic( qint din, qint logout, qint login)
{
  getxln(din,PI2_CODE,"PI2logic");
  cmd[0] = PI2_LOGIC;
  cmd[1] = logout;
  cmd[2] = login;
  standform(cmd, 3);
}

void PI2write( qint din, qint bitcode)
{
  getxln(din,PI2_CODE,"PI2write");
	cmd[0] = PI2_WRITE;
  cmd[1] = bitcode;
	standform(cmd, 2);
}

qint PI2read(qint din)
{
  unsigned char val;

  getxln(din,PI2_CODE,"PI2read");
  tdtdisab();
  shortform(PI2_READ);
  val = get8();
  enable();
  return((qint)val);
}

void PI2autotime(qint din, qint bitn, qint dur)
{
  getxln(din,PI2_CODE,"PI2setAT");
  cmd[0] = PI2_AUTOTIME;
  cmd[1] = bitn;
  cmd[2] = dur;
  cmd[3] = dur >> 8;
  standform(cmd, 4);
}

void PI2toggle( qint din, qint tmask)
{
  getxln(din,PI2_CODE,"PI2toggle");
  cmd[0] = PI2_TOGGLE;
  cmd[1] = tmask;
  standform(cmd, 2);
}

void PI2setbit( qint din, qint bitmask)
{
  getxln(din,PI2_CODE,"PI2setbit");
  cmd[0] = PI2_SETBIT;
  cmd[1] = bitmask;
  standform(cmd, 2);
}

void PI2clrbit( qint din, qint bitmask)
{
  getxln(din,PI2_CODE,"PI2clrbit");
  cmd[0] = PI2_CLRBIT;
  cmd[1] = bitmask;
  standform(cmd, 2);
}

void PI2zerotime( qint din, qint bitmask)
{
  getxln(din,PI2_CODE,"PI2zerotime");
  cmd[0] = PI2_ZEROTIME;
  cmd[1] = bitmask;
  standform(cmd, 2);
}

#ifdef XBDRV_SCHED
static qint PI2gettime_int( qint din, qint bitn)
#else /* XBDRV_SCHED */
qint PI2gettime( qint din, qint bitn)
#endif /* XBDRV_SCHED */
{
  qint val;

  getxln(din,PI2_CODE,"PI2gettime");
  cmd[0] = PI2_GETTIME;
  cmd[1] = bitn;
  tdtdisab();
  standform(cmd, 2);
	val = get16();
  enable();
	return(val);
}

void PI2latch( qint din, qint lmask)
{
  getxln(din,PI2_CODE,"PI2latch");
  cmd[0] = PI2_LATCH;
  cmd[1] = lmask;
  standform(cmd, 2);
}

void PI2debounce( qint din, qint dbtime)
{
  getxln(din,PI2_CODE,"PI2debounce");
  cmd[0] = PI2_DEBOUNCE;
  cmd[1] = dbtime;
  standform(cmd, 2);
}

void PI2map( qint din, qint bitn, qint mmask)
{
  getxln(din,PI2_CODE,"PI2map");
  cmd[0] = PI2_MAP;
  cmd[1] = bitn;
  cmd[2] = mmask;
  standform(cmd, 3);
}

void PI2outsX( qint din, qint pnum)
{
  getxln(din,PI2_CODE,"PI2outsX");
  cmd[0] = PI2_OUTSX;
	cmd[1] = pnum;
  standform(cmd, 2);
}

void PI2writeX( qint din, qint pnum, qint val)
{
  getxln(din,PI2_CODE,"PI2writeX");
  cmd[0] = PI2_WRITEX;
  cmd[1] = pnum;
  cmd[2] = val;
  standform(cmd, 3);
}

qint PI2readX( qint din, qint pnum)
{
  qint val;

  getxln(din,PI2_CODE,"PI2readX");
  cmd[0] = PI2_READX;
  cmd[1] = pnum;
  tdtdisab();
  standform(cmd, 2);
  val = (qint)get8();
  enable();
  return(val);
}

/********************************************************************
  WG1 Procedures
 ********************************************************************/

void WG1on(qint din)
{
  getxln(din,WG1_CODE,"WG1on");
	shortform(WG1_ON);
}

void WG1off(qint din)
{
  getxln(din,WG1_CODE,"WG1off");
  shortform(WG1_OFF);
}

void WG1clear(qint din)
{
  getxln(din,WG1_CODE,"WG1clear");
  shortform(WG1_CLEAR);
}

void WG1amp(qint din, float amp)
{
  qint v;

  getxln(din,WG1_CODE,"WG1amp");
  v = amp * 100.0;
  cmd[0] = WG1_AMP;
  cmd[1] = v;
  cmd[2] = v >> 8;
  standform(cmd, 3);
}

void WG1freq(qint din, float freq)
{
  qint v;

  getxln(din,WG1_CODE,"WG1freq");
  v = (qint)(freq);
  cmd[0] = WG1_FREQ;
	cmd[1] = v;
  cmd[2] = v >> 8;
	standform(cmd, 3);
}

void WG1swrt(qint din, float swrt)
{
  qint v;

  getxln(din,WG1_CODE,"WG1swrt");
  v=(qint)(swrt);
  cmd[0] = WG1_SWRT;
  cmd[1] = v;
  cmd[2] = v >> 8;
  standform(cmd, 3);
}

void WG1phase(qint din, float phase)
{
  qint v;

  getxln(din,WG1_CODE,"WG1phase");
  v=(qint)(phase*50.0);
  cmd[0] = WG1_PHASE;
  cmd[1] = v;
  cmd[2] = v >> 8;
  standform(cmd, 3);
}

void WG1dc(qint din, float dc)
{
  qint v;

  getxln(din,WG1_CODE,"WG1dc");
	v=(qint)(dc*10.0);
  cmd[0] = WG1_DC;
	cmd[1] = v;
  cmd[2] = v >> 8;
  standform(cmd, 3);
}

void WG1dur(qint din, float dur)
{
  qint v;

  getxln(din,WG1_CODE,"WG1dur");
  v=(qint)(dur);
  cmd[0] = WG1_DUR;
  cmd[1] = v;
  cmd[2] = v >> 8;
  standform(cmd, 3);
}

void WG1shape(qint din, qint scon)
{
  getxln(din,WG1_CODE,"WG1shape");
  cmd[0] = WG1_SHAPE;
  cmd[1] = scon;
  cmd[2] = scon >> 8;
  standform(cmd, 3);
}

void WG1rf(qint din, float rf)
{
  qint v;
  getxln(din,WG1_CODE,"WG1rf");
  v = (qint)(rf * 10.0);
  cmd[0] = WG1_RF;
	cmd[1] = v;
  cmd[2] = v >> 8;
	standform(cmd, 3);
}

void WG1trig(qint din, qint tcode)
{
  getxln(din,WG1_CODE,"WG1trig");
  cmd[0] = WG1_TRIG;
  cmd[1] = tcode;
  cmd[2] = tcode >> 8;
  standform(cmd, 3);
}

void WG1seed(qint din, long seed)
{
  getxln(din,WG1_CODE,"WG1seed");
  cmd[0] = WG1_SEED;
  cmd[1] = seed;
  cmd[2] = seed >> 8;
  cmd[3] = seed >> 16;
  cmd[4] = seed >> 24;
  standform(cmd, 5);
}

void WG1delta(qint din, long delta)
{
  getxln(din,WG1_CODE,"WG1delta");
  cmd[0] = WG1_DELTA;
  cmd[1] = delta;
  cmd[2] = delta >> 8;
  cmd[3] = delta >> 16;
  cmd[4] = delta >> 24;
  standform(cmd, 5);
}

void WG1wave(qint din, qint wave[], qint npts)
{
  qint i,n,p;

  getxln(din,WG1_CODE,"WG1wave");
  cmd[0] = WG1_WAVE;
  cmd[1] = npts;
  cmd[2] = npts >> 8;
  standform(cmd,3);

  i = npts;
  p = 0;
  do
  {
    if (i>0)
    {
      n=i;
      if (n>25) n=25;
      standform((char *)(&wave[p]),n << 1);
      i -= n;
      p+=n;
    }
  }while(i>0);
}

qint WG1status(qint din)
{
  char val;

  getxln(din,WG1_CODE,"WG1status");
  tdtdisab();
  shortform(WG1_STATUS);
	val = get8();
  enable();
	return(val);
}

void WG1ton(qint din)
{
  getxln(din,WG1_CODE,"WG1ton");
  shortform(WG1_TON);
}


/********************************************************************
  WG2 Procedures
 ********************************************************************/

void WG2on(qint din)
{
  getxln(din,WG2_CODE,"WG2on");
  shortform(WG2_ON);
}

void WG2off(qint din)
{
  getxln(din,WG2_CODE,"WG2off");
  shortform(WG2_OFF);
}

void WG2clear(qint din)
{
  getxln(din,WG2_CODE,"WG2clear");
  shortform(WG2_CLEAR);
}

void WG2amp(qint din, float amp)
{
	qint v;

  getxln(din,WG2_CODE,"WG2amp");
  v = amp * 100.0;
  cmd[0] = WG2_AMP;
  cmd[1] = v;
  cmd[2] = v >> 8;
  standform(cmd, 3);
}

void WG2freq(qint din, float freq)
{
  qint v;

  getxln(din,WG2_CODE,"WG2freq");
  v = (qint)(freq);
  cmd[0] = WG2_FREQ;
  cmd[1] = v;
  cmd[2] = v >> 8;
  standform(cmd, 3);
}

void WG2swrt(qint din, float swrt)
{
  qint v;

  getxln(din,WG2_CODE,"WG2swrt");
  v=(qint)(swrt);
  cmd[0] = WG2_SWRT;
  cmd[1] = v;
  cmd[2] = v >> 8;
  standform(cmd, 3);
}

void WG2phase(qint din, float phase)
{
  qint v;

  getxln(din,WG2_CODE,"WG2phase");
  v=(qint)(phase*50.0);
  cmd[0] = WG2_PHASE;
  cmd[1] = v;
  cmd[2] = v >> 8;
  standform(cmd, 3);
}

void WG2dc(qint din, float dc)
{
  qint v;

  getxln(din,WG2_CODE,"WG2dc");
  v=(qint)(dc*10.0);
  cmd[0] = WG2_DC;
  cmd[1] = v;
  cmd[2] = v >> 8;
  standform(cmd, 3);
}

void WG2dur(qint din, float dur)
{
  qint v;

  getxln(din,WG2_CODE,"WG2dur");
  v=(qint)(dur);
  cmd[0] = WG2_DUR;
  cmd[1] = v;
	cmd[2] = v >> 8;
  standform(cmd, 3);
}

void WG2shape(qint din, qint scon)
{
  getxln(din,WG2_CODE,"WG2shape");
  cmd[0] = WG2_SHAPE;
  cmd[1] = scon;
  cmd[2] = scon >> 8;
  standform(cmd, 3);
}

void WG2rf(qint din, float rf)
{
  qint v;
  getxln(din,WG2_CODE,"WG2rf");
  v = (qint)(rf * 10.0);
  cmd[0] = WG2_RF;
  cmd[1] = v;
  cmd[2] = v >> 8;
  standform(cmd, 3);
}

void WG2trig(qint din, qint tcode)
{
  getxln(din,WG2_CODE,"WG2trig");
  cmd[0] = WG2_TRIG;
  cmd[1] = tcode;
  cmd[2] = tcode >> 8;
  standform(cmd, 3);
}

void WG2seed(qint din, long seed)
{
  getxln(din,WG2_CODE,"WG2seed");
	cmd[0] = WG2_SEED;
  cmd[1] = seed;
  cmd[2] = seed >> 8;
  cmd[3] = seed >> 16;
  cmd[4] = seed >> 24;
  standform(cmd, 5);
}

void WG2delta(qint din, long delta)
{
  getxln(din,WG2_CODE,"WG2delta");
  cmd[0] = WG2_DELTA;
  cmd[1] = delta;
  cmd[2] = delta >> 8;
  cmd[3] = delta >> 16;
  cmd[4] = delta >> 24;
  standform(cmd, 5);
}

void WG2wave(qint din, qint wave[], qint npts)
{
  qint i,n,p;

  getxln(din,WG2_CODE,"WG2wave");
  cmd[0] = WG2_WAVE;
  cmd[1] = npts;
  cmd[2] = npts >> 8;
  standform(cmd,3);

  i = npts;
  p = 0;
  do
	{
    if (i>0)
		{
      n=i;
      if (n>25) n=25;
      standform((char *)(&wave[p]),n << 1);
      i -= n;
      p+=n;
    }
  }while(i>0);
}

qint WG2status(qint din)
{
  char val;

  getxln(din,WG2_CODE,"WG2status");
  tdtdisab();
  shortform(WG2_STATUS);
  val = get8();
  enable();
  return(val);
}

void WG2ton(qint din)
{
  getxln(din,WG2_CODE,"WG2ton");
  shortform(WG2_TON);
}


/********************************************************************
  DD1 Procedures
 ********************************************************************/
void DD1clear( qint din)
{
	getxln(din,DD1_CODE,"DD1clear");
  shortform(DD1_CLEAR);
}

void DD1go( qint din)
{
  getxln(din,DD1_CODE,"DD1go");
  shortform(DD1_GO);
}

void DD1stop( qint din)
{
  getxln(din,DD1_CODE,"DD1stop");
  shortform(DD1_STOP);
}

void DD1arm( qint din)
{
  getxln(din,DD1_CODE,"DD1arm");
  shortform(DD1_ARM);
}

void DD1mode( qint din, qint mcode)
{
  getxln(din,DD1_CODE,"DD1mode");
  cmd[0] = DD1_MODE;
  cmd[1] = mcode;
  cmd[2] = 0;
  standform(cmd, 3);
}

void DD1srate( qint din, float srate)
{
  unsigned qint v;
	getxln(din,DD1_CODE,"DD1srate");
  v = (qint)((10.0*srate) + 0.5);
  cmd[0] = DD1_SRATE;
  cmd[1] = v;
  cmd[2] = v >> 8;
  standform(cmd, 3);
}

float DD1speriod( qint din, float sper)
{
	unsigned qint v;
	getxln(din,DD1_CODE,"DD1speriod");
	v = (qint)((12.5*sper) + 0.5);
	cmd[0] = DD1_SPERIOD;
	cmd[1] = v;
	cmd[2] = v >> 8;
	standform(cmd, 3);
	return(v*0.08);
}

void DD1clkin( qint din, qint scode)
{
  getxln(din,DD1_CODE,"DD1clkin");
  cmd[0] = DD1_CLKIN;
  cmd[1] = scode;
  cmd[2] = 0;
  standform(cmd, 3);
}

void DD1clkout( qint din, qint dcode)
{
  getxln(din,DD1_CODE,"DD1clkout");
  cmd[0] = DD1_CLKOUT;
  cmd[1] = dcode;
  cmd[2] = 0;
  standform(cmd, 3);
}

void DD1npts( qint din, long npts)
{
  getxln(din,DD1_CODE,"DD1npts");
  cmd[0] = DD1_NPTS;
  cmd[1] = npts;
  cmd[2] = npts >> 8;
	cmd[3] = npts >> 16;
  cmd[4] = npts >> 24;
	standform(cmd, 5);
}

void DD1mtrig( qint din)
{
  getxln(din,DD1_CODE,"DD1mtrig");
  shortform(DD1_MTRIG);
}

void DD1strig( qint din)
{
  getxln(din,DD1_CODE,"DD1strig");
  shortform(DD1_STRIG);
}

qint DD1status(qint din)
{
  qint val;

  getxln(din,DD1_CODE,"DD1status");
	tdtdisab();
  shortform(DD1_STATUS);
  val = (qint)get8();
  enable();
  return(val);
}

void DD1reps( qint din, unsigned qint nreps)
{
  getxln(din,DD1_CODE,"DD1reps");
	cmd[0] = DD1_REPS;
  cmd[1] = nreps;
  cmd[2] = nreps >> 8;
  standform(cmd, 3);
}

qint DD1clip( qint din)
{
  qint val;

  getxln(din,DD1_CODE,"DD1clip");
  tdtdisab();
  shortform(DD1_CLIP);
  val = (qint)get8();
  enable();
  return(val);
}

void DD1clipon( qint din)
{
  getxln(din,DD1_CODE,"DD1clipon");
  shortform(DD1_CLIPON);
}

void DD1echo( qint din)
{
  getxln(din,DD1_CODE,"DD1echo");
  shortform(DD1_ECHO);
}

void DD1tgo( qint din)
{
	getxln(din,DD1_CODE,"DD1tgo");
  shortform(DD1_TGO);
}


/********************************************************************
  DA1 Procedures
 ********************************************************************/
void DA1clear( qint din)
{
  getxln(din,DA1_CODE,"DA1clear");
  shortform(DA1_CLEAR);
}

void DA1go( qint din)
{
  getxln(din,DA1_CODE,"DA1go");
  shortform(DA1_GO);
}

void DA1stop( qint din)
{
  getxln(din,DA1_CODE,"DA1stop");
  shortform(DA1_STOP);
}

void DA1arm( qint din)
{
  getxln(din,DA1_CODE,"DA1arm");
  shortform(DA1_ARM);
}

void DA1mode( qint din, qint mcode)
{
	getxln(din,DA1_CODE,"DA1mode");
  cmd[0] = DA1_MODE;
	cmd[1] = mcode;
  cmd[2] = 0;
  standform(cmd, 3);
}

void DA1srate( qint din, float srate)
{
	unsigned qint v;
	getxln(din,DA1_CODE,"DA1srate");
	v = (qint)((10.0*srate) + 0.5);
	cmd[0] = DA1_SRATE;
	cmd[1] = v;
	cmd[2] = v >> 8;
	standform(cmd, 3);
}

float DA1speriod( qint din, float sper)
{
	unsigned qint v;
	getxln(din,DA1_CODE,"DA1speriod");
	v = (qint)((12.5*sper) + 0.5);
	cmd[0] = DA1_SPERIOD;
	cmd[1] = v;
	cmd[2] = v >> 8;
	standform(cmd, 3);
	return(v*0.08);
}

void DA1clkin( qint din, qint scode)
{
  getxln(din,DA1_CODE,"DA1clkin");
  cmd[0] = DA1_CLKIN;
  cmd[1] = scode;
  cmd[2] = 0;
  standform(cmd, 3);
}

void DA1clkout( qint din, qint dcode)
{
  getxln(din,DA1_CODE,"DA1clkout");
  cmd[0] = DA1_CLKOUT;
  cmd[1] = dcode;
  cmd[2] = 0;
  standform(cmd, 3);
}

void DA1npts( qint din, long npts)
{
  getxln(din,DA1_CODE,"DA1npts");
  cmd[0] = DA1_NPTS;
  cmd[1] = npts;
  cmd[2] = npts >> 8;
  cmd[3] = npts >> 16;
  cmd[4] = npts >> 24;
  standform(cmd, 5);
}

void DA1mtrig( qint din)
{
  getxln(din,DA1_CODE,"DA1mtrig");
  shortform(DA1_MTRIG);
}

void DA1strig( qint din)
{
  getxln(din,DA1_CODE,"DA1strig");
  shortform(DA1_STRIG);
}

qint DA1status(qint din)
{
  qint val;

  getxln(din,DA1_CODE,"DA1status");
  tdtdisab();
  shortform(DA1_STATUS);
  val = (qint)get8();
  enable();
	return(val);
}

void DA1reps( qint din, unsigned qint nreps)
{
  getxln(din,DA1_CODE,"DA1reps");
  cmd[0] = DA1_REPS;
  cmd[1] = nreps;
  cmd[2] = nreps >> 8;
  standform(cmd, 3);
}

qint DA1clip( qint din)
{
  qint val;

  getxln(din,DA1_CODE,"DA1clip");
  tdtdisab();
  shortform(DA1_CLIP);
  val = (qint)get8();
  enable();
  return(val);
}

void DA1clipon( qint din)
{
  getxln(din,DA1_CODE,"DA1clipon");
  shortform(DA1_CLIPON);
}

void DA1tgo( qint din)
{
  getxln(din,DA1_CODE,"DA1tgo");
  shortform(DA1_TGO);
}


/********************************************************************
  AD1 Procedures
 ********************************************************************/
void AD1clear( qint din)
{
  getxln(din,AD1_CODE,"AD1clear");
  shortform(AD1_CLEAR);
}

void AD1go( qint din)
{
  getxln(din,AD1_CODE,"AD1go");
  shortform(AD1_GO);
}

void AD1stop( qint din)
{
  getxln(din,AD1_CODE,"AD1stop");
  shortform(AD1_STOP);
}

void AD1arm( qint din)
{
  getxln(din,AD1_CODE,"AD1arm");
  shortform(AD1_ARM);
}

void AD1mode( qint din, qint mcode)
{
  getxln(din,AD1_CODE,"AD1mode");
  cmd[0] = AD1_MODE;
	cmd[1] = mcode;
  cmd[2] = 0;
	standform(cmd, 3);
}

void AD1srate( qint din, float srate)
{
  unsigned qint v;
  getxln(din,AD1_CODE,"AD1srate");
  v = (qint)((10.0*srate) + 0.5);
  cmd[0] = AD1_SRATE;
  cmd[1] = v;
  cmd[2] = v >> 8;
  standform(cmd, 3);
}

float AD1speriod( qint din, float sper)
{
	unsigned qint v;
	getxln(din,AD1_CODE,"AD1speriod");
	v = (qint)((12.5*sper) + 0.5);
	cmd[0] = AD1_SPERIOD;
	cmd[1] = v;
	cmd[2] = v >> 8;
	standform(cmd, 3);
	return(v*0.08);
}

void AD1clkin( qint din, qint scode)
{
  getxln(din,AD1_CODE,"AD1clkin");
  cmd[0] = AD1_CLKIN;
  cmd[1] = scode;
  cmd[2] = 0;
  standform(cmd, 3);
}

void AD1clkout( qint din, qint dcode)
{
  getxln(din,AD1_CODE,"AD1clkout");
  cmd[0] = AD1_CLKOUT;
  cmd[1] = dcode;
  cmd[2] = 0;
  standform(cmd, 3);
}

void AD1npts( qint din, long npts)
{
	getxln(din,AD1_CODE,"AD1npts");
  cmd[0] = AD1_NPTS;
  cmd[1] = npts;
  cmd[2] = npts >> 8;
  cmd[3] = npts >> 16;
  cmd[4] = npts >> 24;
  standform(cmd, 5);
}

void AD1mtrig( qint din)
{
  getxln(din,AD1_CODE,"AD1mtrig");
  shortform(AD1_MTRIG);
}

void AD1strig( qint din)
{
  getxln(din,AD1_CODE,"AD1strig");
  shortform(AD1_STRIG);
}

qint AD1status(qint din)
{
  qint val;

  getxln(din,AD1_CODE,"AD1status");
  tdtdisab();
  shortform(AD1_STATUS);
  val = (qint)get8();
  enable();
  return(val);
}

void AD1reps( qint din, unsigned qint nreps)
{
  getxln(din,AD1_CODE,"AD1reps");
  cmd[0] = AD1_REPS;
  cmd[1] = nreps;
  cmd[2] = nreps >> 8;
  standform(cmd, 3);
}

qint AD1clip( qint din)
{
  qint val;

  getxln(din,AD1_CODE,"AD1clip");
  tdtdisab();
  shortform(AD1_CLIP);
  val = (qint)get8();
  enable();
  return(val);
}

void AD1clipon( qint din)
{
  getxln(din,AD1_CODE,"AD1clipon");
  shortform(AD1_CLIPON);
}

void AD1tgo( qint din)
{
  getxln(din,AD1_CODE,"AD1tgo");
  shortform(AD1_TGO);
}


/********************************************************************
	AD2 Procedures
 ********************************************************************/
void AD2clear( qint din)
{
  getxln(din,AD2_CODE,"AD2clear");
  shortform(AD2_CLEAR);
}

void AD2go( qint din)
{
  getxln(din,AD2_CODE,"AD2go");
  shortform(AD2_GO);
}

void AD2stop( qint din)
{
  getxln(din,AD2_CODE,"AD2stop");
  shortform(AD2_STOP);
}

void AD2arm( qint din)
{
  getxln(din,AD2_CODE,"AD2arm");
  shortform(AD2_ARM);
}

void AD2mode( qint din, qint mcode)
{
  getxln(din,AD2_CODE,"AD2mode");
  cmd[0] = AD2_MODE;
  cmd[1] = mcode;
  cmd[2] = 0;
	standform(cmd, 3);
}

void AD2srate( qint din, float srate)
{
  unsigned qint v;
  getxln(din,AD2_CODE,"AD2srate");
  v = (qint)((10.0*srate) + 0.5);
  cmd[0] = AD2_SRATE;
  cmd[1] = v;
  cmd[2] = v >> 8;
  standform(cmd, 3);
}

float AD2speriod( qint din, float sper)
{
	unsigned qint v;
	getxln(din,AD2_CODE,"AD2speriod");
	v = (qint)((12.5*sper) + 0.5);
	cmd[0] = AD2_SPERIOD;
	cmd[1] = v;
	cmd[2] = v >> 8;
	standform(cmd, 3);
	return(v*0.08);
}

void AD2clkin( qint din, qint scode)
{
  getxln(din,AD2_CODE,"AD2clkin");
  cmd[0] = AD2_CLKIN;
  cmd[1] = scode;
  cmd[2] = 0;
  standform(cmd, 3);
}

void AD2clkout( qint din, qint dcode)
{
  getxln(din,AD2_CODE,"AD2clkout");
  cmd[0] = AD2_CLKOUT;
  cmd[1] = dcode;
  cmd[2] = 0;
  standform(cmd, 3);
}

void AD2npts( qint din, long npts)
{
	getxln(din,AD2_CODE,"AD2npts");
  cmd[0] = AD2_NPTS;
	cmd[1] = npts;
  cmd[2] = npts >> 8;
  cmd[3] = npts >> 16;
  cmd[4] = npts >> 24;
  standform(cmd, 5);
}

void AD2mtrig( qint din)
{
  getxln(din,AD2_CODE,"AD2mtrig");
  shortform(AD2_MTRIG);
}

void AD2strig( qint din)
{
  getxln(din,AD2_CODE,"AD2strig");
  shortform(AD2_STRIG);
}

qint AD2status(qint din)
{
  qint val;

  getxln(din,AD2_CODE,"AD2status");
  tdtdisab();
  shortform(AD2_STATUS);
  val = (qint)get8();
  enable();
  return(val);
}

void AD2reps( qint din, unsigned qint nreps)
{
  getxln(din,AD2_CODE,"AD2reps");
  cmd[0] = AD2_REPS;
  cmd[1] = nreps;
  cmd[2] = nreps >> 8;
	standform(cmd, 3);
}

qint AD2clip( qint din)
{
  qint val;

  getxln(din,AD2_CODE,"AD2clip");
  tdtdisab();
  shortform(AD2_CLIP);
  val = (qint)get8();
  enable();
  return(val);
}

void AD2gain( qint din, qint chan, qint gain)
{
  getxln(din,AD2_CODE,"AD2gain");
  cmd[0] = AD2_GAIN;
  cmd[1] = chan;
  cmd[2] = gain;
  standform(cmd, 3);
}

void AD2sh(qint din, qint oocode)
{
  getxln(din,AD2_CODE,"AD2sh");
  cmd[0] = AD2_SH;
  cmd[1] = oocode;
  cmd[2] = 0;
  standform(cmd, 3);
}

void AD2sampsep(qint din, float sep)
{
  unsigned qint v;

  getxln(din,AD2_CODE,"AD2sampsep");
  sep = sep - 2.1;
  v = (qint)((sep * 12.5) + 0.5);
  cmd[0] = AD2_SAMPSEP;
  cmd[1] = v;
  cmd[2] = v >> 8;
  standform(cmd, 3);
}

void AD2xchans(qint din, qint nchans)
{
  getxln(din,AD2_CODE,"AD2xchans");
  cmd[0] = AD2_XCHANS;
  cmd[1] = nchans;
  cmd[2] = nchans >> 8;
  standform(cmd, 3);
}

void AD2tgo( qint din)
{
  getxln(din,AD2_CODE,"AD2tgo");
  shortform(AD2_TGO);
}


/********************************************************************
  DA3 Procedures
 ********************************************************************/
void DA3clear( qint din)
{
  getxln(din,DA3_CODE,"DA3clear");
	shortform(DA3_CLEAR);
}

void DA3go( qint din)
{
  getxln(din,DA3_CODE,"DA3go");
  shortform(DA3_GO);
}

void DA3stop( qint din)
{
  getxln(din,DA3_CODE,"DA3stop");
  shortform(DA3_STOP);
}

void DA3arm( qint din)
{
  getxln(din,DA3_CODE,"DA3arm");
  shortform(DA3_ARM);
}

void DA3mode( qint din, qint cmask)
{
  getxln(din,DA3_CODE,"DA3mode");
  cmd[0] = DA3_MODE;
  cmd[1] = cmask;
  cmd[2] = 0;
  standform(cmd, 3);
}

void DA3srate( qint din, float srate)
{
  unsigned qint v;
  getxln(din,DA3_CODE,"DA3srate");
	v = (qint)((10.0*srate) + 0.5);
  cmd[0] = DA3_SRATE;
	cmd[1] = v;
  cmd[2] = v >> 8;
  standform(cmd, 3);
}

float DA3speriod( qint din, float sper)
{
	unsigned qint v;
	getxln(din,DA3_CODE,"DA3speriod");
	v = (qint)((12.5*sper) + 0.5);
	cmd[0] = DA3_SPERIOD;
	cmd[1] = v;
	cmd[2] = v >> 8;
	standform(cmd, 3);
	return(v*0.08);
}

void DA3clkin( qint din, qint scode)
{
  getxln(din,DA3_CODE,"DA3clkin");
  cmd[0] = DA3_CLKIN;
  cmd[1] = scode;
  cmd[2] = 0;
  standform(cmd, 3);
}

void DA3clkout( qint din, qint dcode)
{
  getxln(din,DA3_CODE,"DA3clkout");
  cmd[0] = DA3_CLKOUT;
  cmd[1] = dcode;
  cmd[2] = 0;
  standform(cmd, 3);
}

void DA3npts( qint din, long npts)
{
  getxln(din,DA3_CODE,"DA3npts");
  cmd[0] = DA3_NPTS;
  cmd[1] = npts;
  cmd[2] = npts >> 8;
  cmd[3] = npts >> 16;
  cmd[4] = npts >> 24;
  standform(cmd, 5);
}

void DA3mtrig( qint din)
{
  getxln(din,DA3_CODE,"DA3mtrig");
  shortform(DA3_MTRIG);
}

void DA3strig( qint din)
{
  getxln(din,DA3_CODE,"DA3strig");
  shortform(DA3_STRIG);
}

qint DA3status(qint din)
{
  qint val;

  getxln(din,DA3_CODE,"DA3status");
  tdtdisab();
  shortform(DA3_STATUS);
  val = (qint)get8();
  enable();
  return(val);
}

void DA3reps( qint din, unsigned qint nreps)
{
  getxln(din,DA3_CODE,"DA3reps");
  cmd[0] = DA3_REPS;
  cmd[1] = nreps;
  cmd[2] = nreps >> 8;
  standform(cmd, 3);
}

qint DA3clip( qint din)
{
  qint val;

  getxln(din,DA3_CODE,"DA3clip");
  tdtdisab();
  shortform(DA3_CLIP);
  val = (qint)get8();
  enable();
  return(val);
}

void DA3clipon( qint din)
{
  getxln(din,DA3_CODE,"DA3clipon");
  shortform(DA3_CLIPON);
}

void DA3tgo( qint din)
{
  getxln(din,DA3_CODE,"DA3tgo");
  shortform(DA3_TGO);
}

void DA3setslew(qint din, qint slcode)
{
  getxln(din,DA3_CODE,"DA3setslew");
  cmd[0] = DA3_SETSLEW;
  cmd[1] = slcode;
  cmd[2] = 0;
  standform(cmd, 3);
}

void DA3zero( qint din)
{
	getxln(din,DA3_CODE,"DA3zero");
  shortform(DA3_ZERO);
}


/********************************************************************
  SW2 programming procedures
 ********************************************************************/
void SW2on(qint din)
{
  getxln(din,SW2_CODE,"SW2on");
  shortform(SW2_ON);
}

void SW2off(qint din)
{
  getxln(din,SW2_CODE,"SW2off");
  shortform(SW2_OFF);
}

void SW2ton(qint din)
{
  getxln(din,SW2_CODE,"SW2ton");
  shortform(SW2_TON);
}

void SW2toff(qint din)
{
  getxln(din,SW2_CODE,"SW2toff");
  shortform(SW2_TOFF);
}

void SW2rftime(qint din, float rftime)
{
	qint v;

  v = (qint)(rftime*10.0+0.5);
  getxln(din,SW2_CODE,"SW2rftime");
  cmd[0] = SW2_RFTIME;
  cmd[1] = v;
  cmd[2] = v >> 8;
  standform(cmd, 3);
}

void SW2shape(qint din, qint shcode)
{
  getxln(din,SW2_CODE,"SW2shape");
  cmd[0] = SW2_SHAPE;
  cmd[1] = shcode;
  cmd[2] = shcode >> 8;
  standform(cmd, 3);
}

void SW2trig(qint din, qint tcode)
{
  getxln(din,SW2_CODE,"SW2trig");
  cmd[0] = SW2_TRIG;
  cmd[1] = tcode;
  cmd[2] = tcode >> 8;
  standform(cmd, 3);
}

void SW2dur(qint din, qint dur)
{
  getxln(din,SW2_CODE,"SW2dur");
  cmd[0] = SW2_DUR;
	cmd[1] = dur;
  cmd[2] = dur >> 8;
	standform(cmd, 3);
}

qint  SW2status(qint din)
{
  qint val;

  getxln(din,SW2_CODE,"SW2status");
  tdtdisab();
  shortform(SW2_STATUS);
  val = (qint)get8();
  enable();
  return(val);
}

void SW2clear(qint din)
{
  getxln(din,SW2_CODE,"SW2clear");
  shortform(SW2_CLEAR);
}


/********************************************************************
  PF1 programming procedures
 ********************************************************************/
void PF1type(qint din, qint type, qint ntaps)
{
  getxln(din,PF1_CODE,"PF1type");
  cmd[0] = PF1_TYPE;
  cmd[1] = type;
  cmd[2] = 0;
  cmd[3] = ntaps;
	cmd[4] = 0;
  standform(cmd, 5);
}

void PF1begin(qint din)
{
  getxln(din,PF1_CODE,"PF1begin");
  shortform(PF1_BEGIN);
}

void PF1bypass(qint din)
{
  getxln(din,PF1_CODE,"PF1bypass");
  shortform(PF1_BYPASS);
}

void PF1nopass(qint din)
{
  getxln(din,PF1_CODE,"PF1nopass");
  shortform(PF1_NOPASS);
}

void PF1b16(qint din, qint bcoe)
{
  getxln(din,PF1_CODE,"PF1b16");
  cmd[0] = PF1_B16;
  cmd[1] = bcoe;
  cmd[2] = bcoe >> 8;
  standform(cmd, 3);
}

void PF1a16(qint din, qint acoe)
{
  getxln(din,PF1_CODE,"PF1a16");
	cmd[0] = PF1_A16;
  cmd[1] = acoe;
	cmd[2] = acoe >> 8;
  standform(cmd, 3);
}

void PF1b32(qint din, long bcoe)
{
  getxln(din,PF1_CODE,"PF1b32");
  cmd[0] = PF1_B32;
  cmd[1] = bcoe;
  cmd[2] = bcoe >> 8;
  cmd[3] = bcoe >> 16;
  cmd[4] = bcoe >> 24;
  standform(cmd, 5);
}

void PF1a32(qint din, long acoe)
{
  getxln(din,PF1_CODE,"PF1a32");
  cmd[0] = PF1_A32;
  cmd[1] = acoe;
  cmd[2] = acoe >> 8;
  cmd[3] = acoe >> 16;
  cmd[4] = acoe >> 24;
  standform(cmd, 5);
}

void PF1freq(qint din, qint lpfreq, qint hpfreq)
{
  getxln(din,PF1_CODE,"PF1freq");
  cmd[0] = PF1_FREQ;
  cmd[1] = lpfreq;
  cmd[2] = lpfreq >> 8;
	cmd[3] = hpfreq;
  cmd[4] = hpfreq >> 8;
  standform(cmd, 5);
}

void PF1gain(qint din, qint lpgain, qint hpgain)
{
  getxln(din,PF1_CODE,"PF1gain");
  cmd[0] = PF1_GAIN;
  cmd[1] = lpgain;
  cmd[2] = lpgain >> 8;
  cmd[3] = hpgain;
  cmd[4] = hpgain >> 8;
  standform(cmd, 5);
}


qint xb_rint(float v)
{
  if (v>0)
    return((qint)(v+0.5));
  return((qint)(v-0.5));
}

long rlong(float v)
{
  if (v>0)
    return((long)(v+0.5));
  return((long)(v-0.5));
}

void PF1fir16( qint din, float bcoes[], qint ntaps)
{
  qint i;
	PF1begin(din);
  for(i=0; i<ntaps; i++)
    PF1b16(din, xb_rint(bcoes[i]*0x8000));
  PF1type(din, FIR16, ntaps);
}

void PF1fir32( qint din, float bcoes[], qint ntaps)
{
  qint i;
  PF1begin(din);
  for(i=0; i<ntaps; i++)
    PF1b32(din, rlong(bcoes[i]*0x40000000));
  PF1type(din, FIR32, ntaps);
}

void PF1iir32( qint din, float bcoes[], float acoes[], qint ntaps)
{
  qint i;
  PF1begin(din);
  for(i=0; i<ntaps; i++)
  {
    PF1b32(din, rlong(bcoes[i]*0x800000));
    PF1a32(din, rlong(-acoes[i]*0x800000));
  }
  PF1type(din, IIR32, ntaps);
}

void PF1biq16( qint din, float bcoes[], float acoes[], qint nbiqs)
{
  qint i,n;

  n = nbiqs*3;
  PF1begin(din);
  for(i=0; i<n; i++)
	{
    PF1b16(din, xb_rint(bcoes[i]*0x4000));
    PF1a16(din, xb_rint(-acoes[i]*0x4000));
  }
  PF1type(din, BIQ16, nbiqs);
}

void PF1biq32( qint din, float bcoes[], float acoes[], qint nbiqs)
{
  qint i,n;

  n = nbiqs*3;
  PF1begin(din);
  for(i=0; i<n; i++)
  {
    PF1b32(din, rlong(bcoes[i]*0x40000000+0.5));
    PF1a32(din, rlong(-acoes[i]*0x40000000+0.5));
  }
  PF1type(din, BIQ32, nbiqs);
}


/********************************************************************
  TG6 Procedures
 ********************************************************************/
void TG6clear(qint din)
{
  getxln(din,TG6_CODE,"TG6clear");
  shortform(TG6_CLEAR);
}

void TG6arm(qint din, qint snum)
{
  getxln(din,TG6_CODE,"TG6arm");
	cmd[0] = TG6_ARM;
  cmd[1] = snum;
  standform(cmd, 2);
}

void TG6go(qint din)
{
  getxln(din,TG6_CODE,"TG6go");
  shortform(TG6_GO);
}

void TG6tgo(qint din)
{
  getxln(din,TG6_CODE,"TG6tgo");
  shortform(TG6_TGO);
}

void TG6stop(qint din)
{
  getxln(din,TG6_CODE,"TG6stop");
  shortform(TG6_STOP);
}

void TG6baserate( qint din, qint brcode)
{
  getxln(din,TG6_CODE,"TG6baserate");
  cmd[0] = TG6_BASERATE;
  cmd[1] = brcode;
  standform(cmd, 2);
}

void TG6new( qint din, qint snum, qint lgth, qint dmask)
{
  getxln(din,TG6_CODE,"TG6new");
	cmd[0] = TG6_NEW;
  cmd[1] = snum;
  cmd[2] = lgth;
  cmd[3] = lgth >> 8;
  cmd[4] = dmask;
  standform(cmd, 5);
}

void TG6high( qint din, qint snum, qint _begin, qint _end, qint hmask)
{
  getxln(din,TG6_CODE,"TG6high");
  cmd[0] = TG6_HIGH;
  cmd[1] = snum;
  cmd[2] = _begin;
  cmd[3] = _begin >> 8;
  cmd[4] = _end;
  cmd[5] = _end >> 8;
  cmd[6] = hmask;
  standform(cmd, 7);
}

void TG6low( qint din, qint snum, qint _begin, qint _end, qint lmask)
{
  getxln(din,TG6_CODE,"TG6low");
  cmd[0] = TG6_LOW;
  cmd[1] = snum;
  cmd[2] = _begin;
  cmd[3] = _begin >> 8;
  cmd[4] = _end;
  cmd[5] = _end >> 8;
  cmd[6] = lmask;
  standform(cmd, 7);
}

void TG6value( qint din, qint snum, qint _begin, qint _end, qint val)
{
  getxln(din,TG6_CODE,"TG6value");
  cmd[0] = TG6_VALUE;
  cmd[1] = snum;
  cmd[2] = _begin;
  cmd[3] = _begin >> 8;
  cmd[4] = _end;
  cmd[5] = _end >> 8;
  cmd[6] = val;
  standform(cmd, 7);
}

void TG6dup( qint din, qint snum, qint s_beg, qint s_end, qint d_beg, qint ndups, qint dmask)
{
  getxln(din,TG6_CODE,"TG6dup");
  cmd[0] = TG6_DUP;
  cmd[1] = snum;
  cmd[2] = s_beg;
  cmd[3] = s_beg >> 8;
  cmd[4] = s_end;
  cmd[5] = s_end >> 8;
  cmd[6] = d_beg;
  cmd[7] = d_beg >> 8;
  cmd[8] = ndups;
  cmd[9] = ndups >> 8;
  cmd[10] = dmask;
  standform(cmd, 11);
}

void TG6reps( qint din, qint rmode, qint nreps)
{
  getxln(din,TG6_CODE,"TG6reps");
  cmd[0] = TG6_REPS;
	cmd[1] = rmode;
  cmd[2] = nreps;
  cmd[3] = nreps >> 8;
  standform(cmd, 4);
}

qint  TG6status(qint din)
{
  qint val;

  getxln(din,TG6_CODE,"TG6status");
  tdtdisab();
  shortform(TG6_STATUS);
  val = (qint)get8();
  enable();
  return(val);
}

/********************************************************************
  Auto Detecting D/A Calls
 ********************************************************************/
char nda[20]={"No D/A device IDed."};
void DAclear( qint din)
{

  if (XB1device(DD1_CODE, din)!=0)
    dadev = DD1_CODE;
  else
    if (XB1device(DA1_CODE, din)!=0)
      dadev = DA1_CODE;
    else
      if (XB1device(DA3_CODE, din)!=0)
	dadev = DA3_CODE;
      else
	if (XB1device(PD1_CODE, din)!=0)
	  dadev = PD1_CODE;

  switch(dadev)
  {
    case DD1_CODE:
      DD1clear(din);
      break;

    case DA1_CODE:
      DA1clear(din);
      break;

    case DA3_CODE:
      DA3clear(din);
      break;

    case PD1_CODE:
      PD1clear(din);
      break;

    default:
      xcaller = (char *)&"DAclear";
      showerr("No D/A responding on XBUS.");
  }
}

void DAgo( qint din)
{
  switch(dadev)
  {
    case DD1_CODE:
      DD1go(din);
      break;

    case DA1_CODE:
      DA1go(din);
      break;

    case DA3_CODE:
      DA3go(din);
      break;

    case PD1_CODE:
      PD1go(din);
      break;

    default:
      xcaller = (char *)&"DAgo";
      showerr(nda);
  }
}

void DAtgo( qint din)
{
  switch(dadev)
  {
    case DD1_CODE:
      DD1tgo(din);
      break;

    case DA1_CODE:
      DA1tgo(din);
      break;

    case DA3_CODE:
      DA3tgo(din);
      break;

    case PD1_CODE:
      PD1tgo(din);
      break;

    default:
      xcaller = (char *)&"DAtgo";
      showerr(nda);
  }
}

void DAstop( qint din)
{
  switch(dadev)
  {
    case DD1_CODE:
			DD1stop(din);
      break;

    case DA1_CODE:
      DA1stop(din);
      break;

    case DA3_CODE:
      DA3stop(din);
      break;

    case PD1_CODE:
      PD1stop(din);
      break;

    default:
      xcaller = (char *)&"DAstop";
      showerr(nda);
  }
}

void DAarm( qint din)
{
  switch(dadev)
  {
    case DD1_CODE:
      DD1arm(din);
      break;

    case DA1_CODE:
      DA1arm(din);
      break;

    case DA3_CODE:
      DA3arm(din);
      break;

    case PD1_CODE:
      PD1arm(din);
      break;

    default:
			xcaller = (char *)&"DAarm";
      showerr(nda);
  }
}

void DAmode( qint din, qint mcode)
{
  switch(dadev)
  {
    case DD1_CODE:
      DD1mode(din, mcode);
      break;

    case DA1_CODE:
      DA1mode(din, mcode);
      break;

    case DA3_CODE:
      DA3mode(din, mcode);
      break;

    case PD1_CODE:
      PD1mode(din, mcode);
      break;

    default:
      xcaller = (char *)&"DAmode";
      showerr(nda);
  }
}

void DAsrate( qint din, float srate)
{
  switch(dadev)
  {
    case DD1_CODE:
      DD1srate(din, srate);
      break;

    case DA1_CODE:
      DA1srate(din, srate);
      break;

    case DA3_CODE:
      DA3srate(din, srate);
      break;

    case PD1_CODE:
      PD1srate(din, srate);
      break;

    default:
      xcaller = (char *)&"DAsrate";
      showerr("No D/A device IDed.");
  }
}

float DAsperiod( qint din, float sper)
{
  switch(dadev)
  {
    case DD1_CODE:
      return(DD1speriod(din, sper));

		case DA1_CODE:
			return(DA1speriod(din, sper));

		case DA3_CODE:
			return(DA3speriod(din, sper));

		case PD1_CODE:
			return(PD1speriod(din, sper));

		default:
			xcaller = (char *)&"DAsperiod";
			showerr("No D/A device IDed.");
	}
  return(0);
}

void DAclkin( qint din, qint scode)
{
  switch(dadev)
	{
    case DD1_CODE:
      DD1clkin(din, scode);
      break;

    case DA1_CODE:
      DA1clkin(din, scode);
      break;

    case DA3_CODE:
      DA3clkin(din, scode);
      break;

    case PD1_CODE:
      PD1clkin(din, scode);
      break;

    default:
      xcaller = (char *)&"DAclkin";
      showerr(nda);
	}
}

void DAclkout( qint din, qint dcode)
{
  switch(dadev)
  {
    case DD1_CODE:
			DD1clkout(din, dcode);
      break;

    case DA1_CODE:
      DA1clkout(din, dcode);
      break;

    case DA3_CODE:
      DA3clkout(din, dcode);
      break;

    case PD1_CODE:
      PD1clkout(din, dcode);
      break;

    default:
      xcaller = (char *)&"DAclkout";
      showerr(nda);
  }
}

void DAnpts( qint din, long npts)
{
  switch(dadev)
  {
    case DD1_CODE:
      DD1npts(din, npts);
      break;

    case DA1_CODE:
      DA1npts(din, npts);
      break;

    case DA3_CODE:
      DA3npts(din, npts);
			break;

    case PD1_CODE:
      PD1npts(din, npts);
      break;

    default:
      xcaller = (char *)&"DAnpts";
      showerr(nda);
  }
}

void DAmtrig( qint din)
{
  switch(dadev)
  {
    case DD1_CODE:
			DD1mtrig(din);
      break;

    case DA1_CODE:
      DA1mtrig(din);
      break;

    case DA3_CODE:
      DA3mtrig(din);
      break;

    case PD1_CODE:
      PD1mtrig(din);
      break;

    default:
      xcaller = (char *)&"DAmtrig";
      showerr(nda);
	}
}

void DAstrig( qint din)
{
  switch(dadev)
  {
    case DD1_CODE:
      DD1strig(din);
      break;

    case DA1_CODE:
      DA1strig(din);
      break;

    case DA3_CODE:
      DA3strig(din);
      break;

    case PD1_CODE:
      PD1strig(din);
      break;

    default:
	  xcaller = (char *)&"DAstrig";
      showerr(nda);
  }
}

qint DAstatus(qint din)
{
  switch(dadev)
  {
    case DD1_CODE:
      return(DD1status(din));

    case DA1_CODE:
      return(DA1status(din));

    case DA3_CODE:
	  return(DA3status(din));

    case PD1_CODE:
	  return(PD1status(din));

    default:
      xcaller = (char *)&"DAstatus";
      showerr(nda);
  }
  return(0);
}

void DAreps( qint din, unsigned qint nreps)
{
  switch(dadev)
  {
    case DD1_CODE:
      DD1reps(din, nreps);
      break;

    case DA1_CODE:
			DA1reps(din, nreps);
      break;

    case DA3_CODE:
      DA3reps(din, nreps);
      break;

    case PD1_CODE:
      PD1reps(din, nreps);
			break;

    default:
      xcaller = (char *)&"DAreps";
      showerr(nda);
  }
}


/********************************************************************
  Auto Detecting A/D Calls
 ********************************************************************/
void ADclear( qint din)
{

  if (XB1device(DD1_CODE, din)!=0)
    addev = DD1_CODE;
  else
    if (XB1device(AD1_CODE, din)!=0)
      addev = AD1_CODE;
    else
      if (XB1device(AD2_CODE, din)!=0)
	addev = AD2_CODE;
      else
	if (XB1device(PD1_CODE, din)!=0)
	  addev = PD1_CODE;

  switch(addev)
  {
    case DD1_CODE:
      DD1clear(din);
      break;

    case AD1_CODE:
      AD1clear(din);
			break;

    case AD2_CODE:
      AD2clear(din);
      break;

    case PD1_CODE:
      PD1clear(din);
      break;

    default:
      xcaller = (char *)&"ADclear";
      showerr("No A/D responding on XBUS.");
  }
}

void ADgo( qint din)
{
  switch(addev)
  {
    case DD1_CODE:
      DD1go(din);
      break;

    case AD1_CODE:
      AD1go(din);
      break;

    case AD2_CODE:
      AD2go(din);
      break;

    case PD1_CODE:
      PD1go(din);
      break;

    default:
      xcaller = (char *)&"ADgo";
      showerr(nda);
	}
}

void ADtgo( qint din)
{
  switch(addev)
  {
    case DD1_CODE:
      DD1tgo(din);
      break;

    case AD1_CODE:
      AD1tgo(din);
      break;

    case AD2_CODE:
	  AD2tgo(din);
      break;

    case PD1_CODE:
	  PD1tgo(din);
      break;

    default:
      xcaller = (char *)&"ADtgo";
      showerr(nda);
  }
}

void ADstop( qint din)
{
	switch(addev)
  {
    case DD1_CODE:
      DD1stop(din);
      break;

    case AD1_CODE:
	  AD1stop(din);
      break;

    case AD2_CODE:
      AD2stop(din);
      break;

    case PD1_CODE:
      PD1stop(din);
      break;

    default:
      xcaller = (char *)&"ADstop";
      showerr(nda);
  }
}

void ADarm( qint din)
{
  switch(addev)
	{
    case DD1_CODE:
      DD1arm(din);
      break;

    case AD1_CODE:
      AD1arm(din);
      break;

    case AD2_CODE:
      AD2arm(din);
      break;

    case PD1_CODE:
      PD1arm(din);
      break;

    default:
      xcaller = (char *)&"ADarm";
      showerr(nda);
  }
}

void ADmode( qint din, qint mcode)
{
  switch(addev)
  {
    case DD1_CODE:
      DD1mode(din, mcode);
      break;

    case AD1_CODE:
      AD1mode(din, mcode);
      break;

    case AD2_CODE:
      AD2mode(din, mcode);
      break;

    case PD1_CODE:
      PD1mode(din, mcode);
      break;

		default:
      xcaller = (char *)&"ADmode";
      showerr(nda);
  }
}

void ADsrate( qint din, float srate)
{
  switch(addev)
  {
    case DD1_CODE:
      DD1srate(din, srate);
      break;

    case AD1_CODE:
      AD1srate(din, srate);
      break;

    case AD2_CODE:
      AD2srate(din, srate);
      break;

    case PD1_CODE:
      PD1srate(din, srate);
      break;

    default:
      xcaller = (char *)&"ADsrate";
      showerr(nda);
  }
}

float ADsperiod( qint din, float sper)
{
  switch(addev)
	{
		case DD1_CODE:
			return(DD1speriod(din, sper));

		case AD1_CODE:
			return(AD1speriod(din, sper));

		case AD2_CODE:
			return(AD2speriod(din, sper));

		case PD1_CODE:
			return(PD1speriod(din, sper));

		default:
			xcaller = (char *)&"ADsperiod";
			showerr(nda);
	}
	return(0);
}

void ADclkin( qint din, qint scode)
{
  switch(addev)
  {
    case DD1_CODE:
			DD1clkin(din, scode);
      break;

    case AD1_CODE:
      AD1clkin(din, scode);
      break;

    case AD2_CODE:
      AD2clkin(din, scode);
      break;

    default:
      xcaller = (char *)&"ADclkin";
      showerr(nda);
  }
}

void ADclkout( qint din, qint dcode)
{
  switch(addev)
  {
    case DD1_CODE:
      DD1clkout(din, dcode);
      break;

    case AD1_CODE:
      AD1clkout(din, dcode);
      break;

    case AD2_CODE:
      AD2clkout(din, dcode);
      break;

    case PD1_CODE:
      PD1clkout(din, dcode);
      break;

    default:
	  xcaller = (char *)&"ADclkout";
      showerr(nda);
  }
}

void ADnpts( qint din, long npts)
{
  switch(addev)
  {
    case DD1_CODE:
      DD1npts(din, npts);
      break;

    case AD1_CODE:
      AD1npts(din, npts);
      break;

    case AD2_CODE:
	  AD2npts(din, npts);
      break;

    case PD1_CODE:
	  PD1npts(din, npts);
      break;

    default:
      xcaller = (char *)&"ADnpts";
      showerr(nda);
  }
}

void ADmtrig( qint din)
{
  switch(addev)
  {
    case DD1_CODE:
      DD1mtrig(din);
      break;

    case AD1_CODE:
      AD1mtrig(din);
      break;

    case AD2_CODE:
      AD2mtrig(din);
      break;

    case PD1_CODE:
      PD1mtrig(din);
      break;

    default:
      xcaller = (char *)&"ADmtrig";
      showerr(nda);
  }
}

void ADstrig( qint din)
{
  switch(addev)
	{
    case DD1_CODE:
      DD1strig(din);
      break;

    case AD1_CODE:
      AD1strig(din);
      break;

    case AD2_CODE:
      AD2strig(din);
      break;

    case PD1_CODE:
      PD1strig(din);
      break;

    default:
      xcaller = (char *)&"ADstrig";
			showerr(nda);
  }
}

qint ADstatus(qint din)
{
  switch(addev)
  {
    case DD1_CODE:
      return(DD1status(din));

	case AD1_CODE:
      return(AD1status(din));

    case AD2_CODE:
      return(AD2status(din));

    case PD1_CODE:
      return(PD1status(din));

    default:
      xcaller = (char *)&"ADstatus";
      showerr(nda);
  }
  return(0);
}

void ADreps( qint din, unsigned qint nreps)
{
  switch(addev)
  {
    case DD1_CODE:
      DD1reps(din, nreps);
      break;

    case AD1_CODE:
      AD1reps(din, nreps);
      break;

    case AD2_CODE:
      AD2reps(din, nreps);
      break;

    case PD1_CODE:
      PD1reps(din, nreps);
      break;

    default:
      xcaller = (char *)&"ADreps";
      showerr(nda);
	}
}


/********************************************************************
	PD1 Procedures
 ********************************************************************/
unsigned qint v[100];
qint DSPid[32];
qint DSPin[32];
qint DSPinL[32];
qint DSPinR[32];
qint DSPout[32];
qint DSPoutL[32];
qint DSPoutR[32];
qint COEF[32];
qint DELin[4];
qint DELout[32][4];
qint TAP[32][4];
qint DAC[4];
qint ADC[4];
qint IB[32];
qint OB[32];
qint IREG[32];

void PD1clear( qint din)
{
  getxln(din,PD1_CODE,"PD1clear");
  shortform(PD1_CLEAR);
  PD1resetRTE(din);
  PD1clrIO(din);
  PD1clrsched(din);
}

void PD1go( qint din)
{
  getxln(din,PD1_CODE,"PD1go");
  shortform(PD1_GO);
}

void PD1stop( qint din)
{
  getxln(din,PD1_CODE,"PD1stop");
  shortform(PD1_STOP);
}

void PD1arm( qint din)
{
  getxln(din,PD1_CODE,"PD1arm");
  shortform(PD1_ARM);
  PD1flushRTE(din);
}

void PD1nstrms( qint din, qint nDAC, qint nADC)
{
  getxln(din,PD1_CODE,"PD1nstrms");
  cmd[0] = PD1_NSTRMS;
  cmd[1] = nDAC;
  cmd[2] = 0;
  cmd[3] = nADC;
  cmd[4] = 0;
  standform(cmd, 5);
  PD1nstrmsRTE( din, nDAC, nADC);
}

void PD1mode( qint din, qint mode)
{
  qint ndacs,nadcs;

  xcaller = (char *)&"PD1mode";

  PD1clrsched(din);
  switch(mode & 0x3)
  {
    case DAC1:	   ndacs = 1;  break;
    case DAC2:	   ndacs = 1;  break;
    case DUALDAC:  ndacs = 2;  break;
    default:	   ndacs = 0;
  }
  switch(mode & 0xc)
  {
    case ADC1:	   nadcs = 1;  break;
    case ADC2:	   nadcs = 1;  break;
    case DUALADC:  nadcs = 2;  break;
    default:	   nadcs = 0;
  }
  PD1nstrms(din, ndacs, nadcs);

  switch(mode & 0x3)
  {
    case DAC1:
      PD1specIB(din, IB[0], DAC[0]);
      break;
    case DAC2:
      PD1specIB(din, IB[0], DAC[1]);
      break;
    case DUALDAC:
      PD1specIB(din, IB[0], DAC[0]);
      PD1specIB(din, IB[1], DAC[1]);
      break;
  }

  switch(mode & 0xc)
  {
    case ADC1:
      PD1specOB(din, OB[0], ADC[0]);
      break;
    case ADC2:
      PD1specOB(din, OB[0], ADC[1]);
      break;
    case DUALADC:
      PD1specOB(din, OB[0], ADC[0]);
      PD1specOB(din, OB[1], ADC[1]);
      break;
  }
}

void PD1srate( qint din, float srate)
{
  unsigned qint v;
	getxln(din,PD1_CODE,"PD1srate");
  v = (qint)((10.0*srate) + 0.5);
  cmd[0] = PD1_SRATE;
  cmd[1] = v;
  cmd[2] = v >> 8;
  standform(cmd, 3);
}

float PD1speriod( qint din, float sper)
{
	unsigned qint v;
	getxln(din,PD1_CODE,"PD1speriod");
	v = (qint)((12.5*sper) + 0.5);
	cmd[0] = PD1_SPERIOD;
	cmd[1] = v;
	cmd[2] = v >> 8;
	standform(cmd, 3);
	return(v*0.08);
}

void PD1clkin( qint din, qint scode)
{
  getxln(din,PD1_CODE,"PD1clkin");
  cmd[0] = PD1_CLKIN;
  cmd[1] = scode;
  cmd[2] = 0;
  standform(cmd, 3);
}

void PD1clkout( qint din, qint dcode)
{
  getxln(din,PD1_CODE,"PD1clkout");
  cmd[0] = PD1_CLKOUT;
  cmd[1] = dcode;
  cmd[2] = 0;
  standform(cmd, 3);
}

void PD1npts( qint din, long npts)
{
	getxln(din,PD1_CODE,"PD1npts");
  cmd[0] = PD1_NPTS;
  cmd[1] = npts;
  cmd[2] = npts >> 8;
  cmd[3] = npts >> 16;
  cmd[4] = npts >> 24;
	standform(cmd, 5);
}

void PD1mtrig( qint din)
{
  getxln(din,PD1_CODE,"PD1mtrig");
  shortform(PD1_MTRIG);
}

void PD1strig( qint din)
{
  getxln(din,PD1_CODE,"PD1strig");
  shortform(PD1_STRIG);
}

qint PD1status(qint din)
{
  qint val;

	getxln(din,PD1_CODE,"PD1status");
	tdtdisab();
	shortform(PD1_STATUS);
	val = (qint)get8();
	enable();
	return(val);
}

void PD1reps( qint din, unsigned qint nreps)
{
	getxln(din,PD1_CODE,"PD1reps");
	cmd[0] = PD1_REPS;
	cmd[1] = nreps;
	cmd[2] = nreps >> 8;
	standform(cmd, 3);
}

void PD1tgo( qint din)
{
	getxln(din,PD1_CODE,"PD1tgo");
	shortform(PD1_TGO);
}

void PD1zero( qint din)
{
	getxln(din,PD1_CODE,"PD1zero");
	shortform(PD1_ZERO);
}

void PD1xcmd( qint din, unsigned qint v[], qint n, char *caller)
{
	qint i;

	getxln(din,PD1_CODE,caller);
	cmd[0] = PD1_XCMD;
	cmd[1] = n;
	for(i=0; i<n; i++)
	{
		cmd[i*2+2] = v[i];
		cmd[i*2+3] = v[i] >> 8;
	}
	standform(cmd, (n+1)<<1);
}

/* Note you must call play before calling this procedure */
void PD1xdata( qint din, qint data_id)
{
	getxln(din,PD1_CODE,"PD1xdata");

	cmd[0] = PD1_XDATA;
	cmd[1] = data_id;
	cmd[2] = data_id >> 8;
	standform(cmd, 3);
}

void PD1xboot( qint din)
{
	getxln(din,PD1_CODE,"PD1xboot");
	shortform(PD1_XBOOT);
}

long PD1checkDSPS( qint din)
{
    long rv;
	getxln(din,PD1_CODE,"PD1checkDSPS");
	tdtdisab();
	shortform(PD1_CHECKDSPS);
	rv = (unsigned long)get8();
	rv = rv | ((unsigned long)get8() << 8);
	rv = rv | ((unsigned long)get8() << 16);
	rv = rv | ((unsigned long)get8() << 24);
	enable();
	return(rv);
}

qint PD1what( qint din, qint rcode, qint dnum, char *caller)
{
  qint rv;
	getxln(din,PD1_CODE,caller);
	cmd[0] = PD1_WHAT;
	cmd[1] = rcode;
	cmd[2] = rcode>>8;
	cmd[3] = dnum;
	cmd[4] = dnum>>8;
	tdtdisab();
	standform(cmd, 5);
	rv = get8();
	rv = rv | (get8() << 8);
	enable();
	return(rv);
}

void PD1writeDSP(qint din, unsigned char *vl, long dmask)
{
    qint i;

	v[0] = dmask & 0xffff;
	v[1] = dmask >> 16;

	for(i=0; i<8; i++)
	{
	  v[i+2] = (unsigned qint)vl[i*2] | ((unsigned qint)vl[i*2+1] << 8);
	}
	v[2+8] = RTE_WRITEDSP;
	PD1xcmd(din, v, 11, "PD1writeDSP");
}

void PD1bootDSP(qint din, long dmask, char *fname)
{
    unsigned char tbuf[20];
    qint i;
    FILE *fff;

	getxln(din,PD1_CODE,"PD1bootDSP");
	cmd[0] = PD1_XBOOT;
  cmd[1] = dmask;
  cmd[2] = dmask >> 8;
  cmd[3] = dmask >> 16;
  cmd[4] = dmask >> 24;
	standform(cmd, 5);



	fff = fopen(fname,"rb");
	if (!fff)
	{
	showerr("File not found");
	  exit(0);
	}

	fseek(fff, 256, SEEK_SET);

	tbuf[0] = 99;
	tbuf[1] = 99;
	fread(&tbuf[2], 1, 14, fff);
	PD1writeDSP(din, tbuf, dmask);

	for(i=0; i<225; i++)
	{
	  fread(tbuf, 1, 16, fff);
    PD1writeDSP(din, tbuf, dmask);
	}
	fclose(fff);
}

/********************************************************************
	PD1 Supplimental Procedures etc
 ********************************************************************/
void PD1resetRTE(qint din)
{
	qint i,j;

	for(i=0; i<32; i++)
	{
		DSPid[i] = DSPID_BASE + (DSPID_IND * i);

		DSPin[i] = DSPINL_BASE + (DSPINL_IND * i);
		DSPinL[i] = DSPINL_BASE + (DSPINL_IND * i);
		DSPinR[i] = DSPINR_BASE + (DSPINR_IND * i);

		DSPout[i] = DSPOUTL_BASE + (DSPOUTL_IND * i);
		DSPoutL[i] = DSPOUTL_BASE + (DSPOUTL_IND * i);
		DSPoutR[i] = DSPOUTR_BASE + (DSPOUTR_IND * i);

		COEF[i] = COEF_BASE + (COEF_IND * i);

		IB[i] = IB_BASE + (IB_IND * i);
		OB[i] = OB_BASE + (OB_IND * i);
		IREG[i] = IREG_BASE + (IREG_IND * i);
	}

	for(j=0; j<4; j++)
	{
		DAC[j] = DAC_BASE + (DAC_IND * j);
		ADC[j] = ADC_BASE + (ADC_IND * j);

		DELin[j] = DELIN_BASE + (DELIN_IND * j);
		for(i=0; i<32; i++)
		{
			DELout[i][j] = DELOUT_BASE + (DELOUT_IND1 * j) + (DELOUT_IND2 * i);
			TAP[i][j] = TAP_BASE + (TAP_IND1 * j) + (TAP_IND2 * i);
		}
	}

	v[0] = RTE_INTRST;
	PD1xcmd(din, v, 1, "PD1resetRTE");
}

void PD1nstrmsRTE(qint din, qint nIC, qint nOG)
{
	v[0] = nIC;
	v[1] = nOG;
	v[2] = RTE_NSTRMS;
	PD1xcmd(din, v, 3, "PD1nstrmsRTE");
}

void PD1flushRTE(qint din)
{
	v[0] = RTE_FLUSH;
	PD1xcmd(din, v, 1, "PD1flushRTE");
}

void PD1syncall(qint din)
{
	v[0] = RTE_SYNCALL;
	PD1xcmd(din, v, 1, "PD1syncall");
}

void PD1clrIO(qint din)
{
	v[0] = RTE_CLRIO;
	PD1xcmd(din, v, 1, "PD1clrIO");
}

void PD1setIO(qint din, float dt1, float dt2, float at1, float at2)
{
	v[0] = (int)(3276.7 * dt1);
	v[1] = (int)(3276.7 * dt2);
	v[2] = (int)(3276.7 * at1);
	v[3] = (int)(3276.7 * at2);
	v[4] = RTE_SETIO;
	PD1xcmd(din, v, 5, "PD1setIO");
}

void PD1clrDEL(qint din, qint ch1, qint ch2, qint ch3, qint ch4)
{
	v[0] = ch1;
	v[1] = ch2;
	v[2] = ch3;
	v[3] = ch4;
	v[4] = RTE_CLRDEL;
	PD1xcmd(din, v, 5, "PD1clrDEL");
}

void PD1interpDEL(qint din, qint ifact)
{
	v[0] = ifact;
	v[1] = RTE_INTDEL;
	PD1xcmd(din, v, 2, "PD1interpDEL");
}

void PD1setDEL(qint din, qint tap, qint dly)
{
	v[0] = tap;
	v[1] = dly;
	v[2] = RTE_SETDEL;
	PD1xcmd(din, v, 3, "PD1setDEL");
}

void PD1latchDEL(qint din)
{
	v[0] = RTE_LOADDEL;
	PD1xcmd(din, v, 1, "PD1latchDEL");
}

void PD1flushDEL(qint din)
{
	v[0] = RTE_FLUSHDEL;
	PD1xcmd(din, v, 1, "PD1flushDEL");
}

void PD1clrsched(qint din)
{
	v[0] = RTE_CLRSCHED;
	PD1xcmd(din, v, 1, "PD1clrsched");
}

void PD1addsimp(qint din, qint src, qint des)
{
  qint i;
  for(i=0; i<20; i++)
    v[i] = 0;
	v[0] = src;
	v[1] = des;
	v[2] = RTE_ADDSIMP;
	PD1xcmd(din, v, 3, "PD1addsimp");
}

qint PD1round(float v)
{
	if (v<0)
		return((qint)(v-0.5));
	return((qint)(v+0.5));
}

void PD1addmult(qint din, qint src[], float sf[], qint n, qint des)
{
	qint i,z;

	if ((n<<1)>30)
	{
		showerr("Too many sources specified (max = 15)");
		exit(0);
	}
	z = 0;
	for(i=0; i<n; i++)
	{
		v[z++] = src[i];
		v[z++] = PD1round(-32768.0 * sf[i]);
	}
	v[z++] = 0;
	v[z++] = des;
	v[z++] = RTE_ADDMULT;
	PD1xcmd(din, v, z, "PD1addmult");
}

void PD1specIB(qint din, qint IBnum, qint desaddr)
{
	v[0] = IBnum;
	v[1] = desaddr;
	v[2] = RTE_SPECIB;
	PD1xcmd(din, v, 3, "PD1specIB");
}

void PD1specOB(qint din, qint OBnum, qint srcaddr)
{
	v[0] = OBnum;
	v[1] = srcaddr;
	v[2] = RTE_SPECOB;
	PD1xcmd(din, v, 3, "PD1specOB");
}

void PD1idleDSP(qint din, long dmask)
{
	v[0] = dmask & 0xffff;
	v[1] = dmask >> 16;
	v[2] = RTE_IDLEDSP;
	PD1xcmd(din, v, 3, "PD1idleDSP");
}

void PD1resetDSP(qint din, long dmask)
{
	v[0] = dmask & 0xffff;
	v[1] = dmask >> 16;
	v[2] = RTE_RESETDSP;
	PD1xcmd(din, v, 3, "PD1resetDSP");
}

void PD1bypassDSP(qint din, long dmask)
{
	v[0] = dmask & 0xffff;
	v[1] = dmask >> 16;
	v[2] = RTE_BYPASSDSP;
	PD1xcmd(din, v, 3, "PD1bypassDSP");
}

void PD1lockDSP(qint din, long dmask)
{
	v[0] = dmask & 0xffff;
	v[1] = dmask >> 16;
	v[2] = RTE_LOCKDSP;
	PD1xcmd(din, v, 3, "PD1lockDSP");
}

void PD1interpDSP(qint din, qint ifact, long dmask)
{
	v[0] = dmask & 0xffff;
	v[1] = dmask >> 16;
	v[2] = ifact;
	v[3] = RTE_INTERPDSP;
	PD1xcmd(din, v, 4, "PD1interpDSP");
}

qint PD1whatDEL(qint din)
{
	return(PD1what(din, RTE_WHATDEL, 0, "PD1whatDEL"));
}

qint PD1whatIO(qint din)
{
	return(PD1what(din, RTE_WHATIO, 0, "PD1whatIO"));
}

qint PD1whatDSP(qint din, qint dn)
{
	return(PD1what(din, RTE_WHATDSP, dn, "PD1whatDSP"));
}


/********************************************************************
	SS1 Procedures
 ********************************************************************/
void SS1clear( qint din)
{
	getxln(din,SS1_CODE,"SS1clear");
	shortform(SS1_CLEAR);
}

void SS1gainon( qint din)
{
	getxln(din,SS1_CODE,"SS1gainon");
	shortform(SS1_GAINON);
}

void SS1gainoff( qint din)
{
	getxln(din,SS1_CODE,"SS1gainoff");
	shortform(SS1_GAINOFF);
}

void SS1mode( qint din, qint mcode)
{
	getxln(din,SS1_CODE,"SS1mode");
	cmd[0] = SS1_MODE;
	cmd[1] = mcode;
	standform(cmd, 2);
}

void SS1select( qint din, qint chan, qint inpn)
{
	getxln(din,SS1_CODE,"SS1select");
	cmd[0] = SS1_SELECT;
	cmd[1] = chan;
	cmd[2] = inpn;
	standform(cmd, 3);
}



/********************************************************************
	PM1 Procedures
 ********************************************************************/

void PM1clear( qint din)
{
	getxln(din,PM1_CODE,"PM1clear");
	shortform(PM1_CLEAR);
}

void PM1config( qint din, qint config)
{
	getxln(din,PM1_CODE,"PM1config");
	cmd[0] = PM1_CONFIG;
	cmd[1] = config;
	standform(cmd, 2);
}

void PM1mode( qint din, qint cmode)
{
	getxln(din,PM1_CODE,"PM1mode");
	cmd[0] = PM1_MODE;
	cmd[1] = cmode;
	standform(cmd, 2);
}

void PM1spkon( qint din, qint sn)
{
	getxln(din,PM1_CODE,"PM1spkon");
	cmd[0] = PM1_SPKON;
	cmd[1] = sn-1;
	standform(cmd, 2);
}

void PM1spkoff( qint din, qint sn)
{
	getxln(din,PM1_CODE,"PM1spkoff");
	cmd[0] = PM1_SPKOFF;
	cmd[1] = sn-1;
	standform(cmd, 2);
}


/********************************************************************
	HTI Procedures
 ********************************************************************/
qint isoflag;

void HTIclear( qint din)
{
	getxln(din,HTI_CODE,"HTIclear");
	shortform(HTI_CLEAR);
	isoflag = 0;
}

void HTIgo( qint din)
{
	getxln(din,HTI_CODE,"HTIgo");
	shortform(HTI_GO);
}


void HTIstop( qint din)
{
	getxln(din,HTI_CODE,"HTIstop");
	shortform(HTI_STOP);
}

void HTIreadAER( qint din, float *az, float *el, float *roll)
{
	qint v[5];
	qint i;
	float k;

	getxln(din,HTI_CODE,"HTIreadAER");
	tdtdisab();
	shortform(HTI_READAER);
  if (isoflag)
    k = 0.00699401855;
  else
    k = 0.00549316406;
	for(i=0; i<3; i++)
	{
		v[i] = ((unsigned qint)get8()<<8);
		v[i] = v[i] | get8();
	}
	enable();
	*az   = v[0]*k;
	*el   = v[1]*k;
	*roll = v[2]*k;
}

void HTIreadXYZ( qint din, float *x, float *y, float *z)
{
	qint v[5];
	qint i;
	float k;

	getxln(din,HTI_CODE,"HTIreadXYZ");
	tdtdisab();
	shortform(HTI_READXYZ);
  if (isoflag)
    k = 0.00507568359;
  else
    k = 0.00915527344;
	for(i=0; i<3; i++)
	{
		v[i] = ((unsigned qint)get8()<<8);
		v[i] = v[i] | get8();
	}
	enable();
	*x = v[0]*k;
	*y = v[1]*k;
	*z = v[2]*k;
}


void HTIwriteraw( qint din, char cmdstr[])
{
	qint i;

	getxln(din,HTI_CODE,"HTIwriteraw");
	cmd[0] = HTI_WRITERAW;
	i=0;
	do
	{
		cmd[i+1] = cmdstr[i];
		i++;
	}while(cmd[i]);
	standform(cmd, i+1);
}

void HTIsetraw( qint din, qint nbytes, char c1, char c2)
{
	getxln(din,HTI_CODE,"HTIsetraw");
	cmd[0] = HTI_SETRAW;
	cmd[1] = nbytes;
	cmd[2] = c1;
	cmd[3] = c2;
	standform(cmd, 4);
}

void HTIreadraw( qint din, qint maxchars, char *buf)
{
	qint i;

	getxln(din,HTI_CODE,"HTIreadraw");
	tdtdisab();
	shortform(HTI_READRAW);
	i=0;
	do
	{
		buf[i] = get8();
		i++;
	}while(buf[i-1] && i<maxchars);
	enable();
}

void HTIboresight( qint din)
{
	getxln(din,HTI_CODE,"HTIboresight");
	shortform(HTI_BORESIGHT);
}

void HTIshowparam( qint din, qint pid)
{
	getxln(din,HTI_CODE,"HTIshowparam");
	cmd[0] = HTI_SHOWPARAM;
	cmd[1] = pid;
	standform(cmd, 2);
}

void HTIreset( qint din)
{
	getxln(din,HTI_CODE,"HTIreset");
	shortform(HTI_RESET);
}

float HTIreadone( qint din, qint pid)
{
  qint v;
  float k;

	getxln(din,HTI_CODE,"HTIreadone");
	cmd[0] = HTI_READONE;
	cmd[1] = pid;
	tdtdisab();
	standform(cmd, 2);
	v = ((unsigned qint)get8()<<8);
	v = v | get8();
	enable();

	if (pid<4)
	{
    if (isoflag)
      k = 0.00699401855;
    else
      k = 0.00549316406;
	  return(v * k);
  }

  if (isoflag)
    k = 0.00507568359;
  else
    k = 0.00915527344;
  return(v * k);
}

void HTIfastAER( qint din, qint *az, qint *el, qint *roll)
{
	getxln(din,HTI_CODE,"HTIfastAER");
	tdtdisab();
	shortform(HTI_FASTAER);
	*az = (int)(char)get8();
	*el = (int)(char)get8();
	*roll = (int)(char)get8();
	enable();
}

void HTIfastXYZ( qint din, qint *x, qint *y, qint *z)
{
	getxln(din,HTI_CODE,"HTIfastXYZ");
	tdtdisab();
	shortform(HTI_FASTXYZ);
	*x = (int)(char)get8();
	*y = (int)(char)get8();
	*z = (int)(char)get8();
	enable();
}

int HTIgetecode( qint din)
{
  int ec;
	getxln(din,HTI_CODE,"HTIgetecode");
	tdtdisab();
	shortform(HTI_GETECODE);
	ec = (int)get8();
	enable();
	return(ec);
}

void HTIisISO( qint din)
{
	getxln(din,HTI_CODE,"HTIisISO");
	shortform(HTI_ISISO);
	isoflag = 1;
}


