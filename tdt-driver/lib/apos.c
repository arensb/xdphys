#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <stdio.h>
#include <string.h>

/* ----------------------------------------------------------------------- */
/* Platform-dependent stuff                                                */
/* ----------------------------------------------------------------------- */
#ifdef MSC

#include <dos.h>
#include <io.h>
#include <malloc.h>
#include <conio.h>
#define outportb(a,b) _outp(a,b)
#define outport(a,b) _outpw(a,b)
#define inportb(a) _inp(a)
#define inport(a) _inpw(a)

#else /* MSC */
/* ----------------------------------------------------------------------- */
#ifdef __linux__

#include <dos.h>

#define FP_OFF(fp)      ((unsigned short)((unsigned long)fp & 0xffff))
#define FP_SEG(fp)      ((unsigned short)((unsigned long)(fp) >> 16))
#define _MK_FP(seg,ofs) ((void *) \
			(((unsigned long)(seg) << 16)|(unsigned short)(ofs)))

/* ----------------------------------------------------------------------- */
#else /* __linux__ */

#include <alloc.h>
#include <io.h>
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

#include "apos.h"

/* ----------------------------------------------------------------------- */
/* Globals                                                                 */
/* ----------------------------------------------------------------------- */

int q,qq;

char    *caller;
char    *lastcall;
char    emess[300];
char    apid[5][10];
qint    apsel,qerror;
qint    aps[3];
qint    dspcfg[3];
qint    dspadr[3];
qint    dspadrx[3];
qint    dspdat[3];
qint    dsppcr[3];
qint    dsppcrh[3];
qint    dsppcrx[3];
qint    dspsync[3];
qint    cp[3];
qint    dummy[3];

qint    intset[3];
qint    intack[3];
qint    serdat[3];
qint    sercmd[3];

long    aptimeout;

qint    mrmode;

qint    ap_eflag, ap_emode;
char    ap_err[10][100];


/* ----------------------------------------------------------------------- */
#ifdef INTMOVE
void DataIn(unsigned qint ioport, unsigned qint qseg, unsigned qint qofs, unsigned int count)
{
   long i;
   qint *buf;

	 buf = (qint *)_MK_FP(qseg,qofs);
   for(i=0; i<count; i++)
     buf[i] = inport(ioport);
}

/* ----------------------------------------------------------------------- */
void DataOut(unsigned qint qseg, unsigned qint qofs, unsigned qint ioport, unsigned int count)
{
   long i;
   qint *buf;

   buf = (qint *)_MK_FP(qseg,qofs);
   for(i=0; i<count; i++)
     outport(ioport,buf[i]);
}

#endif


/******************************************************************
  User recoverable error handler
  ----------------------------------------------------------------**/
void GetErr(void)
{
  qint    ll,jj,kk,aflag;
  long    ii;

  kk=0;
  aflag=0;
  outport(dspdat[apsel],ERRACK);
  for(ii=0; ii<1000; ii++);
  do
  {
    ii=aptimeout;
    do
		{
      jj=inportb(dsppcrx[apsel]) & 0x20;
      ii--;
/* ----------------------------------------------------------------------- */
    }while((jj!=0x20) && (inport(cp[apsel])!=DONE) && (inport(cp[apsel])!=READY) && (ii!=0));
    if(ii==0)
    {
      strcpy(emess,"General device failure. (geterr)");
      aflag=1;
    }
    ll=inport(dspdat[apsel]) & 0xFF;
    if(jj==0x20)
    {
      emess[kk]=ll;
      kk++;
      emess[kk]=0;
      if(kk>200)
      {
	strcpy(emess,"General device failure. (geterr)");
	aflag=1;
      }
    }
  }while(inport(cp[apsel])!=DONE  && inport(cp[apsel])!=READY && !aflag);
}


/******************************************************************
  Internal Error handler
  ----------------------------------------------------------------**/
void APerror( char *estr)
{
  int ii;
  long jj;

  strcpy(emess,estr);

  jj = inport(cp[apsel]);

  if(jj==ERRFLAG )
    GetErr();
  else
    strcpy(lastcall,caller);

  ap_eflag = 1;
  sprintf(ap_err[0]," ");
  sprintf(ap_err[1],"   Array Processor Error >>>");
  sprintf(ap_err[2],"   Device:     %s",apid[apsel]);
  sprintf(ap_err[3],"   Operation:  %s",lastcall);
  sprintf(ap_err[4],"   Message:    %s",emess);
  sprintf(ap_err[5],"  ");
  sprintf(ap_err[6],"");

#ifndef WINDOWS
  if(!ap_emode)
  {
    ii=0;
    do
    {
      printf("%s\n",ap_err[ii]);
      ii++;
    }while(ap_err[ii][0]!=0);
    exit(0);
  }            
#endif
  
}



/******************************************************************
  Sends cmd to AP.  Checks for readiness to receive.
  ----------------------------------------------------------------**/
void cmdout(qint cmd)
{
  unsigned long  ii,jj;
  char  sss[200];

  ii=aptimeout;
  do
  {
    ii--;
    jj = inport(cp[apsel]);
		if(jj!=inport(cp[apsel]))
			jj=0;
		else
			if(jj!=inport(cp[apsel]))
				jj=0;
			else
				if(jj!=inport(cp[apsel]))
					jj=0;
	} while(jj!=READY && jj!=ERRFLAG && ii>0);

	if( ii==0 || jj==ERRFLAG )
	{
		sprintf(sss,"Device not ready to receive command. (%ld)",jj);
		APerror(sss);
	}

	if(!ap_eflag)
	{
		outport(cp[apsel],cmd);
		lastcall = (char *)caller;
		ii=aptimeout;
		do
		{
			ii--;
			jj = inport(cp[apsel]);
			if(jj!=inport(cp[apsel]))
				jj=0;
			else
				if(jj!=inport(cp[apsel]))
					jj=0;
				else
					if(jj!=inport(cp[apsel]))
						jj=0;
		} while(jj!=RECEIVED && jj!=READY  && jj!=ERRFLAG && ii>0);

    if( ii==0 || jj==ERRFLAG )
    {
      sprintf(sss,"General device failure. (%ld)",jj);
      APerror(sss);
    }
  }
}

/******************************************************************
  Initializes DSP and PC systems.
  ----------------------------------------------------------------**/
qint apinit( qint devtype )
{
  qint   i,j;
  char   *s;
  char   ss[30];
  qint   af;

  mrmode = 0;

  ap_emode = 0;
  ap_eflag = 0;
  
  caller = (char *)("apinit");

  af = 100;
  s = getenv("SPFACT");
  if (s!=NULL)
  {
    strcpy(ss,s);
    sscanf(ss,"%d",&af);
    if(af<1) af = 1;
    if(af>1000) af = 1000;
  }
  aptimeout = (long)TIMEOUT * af;

  if(devtype!=APa && devtype!=APb)
  {
    APerror("Illegal device type specified.  Use: APa or APb.");
  }  
  
  strcpy(apid[1],"APa");
  strcpy(apid[2],"APb");

  dsppcrx[1] = 0x22E;
  dspcfg[1]  = 0x232;
  intset[1]  = 0x233;
  intack[1]  = 0x234;
  serdat[1]  = 0x231;
  sercmd[1]  = 0x230;
  dummy[1]   = 0x237;

  dsppcrx[2] = 0x24E;
  dspcfg[2]  = 0x252;
  intset[2]  = 0x253;
  intack[2]  = 0x254;
  serdat[2]  = 0x251;
  sercmd[2]  = 0x250;
  dummy[2]   = 0x237;

  dspadr[1]  = 0x220;
  dspadrx[1] = 0x22B;
  dspdat[1]  = 0x222;
  dsppcr[1]  = 0x227;
  dsppcrh[1] = 0x22A;
  dspsync[1] = 0x22C;
  cp[1]      = 0x228;

  dspadr[2]  = 0x240;
  dspadrx[2] = 0x24B;
  dspdat[2]  = 0x242;
  dsppcr[2]  = 0x247;
  dsppcrh[2] = 0x24A;
  dspsync[2] = 0x24C;
  cp[2]      = 0x248;

  apsel = devtype & 0x03;

  io_init(dspadr[apsel]);

  i=0;
  do
  {
    outportb(dspcfg[apsel],4);

    for(j=0; j<1000; j++);

    outport(cp[apsel],0);
    j=inport(dspdat[apsel]);

    outport(dspsync[apsel],ACK);
    j=inport(dspsync[apsel]);

    outportb(dspcfg[apsel],6);
	
    for(j=0; j<1000; j++);

    outportb(dsppcrx[apsel],3);
    outportb(dsppcrh[apsel],2);
    outportb(dspcfg[apsel],7);

    for(j=0; j<1000; j++);
    i++;

  } while((inport(cp[apsel])==ERRFLAG) && (i<100));

  if(i>=100)
    APerror("Board detected in but not resetting");

  if(inport(cp[apsel])==READY)
  {
    outportb(dspcfg[apsel],15);
    outportb(intset[apsel],0);
    cmdout(INITx);
    aps[apsel]=1;
  }
  else
    aps[apsel]=0;

  return(aps[apsel]);
}


long rec24(void)
{
  unsigned qint   ii;
  unsigned long   jj,ll,hh;

  if(ap_eflag)
    return(0);

  if(mrmode)
    return(0);

  jj=aptimeout;
  do
  {
		jj--;
    ii=inport(dspsync[apsel]);
  } while(ii!=LOW_UP && jj!=0);

  if( jj==0 )
  {
    APerror("General device failure. (rec24)");
    return(0);
  }

  ll = inport(dspdat[apsel]);
  outport(dspsync[apsel],ACK);

  jj=aptimeout;

  do
  {
    jj--;
    ii=inport(dspsync[apsel]);
  } while(ii!=HIGH_UP && jj!=0);

  if( jj==0 )
  {
    APerror("General device failure. (rec24)");
    return(0);
  }

  hh = inport(dspdat[apsel]) & 0xff;
  outport(dspsync[apsel],ACK);

  hh=hh << 16;
  return(hh | ll);
}


float recflt(void)
{
  unsigned qint   ii;
  unsigned long   jj;
	unsigned qint   dbuf[2];
  float           *flt;

  if(ap_eflag)
    return(0);

  if(mrmode)
    return(0);

  flt=(float *)(&dbuf[0]);

  jj=aptimeout;
  do
  {
    jj--;
    ii=inport(dspsync[apsel]);
  }while(ii!=LOW_UP && jj!=0);
  if( jj==0 )
  {
    APerror("General device failure. (recflt)");
    return(0);
  }
  dbuf[0]=inport(dspdat[apsel]);
  outport(dspsync[apsel],ACK);

  jj=aptimeout;
  do
  {
    jj--;
    ii=inport(dspsync[apsel]);
  }while(ii!=HIGH_UP && jj!=0);
  if( jj==0 )
  {
    APerror("General device failure. (recflt)");
		return(0);
  }
  dbuf[1]=inport(dspdat[apsel]);
  outport(dspsync[apsel],ACK);

  return(*flt);
}


void send24(long val)
{
  unsigned qint   ii;
  unsigned long   jj;

  if(!ap_eflag)
  {
    jj=aptimeout;
    do
    {
      jj--;
      ii=inport(dspsync[apsel]);
    }while(ii!=ACK && jj!=0);
    if( jj==0 )
    {
      APerror("General device failure. (send24)");
    }
    else
    {
      outport(dspdat[apsel],(unsigned int)val);
      outport(dspsync[apsel],LOW_UP);

      jj=aptimeout;
      do
      {
				jj--;
	ii=inport(dspsync[apsel]);
      }while(ii!=ACK && jj!=0);
      if( jj==0 )
      {
	APerror("General device failure. (send24)");
      }
      else
      {
	outport(dspdat[apsel],(unsigned int)(val >> 16));
	outport(dspsync[apsel],HIGH_UP);
      }
    }
  }
}


void sendflt(float val)
{
  unsigned qint   ii;
  unsigned long   jj;
  unsigned long   *hld;

  if(!ap_eflag)
  {
    hld=(unsigned long *) &val;

    jj=aptimeout;
    do
    {
      jj--;
      ii=inport(dspsync[apsel]);
    }while(ii!=ACK && jj!=0);
    if( jj==0 )
		{
      APerror("General device failure. (sendflt)");
    }
    else
    {
      outport(dspdat[apsel],(unsigned int)*hld);
      outport(dspsync[apsel],LOW_UP);

      jj=aptimeout;
      do
      {
	jj--;
	ii=inport(dspsync[apsel]);
      }while(ii!=ACK && jj!=0);
      if( jj==0 )
      {
	APerror("General device failure. (sendflt)");
      }
      else
      {
	outport(dspdat[apsel],(unsigned int)(*hld >> 16));
	outport(dspsync[apsel],HIGH_UP);
      }
    }
  }
}


void P0(qint cmd, char idstr[])
{
  caller = (char *)idstr;

  cmdout(cmd);
}

void P1(qint cmd, float arg1, char idstr[])
{
  caller = (char *)idstr;
  cmdout(cmd);
  if(!ap_eflag)
    sendflt(arg1);
}


void P2(qint cmd, float arg1, float arg2, char idstr[])
{
  caller = (char *)idstr;
  cmdout(cmd);
  if(!ap_eflag)
  {
    sendflt(arg1);
    sendflt(arg2);
  }
}


/******************************************************************
  Interface to DataOut. Performs control of AP"s DMA and
  handles LONGINT number of data transfers.
  ----------------------------------------------------------------**/
void SendData(qint buf[], long adr, long nwds)
{
  if(!ap_eflag)
  {
    outport(dsppcrx[apsel],0x021B);
    outportb(dspadrx[apsel],(unsigned char)((adr >> 16) & 0xFF));
    outport(dspadr[apsel],(unsigned int)(adr & 0xFFFF));

		DataOut(FP_SEG(buf),FP_OFF(buf),dspdat[apsel],(unsigned int)nwds);

    outport(dsppcrx[apsel],0x0203);
  }
}


/******************************************************************
  Interface to DataIn. Performs control of AP"s DMA and
  handles LONGINT number of data transfers.
  ----------------------------------------------------------------**/
void ReceiveData(qint buf[], long adr, long nwds)
{
  if(!ap_eflag)
  {
    outport(dsppcrx[apsel],0x021b);
    outportb(dspadrx[apsel],(unsigned char)((adr >> 16) & 0xFF));
    outport(dspadr[apsel],(unsigned int)(adr & 0xFFFF));

    DataIn(dspdat[apsel],FP_SEG(buf),FP_OFF(buf),(unsigned qint)nwds);

    outport(dsppcrx[apsel],0x0203);

    (void)inport(dspdat[apsel]);
  }
}


/******************************************************************
  Pushes Integer variable or array on AP stack.  Npts is
  the number of elements.  Use npts==0 for scalars.
  ----------------------------------------------------------------**/
void push16(qint buf[], long npts)
{
  long  adr;

  P0(PUSHx,"push16");
  adr=rec24();
  send24(npts << 2);
  SendData(buf,adr,npts);
  cmdout(INT_DSPx);
}

/******************************************************************
   Does push and then pop from APOS memory at location specified
   for NPTS specified
  ----------------------------------------------------------------**/
void pushpoptest(qint buf[], long adr, long npts)
{
  SendData(buf,adr,npts);
  ReceiveData(buf,adr,npts);
}


/******************************************************************
  Pushes Single variable or array on AP stack.  Npts is
  the number of elements.  Use npts==0 for scalars.
  ----------------------------------------------------------------**/
void pushf(float buf[], long npts)
{
  long  adr;

  P0(PUSHx,"pushf");
  adr=rec24();
  send24(npts << 2);
  SendData((qint *) buf,adr,npts << 1);
  cmdout(SNG_DSPx);
}


/******************************************************************
  Pops Integer scalar or buffer from top of stack.
  ----------------------------------------------------------------**/
void pop16(qint buf[])
{
  long  adr,npts;

  P0(DSP_INTx,"pop16");
  cmdout(POPx);
  adr=rec24();
  npts=rec24();
  ReceiveData(buf,adr,npts >> 2);
}


/******************************************************************
  Pops Single buffer from top of stack.
  ----------------------------------------------------------------**/
void popf(float buf[])
{
  long  adr,npts;

  P0(DSP_SNGx,"popf");
  cmdout(POPx);
  adr=rec24();
  npts=rec24();
  ReceiveData((qint *) buf,adr,npts >> 1);
}


void popport16(qint pp)
{
  long spts;
  long adr,npts;
  qint *tbuf;

  P0(DSP_INTx,"popport16");
  cmdout(POPx);
  adr=rec24();
  npts=rec24() >> 2;
  if(!ap_eflag)
  {
    tbuf = (qint *) malloc(TBUFSIZE << 1);
    if(!tbuf)
    {
      ap_emode = 0;
      APerror("Not enough PC memory for temp buffer.");
    }
    do
    {
      spts=npts;
      if(spts>TBUFSIZE) spts=TBUFSIZE;
      ReceiveData(tbuf,adr,spts);
      DataOut(FP_SEG(tbuf),FP_OFF(tbuf),pp,(unsigned int)spts);
      npts=npts-spts;
      adr=adr+(spts << 1);
    }while(npts>0);
    free(tbuf);
  }
}


void popportf(qint pp)
{
  long spts;
  long adr,npts;
  qint *tbuf;

  P0(DSP_SNGx,"poportf");
  cmdout(POPx);
  adr=rec24();
  npts=rec24() >> 1;
  if(!ap_eflag)
  {
    tbuf = (qint *) malloc(TBUFSIZE << 1);
    if(!tbuf)
    {
      ap_emode = 0;
      APerror("Not enough PC memory for temp buffer.");
    }
    do{
      spts=npts;
     if(spts>TBUFSIZE) spts=TBUFSIZE;
      ReceiveData(tbuf,adr,spts);
      DataOut(FP_SEG(tbuf),FP_OFF(tbuf),pp,(unsigned int)spts);
      npts=npts-spts;
      adr=adr+(spts << 1);
    }while(npts>0);
    free(tbuf);
  }
}

void pushport16(qint pp, long npts)
{
  long spts;
  long adr;
  qint *tbuf;

  P0(PUSHx,"pushport16");
  adr=rec24();
  send24(npts << 2);
  if(!ap_eflag)
  {
    tbuf = (qint *) malloc(TBUFSIZE << 1);
    if(!tbuf)
    {
      ap_emode = 0;
      APerror("Not enough PC memory for temp buffer.");
    }
    do
    {
      spts=npts;
      if(spts>TBUFSIZE) spts=TBUFSIZE;
      DataIn(pp,FP_SEG(tbuf),FP_OFF(tbuf),(unsigned int)spts);
      SendData(tbuf,adr,spts);
      npts=npts-spts;
      adr=adr+(spts << 1);
    }while(npts>0);
    free(tbuf);
    cmdout(INT_DSPx);
  }
}

void pushportf(qint pp,long npts)
{
  long spts;
  long adr;
  qint *tbuf;

  P0(PUSHx,"pushportf");
  adr=rec24();
  send24(npts << 2);
  if(!ap_eflag)
  {
    tbuf = (qint *) malloc(TBUFSIZE << 1);
    if(!tbuf)
    {
      ap_emode = 0;
      APerror("Not enough PC memory for temp buffer.");
    }
    npts=npts << 1;
    do
    {
      spts=npts;
      if(spts>TBUFSIZE) spts=TBUFSIZE;
      DataIn(pp,FP_SEG(tbuf),FP_OFF(tbuf),(unsigned int)spts);
      SendData(tbuf,adr,spts);
      npts=npts-spts;
      adr=adr+(spts << 1);
    }while(npts>0);
    free(tbuf);
    cmdout(SNG_DSPx);
  }
}


void popdisk16(char *fname)
{
  long spts;
  long adr,npts;
  qint *tbuf;
  FILE *fout;

  P0(DSP_INTx,"popdisk16");
  cmdout(POPx);
  adr=rec24();
  npts=rec24() >> 2;
  if(!ap_eflag)
  {
    tbuf = (qint *) malloc(TBUFSIZE << 1);
    if(!tbuf)
    {
      ap_emode = 0;
      APerror("Not enough PC memory for temp buffer.");
    }
    fout=fopen(fname,"wb");
    if(!fout)
      APerror("File access error.");
    else
    {
      do{
	spts=npts;
	if(spts>TBUFSIZE) spts=TBUFSIZE;
	ReceiveData(tbuf,adr,spts);
	fwrite(tbuf,2,(unsigned int)spts,fout);
	npts=npts-spts;
	adr=adr+(spts << 1);
      }while(npts>0);
      fclose(fout);
    }
    free(tbuf);
  }
}

void popdiskf(char *fname)
{
  long spts;
  long adr,npts;
  qint *tbuf;
  FILE *fout;

  P0(DSP_SNGx,"popdiskf");
  cmdout(POPx);
  adr=rec24();
  npts=rec24() >> 1;
  if(!ap_eflag)
  {
    tbuf = (qint *)malloc(TBUFSIZE << 1);
    if(!tbuf)
    {
      ap_emode = 0;
      APerror("Not enough PC memory for temp buffer.");
    }
    fout=fopen(fname,"wb");
    if(!fout)
      APerror("File access error.");
    else
    {
      do{
	spts=npts;
	if(spts>TBUFSIZE) spts=TBUFSIZE;
	ReceiveData(tbuf,adr,spts);
	fwrite(tbuf,2,(unsigned int)spts,fout);
	npts=npts-spts;
	adr=adr+(spts << 1);
      }while(npts>0);
      fclose(fout);
    }
    free(tbuf);
  }
}

void popdiska(char *fname)
{
  long spts,i;
  long adr,npts;
  float *tbuf;
  FILE *fout;

  P0(DSP_SNGx,"popdiska");
  cmdout(POPx);
  adr=rec24();
  npts=rec24() >> 1;
  if(!ap_eflag)
  {
    tbuf = (float *) malloc(TBUFSIZE << 1);
    if(!tbuf)
    {
      ap_emode =0;
      APerror("Not enough PC memory for temp buffer.");
    }
    fout=fopen(fname,"wt");
    if(!fout)
      APerror("File access error.");
    else
    {
      do{
	spts=npts;
	if(spts>TBUFSIZE) spts=TBUFSIZE;
	ReceiveData((qint *)tbuf,adr,spts);
	for(i=0; i<(spts >> 1); i++)
	  fprintf(fout,"%f\n",tbuf[i]);
	npts=npts-spts;
	adr=adr+(spts << 1);
      }while(npts>0);
      fclose(fout);
    }
    free(tbuf);
  }
}

void dama2disk16(long bid, char *fname, qint catflag)
{
  long spts;
  long adr,npts;
  qint *tbuf;
  FILE *fout;

  P0(DAMAINFOx,"dama2disk16");
  send24(bid);
  adr=rec24();
	npts=rec24() >> 1;
	if(!ap_eflag)
  {
    tbuf = (qint *) malloc(TBUFSIZE << 1);
    if(!tbuf)
    {
      ap_emode = 0;
      APerror("Not enough PC memory for temp buffer.");
    }

    if(catflag)
    {
      fout=fopen(fname,"ab");
      if(!fout)
	fout=fopen(fname,"wb");
    }
    else
      fout=fopen(fname,"wb");

    if(!fout)
      APerror("File access error.");
    else
    {
      do{
	spts=npts;
	if(spts>TBUFSIZE) spts=TBUFSIZE;
	ReceiveData(tbuf,adr,spts);
	fwrite(tbuf,2,(unsigned int)spts,fout);
	npts=npts-spts;
	adr=adr+(spts << 1);
      }while(npts>0);
			fclose(fout);
		}
		free(tbuf);
	}
}


void disk2dama16(long bid, char *fname, long seekpos)
{
	long spts;
	long adr,npts,bpts,npts2;
	qint *tbuf;
	FILE *fin;
	int i;

	P0(DAMAINFOx,"disk2dama16");
	send24(bid);
	adr = rec24();
	bpts = npts = rec24() >> 1;

	if(!ap_eflag)
	{
		fin=fopen(fname,"rb");
		if(!fin)
			APerror("File not found.");
		else
		{
			fseek(fin,0l,SEEK_END);
			npts2 = (ftell(fin)  >> 1 ) - seekpos;
			if(npts2 <= 0)
				APerror("Seek position beyond end of file.");
			fseek(fin,seekpos << 1,SEEK_SET);

			if(npts>npts2)
				npts = npts2;

			tbuf = (qint *)malloc(TBUFSIZE << 1);
			if(!tbuf)
			{
				ap_emode = 0;
				APerror("Not enough PC memory for temp buffer.");
			}
			do
			{
				spts=npts;
				if(spts>TBUFSIZE) spts=TBUFSIZE;
				fread(tbuf,2,(unsigned int)spts,fin);
				SendData(tbuf,adr,spts);
				npts=npts-spts;
				adr=adr+(spts << 1);
			}while(npts>0);
			if(bpts > npts2)
			{
				for(i=0; i<TBUFSIZE; i++)
					tbuf[i] = 0;
				npts = bpts - npts2;
				do
				{
					spts=npts;
					if(spts>TBUFSIZE) spts=TBUFSIZE;
					SendData(tbuf,adr,spts);
					npts=npts-spts;
					adr=adr+(spts << 1);
				}while(npts>0);
			}
			fclose(fin);
			free(tbuf);
		}
	}
}


void pushdisk16(char *fname)
{
  long spts;
  long adr,npts;
  qint *tbuf;
  FILE *fin;

  P0(PUSHx,"pushdiski");
  fin=fopen(fname,"rb");
  if(!fin)
    APerror("File not found.");
  else
  {
    fseek(fin,0l,SEEK_END);
    npts=ftell(fin);
    fseek(fin,0l,SEEK_SET);
    adr=rec24();
    send24(npts << 1);
    npts=npts >> 1;
    tbuf = (qint *) malloc(TBUFSIZE << 1);
    if(!tbuf)
    {
      ap_emode = 0;
      APerror("Not enough PC memory for temp buffer.");
    }
    do{
      spts=npts;
      if(spts>TBUFSIZE) spts=TBUFSIZE;
      fread(tbuf,2,(unsigned int)spts,fin);
      SendData(tbuf,adr,spts);
      npts=npts-spts;
      adr=adr+(spts << 1);
    }while(npts>0);
    fclose(fin);
    free(tbuf);
    cmdout(INT_DSPx);
  }
}

void pushdiskf(char *fname)
{
  long spts;
  long adr,npts;
  qint *tbuf;
  FILE *fin;

  P0(PUSHx,"pushdiskf");
  fin=fopen(fname,"rb");
  if(!fin)
    APerror("File not found.");
  else
  {
    fseek(fin,0l,SEEK_END);
    npts=ftell(fin);                              
    fseek(fin,0l,SEEK_SET);
    adr=rec24();
    send24(npts);
    npts=npts >> 1;
    tbuf = (qint *) malloc(TBUFSIZE << 1);
    if(!tbuf)
    {
      ap_eflag = 0;
      APerror("Not enough PC memory for temp buffer.");
    }
    do{
      spts=npts;
      if(spts>TBUFSIZE) spts=TBUFSIZE;
      fread(tbuf,2,(unsigned int)spts,fin);
      SendData(tbuf,adr,spts);
      npts=npts-spts;
      adr=adr+(spts << 1);
    }while(npts>0);
    fclose(fin);
    free(tbuf);
    cmdout(SNG_DSPx);
  }
}

void pushdiska(char *fname)
{
  long spts;
  long adr,npts;
  float *tbuf;
  FILE *fin;

  P0(PUSHx,"pushdiska");
  fin=fopen(fname,"rt");
  if(!fin)
    APerror("File not found.");
  else
  {
    adr=rec24();
    send24(100);     /* This is a dummy number of points */
    tbuf = (float *) malloc(TBUFSIZE << 1);
    if(!tbuf)
    {
      ap_emode = 0;
      APerror("Not enough PC memory for temp buffer.");
    }
    npts = 0;
    do{
      spts = 0;
      do
      {
	fscanf(fin,"%f\n",&tbuf[spts]);
	spts ++;
      }while(!feof(fin) && spts < (TBUFSIZE >> 1));
      SendData((qint *)tbuf,adr,spts << 1);
      npts += spts;
      adr=adr+(spts << 2);
    }while(!feof(fin));
    fclose(fin);
    drop();
    dpush(npts);
    cmdout(SNG_DSPx);
    free(tbuf);
  }
}


void poppush(qint s ,qint d)
{
  long spts;
  long npts;
  qint *tbuf;
  long adr_s,adr_d;
  qint holder;

  caller = (char *)("poppush");
  s=s & 0x0f;
  d=d & 0x0f;
  if(s==d)
    APerror("Illegal Source-Destination pair designated.");
  if(!aps[s])
    APerror("Non-present AP specified as source.");
  if(!aps[d])
    APerror("Non-present AP specified as destination.");
  holder=apsel;
  apsel=s;
  cmdout(POPx);
  adr_s=rec24();
  npts=rec24() >> 2;

  apsel=d;
  cmdout(PUSHx);
  adr_d=rec24();
  send24(npts);

  tbuf = (qint *) malloc(TBUFSIZE << 1);
  if(!tbuf)
  {
    ap_emode = 0;
    APerror("Not enough PC memory for temp buffer.");
  }
  do{
    spts=npts;
    if(spts>TBUFSIZE) spts=TBUFSIZE;
    apsel=s;
    ReceiveData(tbuf,adr_s,spts);
    apsel=d;
    SendData(tbuf,adr_d,spts);
    npts=npts-spts;
    adr_s=adr_s+(spts << 1);
    adr_d=adr_d+(spts << 1);
  }while(npts>0);
  free(tbuf);
  apsel=holder;
}


/******************************************************************
  Allocs Single variable or array on AP stack.  Npts is
  the number of elements allocated.  No data is transferred.
  ----------------------------------------------------------------**/
void dpush(long npts)
{
  P0(DPUSHx,"dpush");
  send24(npts << 2);
}


/******************************************************************
  Identifies block section of complete buffer.
  ----------------------------------------------------------------**/
void block(long sp,long ep)
{
  P0(BLOCKx,"block");
  send24(sp);
  send24(ep);
}


void noblock( void )
{
  P0(NOBLOCKx,"noblock");
}

void swap( void )
{
  P0(SWAPx,"swap");
}

void qdup( void )
{
  P0(QDUPx,"qdup");
}

void allotf(long bid, long npts)
{
  P0(ALLOTFx,"allotf");
  send24(bid);
  send24(npts << 2);
}

void allot16(long bid, long npts)
{
  P0(ALLOT16x,"allot16");
  send24(bid);
  send24(npts << 1);
}

void deallot(long bid)
{
  P0(DEALLOTx,"deallot");
  send24(bid);
}

void trash( void )
{
  P0(TRASHx,"trash");
}

void qpopf( long bid)
{
  P0(QPOPFx,"qpopf");
  send24(bid);
}

void qpoppartf( long bid, long spos)
{
  P0(QPOPPARTFx,"qpoppartf");
  send24(bid);
  send24(spos);
}

void qpushf( long bid)
{
  P0(QPUSHFx,"qpushf");
  send24(bid);
}

void qpushpartf( long bid, long spos, long npts)
{
  P0(QPUSHPARTFx,"qpushpartf");
  send24(bid);
  send24(spos);
  send24(npts);
}

void drop( void )
{
  P0(DROPx,"drop");
}

void dropall( void )
{
  P0(DROPALLx,"dropall");
}

void make(long ind, float v)
{
  P0(MAKEx,"make");
  send24(ind);
  sendflt(v);
}

float whatis(long ind)
{
  P0(WHATISx,"whatis");
  send24(ind);
  return(recflt());
}

void parse(char ss[])
{
  int   i,j,ind;
  float v;
  char  s2[20],s1[10],s[500];

  strcpy(s,ss);
  strcat(s,"~");
  strcpy(caller,"parse");
  if(stackdepth()==0)
  {
    APerror("Must have a stacked buffer to parse values into.");
  }
  else
  {
    i = 0;
    strcpy(s2,"");
    ind = 0;
    do
    {
      j = s[i];
      if(((j>='0') && (j<='9')) ||
	 (j=='.') || (j=='-') || (j=='e') || (j=='E'))
      {
	s1[0] = s[i];
	s1[1] = 0;
	strcat(s2,s1);
      }
      else
      {
	if(s2[0]!=0)
	{
	  sscanf(s2,"%f",&v);
	  make(ind,v);
	  ind = ind + 1;
	  strcpy(s2,"");
	}
      }
      i++;
    }while(i<strlen(s));
  }
}

void extract( void )
{
  P0(EXTRACTx,"extract");
}

void reduce( void )
{
	P0(REDUCEx,"reduce");
}

void cat( void )
{
  P0(CATx,"cat");
}

void catn(qint n)
{
  P0(CATNx,"catn");
  send24((long)n);
}

void dupn(qint n)
{
  P0(DUPNx,"dupn");
  send24((long)n);
}

void qpop16( long bid)
{
  P0(QPOP16x,"qpop16");
  send24(bid);
}

void qpoppart16( long bid, long spos)
{
  P0(QPOPPART16x,"qpoppart16");
  send24(bid);
  send24(spos);
}

void qpush16( long bid)
{
  P0(QPUSH16x,"qpush16");
  send24(bid);
}

void qpushpart16( long bid, long spos, long npts)
{
  P0(QPUSHPART16x,"qpushpart16");
  send24(bid);
  send24(spos);
  send24(npts);
}

void totop( long sn)
{
  P0(TOTOPx,"totop");
  send24(sn);
}

void makedama16(qint bid, long ind, qint v)
{
  P0(MAKEDAMA16x,"makedama16");
  send24((long)bid);
  send24(ind);
  send24((long)v);
}

void makedamaf(qint bid, long ind, float v)
{
  P0(MAKEDAMAFx,"makedamaf");
  send24((long)bid);
  send24(ind);
  sendflt(v);
}

long getaddr(qint bid)
{
	P0(GETADDRx,"getaddr");
	send24((long)bid);
	return((long)rec24());
}

void setaddr(qint bid, long addr)
{
	P0(SETADDRx,"setaddr");
	send24((long)bid);
	send24(addr);
}

long _allotf(long npts)
{
	P0(_ALLOTFx,"_allotf");
	send24(npts << 2);
	return(rec24());
}

long _allot16(long npts)
{
	P0(_ALLOT16x,"_allot16");
	send24(npts << 1);
	return(rec24());
}


/*Data Generators                  DGEN*/
void fill(float start, float step)
{       P2(FILLx,start,step,"fill");    }
void gauss( void )
{       P0(GAUSSx,"gauss");   }
void qrand( void )
{       P0(RANDx,"qrand");   }
void flat( void )
{       P0(FLATx,"flat");   }
void seed( long sval )
{
  P0(SEEDx,"seed");
  send24(sval);
}
void value( float  v)
{       P1(VALUEx,v,"value");   }
void tone(float f,float sr)
{       P2(TONEx,f,sr,"tone");   }


/*Complex operators                COMP */
void xreal( void )
{       P0(XREALx,"xreal");   }
void ximag( void )
{       P0(XIMAGx,"ximag");   }
void polar( void )
{       P0(POLARx,"polar");   }
void rect( void )
{       P0(RECTx,"rect");   }
void shuf( void )
{       P0(SHUFx,"shuf");   }
void split( void )
{       P0(SPLITx,"split");   }
void cmult( void )
{       P0(CMULTx,"cmult");   }
void cadd( void )
{       P0(CADDx,"cadd");   }
void cinv( void )
{       P0(CINVx,"cinv");   }


/*Basic math                       MATH */
void scale(float sf)
{       P1(SCALEx,sf,"scale");   }
void shift(float sf )
{       P1(SHIFTx,sf,"shift");   }
void logten( void )
{       P0(LOG10x,"logten");   }
void loge( void )
{       P0(LOGEx,"loge");   }
void logn(float base)
{       P1(LOGNx,base,"logn");   }
void alogten( void )
{       P0(ALOG10x,"alogten");   }
void aloge( void )
{       P0(ALOGEx,"aloge");   }
void add( void )
{       P0(ADDx,"add");   }
void subtract( void )
{       P0(SUBTRACTx,"subtract");   }
void mult( void )
{       P0(MULTx,"mult");   }
void divide( void )
{       P0(DIVIDEx,"divide");   }
void sqroot( void )
{       P0(SQROOTx,"sqroot");   }
void square( void )
{       P0(SQUAREx,"square");   }
void power(float pw)
{       P1(POWERx,pw,"power");   }
void inv( void )
{       P0(INVx,"inv");   }
void maxlim(float max)
{       P1(MAXLIMx,max,"maxlim");   }
void minlim(float min)
{       P1(MINLIMx,min,"minlim");   }
void maglim(float max)
{       P1(MAGLIMx,max,"maglim");   }
void radd( void )
{       P0(RADDx,"radd");   }
void absval( void )
{       P0(ABSVALx,"absval");   }


/*FFT Stuff                        FFT */
void cfft( void )
{       P0(CFFTx,"cfft");   }
void cift( void )
{       P0(CIFTx,"cift");   }
void hann( void )
{       P0(HANNx,"hann");   }
void hamm( void )
{       P0(HAMMx,"hamm");   }
void rfft( void )
{       P0(RFFTx,"rfft");   }
void rift( void )
{       P0(RIFTx,"rift");   }
void seperate( void )
{       P0(SEPERATEx,"seperate");   }


/*TRIG Stuff                      TRIG */
void cosine( void )
{       P0(COSINEx,"cosine");   }
void sine( void )
{       P0(SINEx,"sine");   }
void tangent( void )
{       P0(TANGENTx,"tangent");   }
void acosine( void )
{       P0(ACOSINEx,"acosine");   }
void asine( void )
{       P0(ASINEx,"asine");   }
void atangent( void )
{       P0(ATANGENTx,"atangent");   }
void atantwo( void )
{       P0(ATANTWOx,"atantwo");   }
void qwind(float trf,float sr)
{       P2(QWINDx,trf,sr,"qwind");   }


/*Digital Filter                   DF */
void iir( void )
{ P0(IIRx,"iir"); }
void fir( void )
{ P0(FIRx,"fir"); }
void _iir( void )
{ P0(_IIRx,"_iir"); }
void _fir( void )
{ P0(_FIRx,"_fir"); }
void tdt_reverse( void )
{ P0(REVERSEx,"reverse"); }


/*SUM Stuff                       SUM */
float sum( void )
{
  P0(SUMx,"sum");
  return(recflt());
}

float average( void )
{
  P0(AVERAGEx,"average");
  return(recflt());
}

float maxval( void )
{
  P0(MAXVALx,"maxval");
  return(recflt());
}

float minval( void )
{
  P0(MINVALx,"minval");
  return(recflt());
}

float maxmag( void )
{
  P0(MAXMAGx,"maxmag");
  return(recflt());
}

long maxval_( void )
{
  P0(MAXVAL_x,"maxval_");
  return(rec24());
}

long minval_( void )
{
  P0(MINVAL_x,"minval_");
  return(rec24());
}

long maxmag_( void )
{
  P0(MAXMAG_x,"maxmag_");
  return(rec24());
}

/* Status Stuff                STAT */
long freewords( void )
{
  P0(FREEWORDSx,"freewords");
  return(rec24());
}

qint ap_present(qint dn)
{
  return(aps[dn & 0x0f]);
}

qint ap_active(qint dn)
{
  if(inport(cp[dn & 0x0f])==ERRFLAG)
    APerror("AP sending error FLAG.");
  else
    if(inport(cp[dn & 0x0f])==READY)
      return(0);
    else
      return(1);
  return(1);
}

void ap_select(qint dn)
{
  apsel=dn & 0x0f;
}

long topsize(void)
{
  P0(TOPSIZEx,"topsize");
  return(rec24());
}

long stackdepth(void)
{
  P0(STACKDEPTHx,"stackdepth");
  return(rec24());
}

long lowblock(void)
{
  P0(LOWBLOCKx,"lowblock");
  return(rec24());
}

long hiblock(void)
{
  P0(HIBLOCKx,"hiblock");
  return(rec24());
}

void optest(void)
{
  P0(OPTESTx,"optest");
}

long tsize(long bufn)
{
  P0(TSIZEx,"tsize");
  send24(bufn);
  return(rec24());
}



/* Play/Record                  */

void play(qint dbn)
{
  P0(PLAYx,"play");
  send24(dbn);
}

void chgplay(qint dbn)
{
	P0(CHGPLAYx,"chgplay");
    send24(dbn);
}

void dplay(qint dbn1, qint dbn2)
{
  P0(DPLAYx,"dplay");
  send24(dbn1);
  send24(dbn2);
}

void record(qint dbn)
{
  P0(RECORDx,"record");
  send24(dbn);
}

void drecord(qint dbn1, qint dbn2)
{
  P0(DRECORDx,"drecord");
  send24(dbn1);
  send24(dbn2);
}

void fastrecord(qint dbn)
{
  P0(FASTRECORDx,"fastrecord");
  send24(dbn);
}

void pfireone( qint dbn)
{
  P0(PFIREONEx,"pfireone");
  send24(dbn);
}

void pfireall(void)
{
  P0(PFIREALLx,"pfireall");
}

qint ppausestat( qint dbn)
{
  P0(PPAUSESTATx,"ppausestat");
  send24(dbn);
  return((qint)rec24());
}  

void seqplay(qint dbn)
{
  P0(SEQPLAYx,"seqplay");
  send24(dbn);
}

void seqrecord(qint dbn)
{
  P0(SEQRECORDx,"seqrecord");
  send24(dbn);
}

void mrecord(qint dbn)
{
  P0(MRECORDx,"mrecord");
  send24(dbn);
}

void mplay(qint dbn)
{
  P0(MPLAYx,"mplay");
  send24(dbn);
}

qint playseg(long chan)
{
  P0(PLAYSEGx,"playseg");
  send24(chan);
  return((qint)rec24());
}

qint recseg(long chan)
{
  P0(RECSEGx,"recseg");
  send24(chan);
  return((qint)rec24());
}

long playcount(long chan)
{
  P0(PLAYCOUNTx,"playcount");
  send24(chan);
  return(rec24());
}

long reccount(long chan)
{
  P0(RECCOUNTx,"reccount");
  send24(chan);
  return(rec24());
}


/* Misc.                   */

void plotmap( long xx1, long yy1, long xx2, long yy2)
{
  P0(PLOTMAPx,"plotmap");
  send24(xx1);
  send24(yy1);
  send24(xx2);
  send24(yy2);
}

void plotwith( long xx1, long yy1, long xx2, long yy2, float ymin, float ymax)
{
	P0(PLOTWITHx,"plotwith");
	send24(xx1);
	send24(yy1);
	send24(xx2);
	send24(yy2);
	sendflt(ymin);
	sendflt(ymax);
}

void plotwithCS( long xx1, long yy1, long xx2, long yy2, float ymin, float ymax)
{
	P0(PLOTWITHCSx,"plotwithCS");
  send24(xx1);
  send24(yy1);
  send24(xx2);
	send24(yy2);
  sendflt(ymin);
  sendflt(ymax);
}

void usercall( long cid, float argf, long arg24)
{
  P0(USERCALLx,"usercall");
  send24(cid);
  sendflt(argf);
  send24(arg24);
}

float userfunc( long cid, float argf, long arg24)
{
  P0(USERFUNCx,"userfunc");
  send24(cid);
  sendflt(argf);
  send24(arg24);
  return(recflt());
}

/* Macro Stuff                      MMT */
void recordmac(qint bid)
{
  P0(RECORDMACx,"recordmac");
  send24((long)bid);
  mrmode = 1;
}

void runmac(qint bid)
{
  P0(RUNMACx,"runmac");
  send24((long)bid);
}

void endmac(void)
{             
  P0(ENDMACx,"endmac");
  mrmode = 0;
}

qint whatmac(void)
{
  if(inport(cp[apsel])==MACACT)
    return((qint)inport(dspdat[apsel])>>8);
  else
    return(0);
}

qint whatstep(void)
{
  if(inport(cp[apsel])==MACACT)
    return((qint)inport(dspdat[apsel]) & 0xff);
  else
    return(0);
}

void stopmac(void)
{
  if(inport(cp[apsel])==MACACT)
    outport(cp[apsel],STOPMAC);
}


void ls(qint labn)
{
  P0(LABSTEPx,"ls");
  send24((long)labn);
}


long stepval24(qint macn, qint stepn)
{
  P0(STEPVAL24x,"stepval24");
  send24((long)macn);
  send24((long)stepn);
  return(rec24());
}

float stepvalf(qint macn, qint stepn)
{
  P0(STEPVALFx,"stepvalf");
  send24((long)macn);
  send24((long)stepn);
  return(recflt());
}

void if24(qint vstep1, qint cc, qint vstep2, qint desstep)
{
  P0(IF24x,"if24");
  send24((long)vstep1);
  send24((long)cc);
  send24((long)vstep2);
  send24((long)desstep);
}

void iff(qint vstep1, qint cc, qint vstep2, qint desstep)
{
  P0(IFFx,"iff");
  send24((long)vstep1);
  send24((long)cc);
  send24((long)vstep2);
  send24((long)desstep);
}

void loopn(qint stepn, qint n)
{
  P0(LOOPNx,"loopn");
  send24((long)stepn);
  send24((long)n);
}

void jump(qint stepn)
{
  P0(JUMPx,"jump");
  send24((long)stepn);
}

void counter24(long start, long step)
{
  P0(COUNTER24x,"counter24");
  send24(start);
  send24(step);
}

void counterf(float start, float step)
{
  P0(COUNTERFx,"counterf");
  sendflt(start);
  sendflt(step);
}

void constant(long lv, float fv)
{
  P0(CONSTANTx,"constant");
  send24(lv);
  sendflt(fv);
}

void usestepval(qint argn)
{
  P0(USESTEPVALx,"usestepval");
  send24((long)argn);
}

void usedamalist(qint argn)
{
	P0(USEDAMALISTx,"usedamalist");
	send24((long)argn);
}


/* More Math                        MMT */
void cumsum( void )
{       P0(CUMSUMx,"cumsum");   }

void decimate(qint fact)
{
	P0(DECIMATEx,"decimate");
	send24((long)fact);
}

void interpol(qint fact)
{
	P0(INTERPOLx,"interpol");
	send24((long)fact);
}

void foldnadd(qint artflag)
{
	P0(FOLDNADDx,"foldnadd");
	send24((long)artflag);
}

qint getnarts(void)
{
	P0(GETNARTSx,"getnarts");
	return((qint)rec24());
}
