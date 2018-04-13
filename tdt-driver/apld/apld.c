#include <stdlib.h>
#include <stdio.h>
#include <dos.h>
#include <string.h>
#include <stdint.h>


#define   BLOCKSIZE  30000

/* OBJDIR is defined by the Makefile */
/* #define   OBJDIR "/usr/local/lib" */


typedef struct
{
  unsigned short f_magic;
  unsigned short f_nscs;
  uint32_t       f_timdat;
  uint32_t       f_symptr;
  uint32_t       f_nsyms;
  unsigned short f_opthrd;
  unsigned short f_flags;
} filehdr;

typedef struct
{
  char           s_name[8];
  uint32_t       s_paddr;
  uint32_t       s_vaddr;
  uint32_t       s_size;
  uint32_t       s_scnptr;
  uint32_t       s_relptr;
  uint32_t       s_lnnoptr;
  unsigned short s_nreloc;
  unsigned short s_nlnno;
  long           s_flags;
} scnhdr;

long      i,j,k;
FILE      *fff;
long      adr,nwds,bpts;
unsigned short z[BLOCKSIZE];
int        base;
int        act[2];
int        dev;
filehdr   fh;
scnhdr    sh[5];
int        qap1;


void setaddr(long adr)
{
  outportb(base+0x0B,(adr >> 16) & 0xff);
  outport(base+0x00,adr);
}

void dataout(unsigned short z[], unsigned ioport, unsigned count)
{
  unsigned int i;
  for(i=0; i<count; i++)
  {
    outport(ioport,z[i]);
  }
}

int main(int argc, char *argv[], char *env[])
{
  char buf[255];
  printf("\n Usage: APLD {APa} {APb}\n\n");

  act[0]=0; act[1]=0; qap1=0;

  if(argc==1)
  {
    act[0]=1;
  }
  else
  {
    for(i=1; i<argc; i++)
    {
      if(!strcmp(argv[i],"A") || !strcmp(argv[i],"a"))
        act[0]=1;
      if(!strcmp(argv[i],"B") || !strcmp(argv[i],"b"))
        act[1]=1;
			if(!strcmp(argv[i],"APa") || !strcmp(argv[i],"apa"))
        act[0]=1;
			if(!strcmp(argv[i],"APb") || !strcmp(argv[i],"apb"))
        act[1]=1;
      if(!strcmp(argv[i],"QAP1") || !strcmp(argv[i],"qap1"))
        qap1=1;
    }
  }
  if(!act[0] && !act[1]) act[0]=1;

  for(dev=0; dev<2; dev++)
  {
    if(act[dev])
    {
      base=0x220+(dev << 5);

      io_init(base);

      if(qap1)
        if(dev==0)
          printf("  Loading: QAP1a\n");
        else
          printf("  Loading: QAP1b\n");
      else
        if(dev==0)
          printf("  Loading: AP2a\n");
        else
          printf("  Loading: AP2b\n");

      if(qap1)
      {
        outportb(base+0xe,2);
        for(i=0; i<1000; i++) outportb(0x80, 0);
        outportb(base+0x0e,0);
        for(i=0; i<1000; i++) outportb(0x80, 0);
        outportb(base+0x0e,2);
        for(i=0; i<1000; i++) outportb(0x80, 0);

        outportb(base+0x07, 0x1b);
        outportb(base+0x0a, 0x00);
        outportb(base+0x0e, 0x02);
      }
      else
      {
        outportb(base+0x12,4);
        for(i=0; i<1000; i++) outportb(0x80, 0);
        outportb(base+0x12,0);
        for(i=0; i<1000; i++) outportb(0x80, 0);
        outportb(base+0x12,4);
        for(i=0; i<1000; i++) outportb(0x80, 0);

        outportb(base+0x07, 0x1b);
        outportb(base+0x0a, 0x02);
        outportb(base+0x12, 0x05);
      }

	  sprintf(buf, "%s/ap.obj", OBJDIR);
      fff=fopen(buf,"rb");
      if(!fff)
      {
        printf("\n\n  QAP Program File \"%s\" not found!!!\n\n", buf);
        exit(1);
      }

      /* Read file header */
      fread(&fh,sizeof(filehdr),1,fff);
      fread(sh,sizeof(scnhdr),fh.f_nscs,fff);

      for(i=0; i<fh.f_nscs; i++)
      {
        setaddr(sh[i].s_paddr);
        nwds = sh[i].s_size >> 1;
        if(sh[i].s_size & 1)
        {
          printf("\n\n  APOS onboard code file corrupted!!!\n\n");
          exit(1);
        }
        do
        {
          bpts = nwds;
          if(bpts > BLOCKSIZE) bpts = BLOCKSIZE;
          nwds = nwds - bpts;
          fread(z, sizeof(short), bpts, fff);
          dataout(z,base+0x02,(unsigned int)bpts);
        }while(nwds>0);
      }
      fclose(fff);
      if(qap1)
      {
        outportb(base+0x0e,0x03);
        for(i=0; i<1000; i++) outportb(0x80, 0);
        outportb(base+0x07,3);
        outportb(base+0x0a,0);
      }
      else
      {
        outportb(base+0x12,0x07);
        for(i=0; i<1000; i++)  outportb(0x80, 0);
        outportb(base+0x07,3);
        outportb(base+0x0a,2);
      }
      i=30000;
      do
      {
        if(!(i--))
        {
          printf("  Array Processor not responding after code loading!!!\n\n");
          exit(1);
        }
      }while(inport(base+0x08)!=1110);
      if(!qap1)
        outportb(base+0x12,0x0f);
    }
  }
  printf("\n\n");
  exit(0);
}
