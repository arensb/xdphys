/********************************************************************
	SPIKE.C

		This program will illustrate how a single unit rate-frequency
		profile program can be written using TDT System II.

		The following timing diagram illustrates
		how the program operates:

						_                                                  _
		Start	_| |________________________________________________| |___
		(TG6)  .
												 <---  REPEAT_T  --->
					 .

					 .
																					________________
		 Stim _.__________________._________./                \_________
		Ouput                                \________________/
		(DD1)  .                            .
							<-- STIM_T -->   <-SEP_T->   <-- STIM_T -->
					 .														.

					 .                            .
									____________                 ____________
	 Record _._____|            |_________._____|            |________
	Control
		(TG6)
						<---> <-RECORD_T->           <---> <-RECORD_T->
							^                            ^
					 DELAY_T                      DELAY_T


 ********************************************************************/

#include <stdlib.h>
#include <math.h>
#include <conio.h>
#include <stdio.h>
#include <string.h>
#include <graphics.h>
#include <dos.h>

#include "c:\tdt\drivers\xbdrv\c\xbdrv.h"
#include "c:\tdt\drivers\apos\c\apos.h"
#include "c:\tdt\drivers\examples\tdtplot.h"

#define DEBUG 1

/* Logical Names for DAMA Buffers */
#define STIM           1
#define SPIKES         2
#define ONEMILLI	     3
#define FIRST_SS      10

#define PLAYSPEC	   100
#define SEQ1	       101
#define SEQ2         102

/* See timing diagram above */
#define STIM_T      20.0
#define RF_T	       5.0
#define SEP_T       10.0
#define REPEAT_T   100.0

#define DELAY_T      2.0
#define RECORD_T    15.0

/* Number of bins in histogram displays */
#define HIST_BINS	   100

/* The Stimulas is a tone blip with the following characteristics */
#define SRATE       20.0     /* Sampling Period in Microseconds */
#define LOWFREQ   2000.0     /* Lowest freq in sweep */
#define HIFREQ    8000.0     /* Highest freq in sweep */
#define NUM_STEPS    100     /* Number if linear steps in sweep */
#define NUM_AT_EACH   10     /* Number of presentations ar each freq */
#define PROBE_LEVEL   90     /* Simultated output level in dB SPL */

/* Simulated spikes are pregenerated and combined real time to */
/* make simulated spike trains																 */
#define TOT_NUM_SS    10     /* Number of simulated spikes to pick from */
#define SS_DUR	       2     /* Duration of these spikes */

#define NUM_SPON_SPK   2     /* Number of spurious spikes */
#define MAX_SPIKES    20     /* Max spikes at full sensativity */

/* Simulated cell sensativity buffer is generated */
#define SENSE_RES    100     /* Number if points in Sense Buffer */


int  IntroInit(void);
void InitAllotSeq(void);
void SetupModules(void);
void BuildSS(void);
void MakeSpikeBuf(void);

long  npts, tpts, ompts, sspts;
float freq,fstep,level,nyqfreq,histcon,maxfreq;
int   mt,maxresp;
float Sense[SENSE_RES];
int   SponHist[HIST_BINS];
int   StimHist[HIST_BINS];
int   RespCurve[NUM_STEPS];
int   SponTot,StimTot;
plotinfo p1,p2;

void main()
{
	long t;
	int i,x,maxx;
	int flag;
	char s[100];

	if(!IntroInit())
		exit(0);

	maxfreq = 0;
	maxresp = 0;
	flag = 1;


	/* See comments below */
	InitAllotSeq();
	SetupModules();
	BuildSS();

	/* Initialize variable and start main loop */
	for(i=0; i<NUM_STEPS; i++)
		RespCurve[i]=0;
	x = 0;
	freq = LOWFREQ;
	gon();
	do
	{
		/* Generate Stim Buffer */
		dpush(npts);
		delay(100);
		tone(freq,SRATE);
		scale(5000.0);
		qwind(RF_T,SRATE);
		qpop16(STIM);

		/* This is loop for each frequecy */
		mt = NUM_AT_EACH;
		do
		{
			MakeSpikeBuf();        /* Generate new similated spike train */
			seqplay(PLAYSPEC);     /* Ready DAC for play */
			DD1arm(1);

			/* First time through loop initialize the following */
			if(mt==NUM_AT_EACH)
			{

				StimTot=0;
				SponTot=0;
				for(i=0; i<HIST_BINS; i++)
				{
					StimHist[i]=0;
					SponHist[i]=0;
				}

				ET1drop(1);          /* Cleat ET1 FIFO */
				ET1go(1);            /* Enable ET1 */
				TG6arm(1,0);         /* Start TG6 running */
				TG6go(1);
			}

			/* Process spontaneous times block from ET1 */
			while(ET1read16(1)!=0);   /* Wait for block marker */
			do
			{
				t = ET1read32(1);    /* Read a time */
				if(t>0)              /* If >0 then is valid time */
				{
					SponTot++;
					SponHist[(int)(t*histcon)]++;
				}
			}while(t!=0 && !kbhit()); /* If =0 then begin stimulated times block */


			/* Process stimulated times block from ET1 */
			do
			{
				t = ET1read32(1);    /* Read a time */
				if(t>0)              /* If >0 than is valid time */
				{
					StimTot++;
					StimHist[(int)(t*histcon)]++;
				}
			}while((DD1status(1) || t>0) && !kbhit());
			/* Quit when DD1 finished and FIFO is empty */


			mt--;
		}while(mt && !kbhit());  /* Loop back for NUM_AT_EACH times */

		TG6stop(1);    /* Stop TG6 to set up for next frequency */

		/* Updata Plots and Text on video */
		push16(SponHist,HIST_BINS);
		tdtplot1(&p1);
		tdtplot2(&p1, LIGHTGREEN, START);
		push16(StimHist,HIST_BINS);
		tdtplot1(&p1);
		tdtplot2(&p1, LIGHTRED, ADD);

		RespCurve[x] = StimTot - SponTot;
		if(RespCurve[x]>maxresp)
		{
			maxresp = RespCurve[x];
			maxfreq = freq;
			maxx = x;
		}

		if(RespCurve[x]<0)
			RespCurve[x] = 0;

		push16(RespCurve,NUM_STEPS);
		tdtplot1(&p2);
		if(x==0)
			tdtplot2(&p2, LIGHTCYAN, START);
		else
			tdtplot2(&p2, LIGHTCYAN, REFRESH);

		dropall();

		setcolor(YELLOW);
		settextjustify(LEFT_TEXT,TOP_TEXT);
		setviewport(70,370,400,450,1);
		clearviewport();

		sprintf(s,"  Frequency: %6.1f     ",freq);
		outtextxy(10,10,s);

		sprintf(s,"Spontaneous: %d     ",SponTot);
		outtextxy(10,25,s);

		sprintf(s," Stimulated: %d     ",StimTot);
		outtextxy(10,40,s);


		if(flag)
		{
			x++;
			freq += fstep;
		}

		if(freq >= HIFREQ)
		{
			freq = maxfreq;
			x = maxx;
			flag = 0;
		}

	}while(freq<HIFREQ && !kbhit());  /* Loop until reach HIFREQ */

	/* sHalt all devices (if running) */
	DD1stop(1);
	ET1stop(1);
	TG6stop(1);

	(void)getch();
	goff();
}


/***************************************************************/
/* Display user instructions and initialize plot structures    */
/***************************************************************/
int IntroInit(void)
{
	char c;

	/* Give program instructions */
	clrscr();
	printf("Spike Simulation #1         Uses: AP2,DD1,SD1,ET1,TG6        Tucker-Davis Tech.\n");
	printf("-------------------------------------------------------------------------------\n\n");
	printf("    This program will illustrate how  \n");
	printf("    TDT System II can be used to run  \n");
	printf("    an automated program for deriving \n");
	printf("    a rate-frequency profile.       \n\n");
	printf("    Cell response signals are simu-   \n");
	printf("    lated on DAC Channel-2 but are    \n");
	printf("    similar to those produced by      \n");
	printf("    real auditory units (sort of).  \n\n");
	printf("    The AP2 & DD1 are used to gen-    \n");
	printf("    erate both the auditory stimulus  \n");
	printf("    and the simulated cell response.  \n");
	printf("    The SD1 & ET1 will do spike       \n");
	printf("    discrimination and timing and the \n");
	printf("    TG6 will provide overall system   \n");
	printf("    synchroniztion.                 \n\n");
	printf("    Press any key to continue...      \n");
	printf("      or [x] to quit.                 \n");

	c=getch();
	if(c=='X' || c=='x')
		return(0);

	clrscr();
	printf("Spike Simulation #1         Uses: AP2,DD1,SD1,ET1,TG6        Tucker-Davis Tech.\n");
	printf("-------------------------------------------------------------------------------\n\n");
	printf("    Connect the System BNCs as follows:\n");
	printf("      DD1 chan-1 to Scope input-1.     \n");
	printf("      DD1 chan-2 to SD1 input.         \n");
	printf("      SD1 MUX output to Scope input-2. \n");
	printf("      SD1 FIRST to ET1 IN-1.           \n");
	printf("      SD1 LAST to ET1 IN-2.            \n");
	printf("      TG6 output-1 to Scope Trigger.   \n");
	printf("       or SD1 FIRST to Scope Trigger.  \n");
	printf("      TG6 output-2 to DD1 TRIG input.  \n");
	printf("      TG6 output-3 to ET1 ENAB input.  \n");
	printf("                                     \n\n");
	printf("    Press any key to continue...       \n");
	printf("      or [x] to quit.                  \n");

	c=getch();
	if(c=='X' || c=='x')
		return(0);

	clrscr();
	printf("Spike Simulation #1         Uses: AP2,DD1,SD1,ET1,TG6        Tucker-Davis Tech.\n");
	printf("-------------------------------------------------------------------------------\n\n");
	printf("    The program will step through a    \n");
	printf("    series of frequencies.  At each    \n");
	printf("    frequency multiple presentations   \n");
	printf("    will be used to determine a spike  \n");
	printf("    time-histogram.  Histograms are    \n");
	printf("    generated for both spontaneous and \n");
	printf("    stimulated activity.  These numbers\n");
	printf("    are compared and a frequecy sensa- \n");
	printf("    tivity curve is generated.       \n\n");
	printf("    Two plots will be displayed: A     \n");
	printf("    A spike time-histogram showing     \n");
	printf("    temporal characteristics of spike  \n");
	printf("    activity.  And the spike rate      \n");
	printf("    curve plotted versus frequecy.   \n\n");
	printf("    Press any key to continue...       \n");
	printf("      or [x] to quit.                  \n");

	c=getch();
	if(c=='X' || c=='x')
		return(0);


	/* Set up Plot structures */
	initplotinfo(&p1);
	p1.xx1=100;      p1.yy1=30;
	p1.xx2=400;      p1.yy2=130;
	p1.ymin=0.0;     p1.ymax=10.0;
	p1.xmin=DELAY_T; p1.xmax=RECORD_T+DELAY_T;
	strcpy(p1.lab_x,"Milliseconds");
	strcpy(p1.lab_y,"");
	strcpy(p1.title,"Time-Histogram");

	initplotinfo(&p2);
	p2.xx1=100;      p2.yy1=200;
	p2.xx2=400;      p2.yy2=300;
	p2.ymin=0.0;     p2.ymax=100.0;
	p2.xmin=LOWFREQ; p2.xmax=HIFREQ;
	strcpy(p2.lab_x,"Frequency");
	strcpy(p2.lab_y,"");
	strcpy(p2.title,"Rate-Frequency");

	return(1);
}


/***************************************************************/
/* Initialize XBUS/AP2 and Allocate and init DAMA buffers      */
/***************************************************************/
void InitAllotSeq(void)
{
	int i;

	/* Initialize AP2 */
	if(!apinit(APa))
	if(!apinit(APb))
	{
		printf("\n\n\nAP2 Error!!!\n\n\n");
		exit(0);
	}

	/* Initialize XBUS */
	if(!XB1init(USE_DOS))
	{
		printf("\n\n\nXBUS Error!!!\n\n\n");
		exit(0);
	}

	/* These vales are used throughout the program */
	nyqfreq = 5.0e5/SRATE;
	histcon = HIST_BINS/RECORD_T/1000.0;
	fstep = (HIFREQ-LOWFREQ)/NUM_STEPS;

	/* Allocate play sequencing buffers */
	allot16(PLAYSPEC,10);
	allot16(SEQ1,10);
	allot16(SEQ2,10);

	/* Setup Stimulis buffer */
	npts = (long)(1000.0 * STIM_T / SRATE);
	allot16(STIM,npts);

	/* Setup buffer for simulated spikes */
	tpts = (long)(1000.0 * (STIM_T*2.0 + SEP_T) / SRATE);
	allot16(SPIKES, tpts);

	/* Setup buffers for library of simulated spikes */
	sspts = (long)(1000.0 * SS_DUR / SRATE);
	for(i=0; i<TOT_NUM_SS; i++)
		allot16(FIRST_SS+i, sspts);

	/* This builds a DAMA buffer with 1ms worth of zeros */
	ompts = (long)(1000.0 / SRATE);
	allot16(ONEMILLI, ompts);
	dpush(ompts);
	value(0.0);
	qpop16(ONEMILLI);

	/* Build Play Specification Buffer (two channels) */
	dpush(10);
	make(0, SEQ2);
	make(1, SEQ1);
	make(2, 0);
	qpop16(PLAYSPEC);

	/* Build Channel-1 Sequencing buffer (stimulus buffer) */
	dpush(10);
	make(0,ONEMILLI);          /* First play ONEMILLI */
	make(1,STIM_T+SEP_T);      /* enough times to acheive required duration */
	make(2,STIM);              /* Then play STIM */
	make(3,1);                 /* Just once */
	make(4,0);                 /* Terminate with a zero and pop to DAMA */
	qpop16(SEQ1);

	/* Build Channel-2 Sequencing buffer (simulated spike train) */
	dpush(10);
	make(0,SPIKES);            /* Simply play spike train once */
	make(1,1);
	make(2,0);
	qpop16(SEQ2);

	/* This builds simulated sensativity buffer */
	dpush(SENSE_RES);
	value(0);
	block(SENSE_RES>>3, SENSE_RES>>2);
	value(1);
	hann();
	scale(-70);
	noblock();
	shift(90.0);
	dpush(SENSE_RES);
	gauss();
	scale(3.0);
	add();
	popf(Sense);
}


/***************************************************************/
/* Program XBUS modules including TG6 timing sequence.         */
/*                                                             */
/*   The TG6 will be programmed for the following timing:      */
/*    					                                               */
/*   					 __                                        __    */
/*	 OUT-1  __|  |______________________________________|      */
/*            .10                                              */
/*            .         <---  REPEAT_T --->                    */
/*            .                                                */
/*   					 __                                              */
/*	 OUT-2  __|  |_________________________________________    */
/*   					.10                                              */
/*            .                                                */
/*            .                                                */
/*  					.     ____________           ____________        */
/*	 OUT-3  __.____|            |_________|            |___    */
/*    					                          .                    */
/*    	DELAY_T<---><-RECORD_T->          .                    */
/*    					                          .                    */
/*    				 <-- STIM_T+SEP_T+DELAY_T --><-RECORD_R->        */
/*    					                                               */
/*    					                                               */
/*   NOTE,  All times are in milliseconds                      */
/*    					                                               */
/***************************************************************/
void SetupModules(void)
{
	/* Simply Clear ET1 to factory defaults */
	ET1clear(1);

	/* Program TG6 with timing shown above */
	TG6clear(1);
	TG6baserate(1, _1ms);
	TG6reps(1, CONTIN_REPS, 0);
	TG6new(1, 0, REPEAT_T, 0);
	TG6high(1, 0, 0, 10, 0x3);
	TG6high(1, 0, DELAY_T, DELAY_T+RECORD_T, 0x4);
	TG6high(1, 0, STIM_T+SEP_T+DELAY_T, STIM_T+SEP_T+DELAY_T+RECORD_T, 0x4);

	/* Program DD1 for Dual Channel Play at the specified SRATE and NPTS */
	DD1clear(1);
	DD1mode(1,DUALDAC);
	DD1srate(1,SRATE);
	DD1npts(1,tpts);
}


/***************************************************************/
/* Build simulated spike library.                              */
/***************************************************************/
void BuildSS(void)
{
	int i,rampup,rampdown;
	long maxind;

	clrscr();
	pushdiska("spikfilt.b");
	for(i=0; i<TOT_NUM_SS; i++)
	{
		gotoxy(5,10);
		printf("Generating Simulated Spikes: %d     ",i+1);

		if(i<(TOT_NUM_SS>>2))
		{
			rampup   = 6 + random(10);
			rampdown = 3 + random(300);
		}
		else
		{
			rampup   = 6 + random(10);
			rampdown = 3 + random(10);
		}

		dpush(1024);
		value(0.0);
		block(0,rampup);
		fill(0.0, 1.0/rampup);
		block(rampup,rampup+rampdown);
		fill(1.0, -1.0/rampdown);
		noblock();
		fir();
		scale(1.0/maxmag());
		maxind = maxval_();
		block(maxind-(sspts>>2), maxind+(sspts >> 2)*3-1);
		extract();
		scale((5.0+(float)random(10))/10.0);
		alogten();
		shift(-1.0);
		hann();
		scale(2000);
		qpop16(FIRST_SS+i);
		drop();
	}
}


/***************************************************************/
/* Build simulated spike train using spike library.            */
/***************************************************************/
void MakeSpikeBuf(void)
{
	long ss,sspos,i;
	float thresh;
	int gval[100];

	dpush(tpts);
	value(0.0);
	ss = random(NUM_SPON_SPK);
	for(i=0; i<ss; i++)
	{
		sspos = random(tpts-sspts);
		block(sspos,sspos+sspts);
		qpush16(random(TOT_NUM_SS)+FIRST_SS);
		add();
	}

	thresh = Sense[(int)((float)SENSE_RES * freq / nyqfreq)];
	ss = random((int)((float)(PROBE_LEVEL-thresh)*MAX_SPIKES/100.0));

	if(ss>0)
	{
		dpush(ss+1);
		gauss();
		scale(npts/8);
		pop16(gval);
		for(i=0; i<ss; i++)
		{
			sspos = tpts-(npts >> 3)*5+gval[i];
			block(sspos,sspos+sspts);
			qpush16(random(TOT_NUM_SS)+FIRST_SS);
			add();
		}
	}
	noblock();
	qpop16(SPIKES);
}
