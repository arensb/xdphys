#include <stdlib.h>
#include <math.h>
#include <conio.h>
#include <stdio.h>
#include <string.h>
#include <dos.h>

#include "\xbdrv\xbdrv.h"
#include "\apos\apos.h"
#include "\sampdemo\tdtplot.h"

#define SAMP1     1
#define WAVE1	    2
#define NPTS      1000l
#define SRATE     10.0

void secdel(float d);

void main(int argc, char *argv[])
{
	char c;
	int apn;
	int dadev,addev;

	apn = APa;
	if(argc>1)
	{
		if(!strcmp(argv[1],"APb") ||
			 !strcmp(argv[1],"apb") ||
			 !strcmp(argv[1],"B")   ||
			 !strcmp(argv[1],"b")   )
			apn = APb;
	}

	if(!apinit(apn))
	{
		printf("\n\nAP2 NOT Initializing.\n\n");
		exit(0);
	}

	if(!XB1init(apn))
	{
		printf("\n\nXBUS NOT Initializing. \n\n");
		exit(0);
	}

	allot16(WAVE1,NPTS);
	allot16(SAMP1,NPTS);

	dpush(NPTS);
	value(0.0);
	qpop16(WAVE1);

	dadev = 0;
	addev = 0;

	if(XB1device(DA1_CODE,1))
		dadev = DA1_CODE;

	if(XB1device(DA3_CODE,1))
		dadev = DA3_CODE;

	if(XB1device(AD1_CODE,1))
		addev = AD1_CODE;

	if(XB1device(AD2_CODE,1))
		addev = AD2_CODE;

	if(XB1device(DD1_CODE,1))
	{
		dadev = DD1_CODE;
		addev = DD1_CODE;
	}

	if(dadev)
	{
		switch(dadev)
		{
			case DA1_CODE:
				printf("\nTesting DA1...  ");
				DA1clear(1);
				DA1mode(1,DAC1);
				DA1srate(1,SRATE);
				DA1npts(1,NPTS*100);
				play(WAVE1);
				DA1arm(1);
				DA1go(1);
				secdel(1.2);
				if(DA1status(1))
				{
					printf("\n  DA1 Operation Error.\n\n");
					exit(0);
				}
				printf("OK\n");
				break;

			case DD1_CODE:
				printf("\nTesting DD1 (D/A)...  ");
				DD1clear(1);
				DD1mode(1,DAC1);
				DD1srate(1,SRATE);
				DD1npts(1,NPTS*100l);
				play(WAVE1);
				DD1arm(1);
				DD1go(1);
				secdel(1.1);
				if(DD1status(1))
				{
					printf("\n\n  DD1 Operation Error.\n\n");
					exit(0);
				}
				printf("OK\n");
				break;

			case DA3_CODE:
				printf("\nTesting DA3...  ");
				DA3clear(1);
				DA3mode(1,DAC1);
				DA3srate(1,SRATE);
				DA3npts(1,NPTS*100l);
				play(WAVE1);
				DA3arm(1);
				DA3go(1);
				secdel(1.1);
				if(DA3status(1))
				{
					printf("\n\n  DA3 Operation Error.\n\n");
					exit(0);
				}
				printf("OK\n");
				break;
		}
	}

	if(addev)
	{
		switch(addev)
		{
			case AD1_CODE:
				printf("\nTesting AD1...  ");
				AD1clear(1);
				AD1mode(1,ADC1);
				AD1srate(1,SRATE);
				AD1npts(1,NPTS*100);
				record(SAMP1);
				AD1arm(1);
				AD1go(1);
				secdel(1.1);
				if(AD1status(1))
				{
					printf("\n  AD1 Operation Error.\n\n");
					exit(0);
				}
				printf("OK\n");
				break;

			case AD2_CODE:
				printf("\nTesting AD2...  ");
				AD2clear(1);
				AD2mode(1,ADC1);
				AD2srate(1,SRATE);
				AD2npts(1,NPTS*100);
				record(SAMP1);
				AD2arm(1);
				AD2go(1);
				secdel(1.1);
				if(AD2status(1))
				{
					printf("\n  AD2 Operation Error.\n\n");
					exit(0);
				}
				printf("OK\n");
				break;

			case DD1_CODE:
				printf("\nTesting DD1 (A/D)...  ");
				DD1clear(1);
				DD1mode(1,ADC1);
				DD1srate(1,SRATE);
				DD1npts(1,NPTS*100);
				record(SAMP1);
				DD1arm(1);
				DD1go(1);
				secdel(1.1);
				if(DD1status(1))
				{
					printf("\n  DD1 Operation Error.\n\n");
					exit(0);
				}
				printf("OK\n");
				break;
		}
	}
}

void secdel(float d)
{
	struct time t;
	float bt1,bt2;

	gettime(&t);
	bt1 = t.ti_hour*3600.0 + t.ti_min*60.0 + t.ti_sec + t.ti_hund/100.0;
	do
	{
		gettime(&t);
		bt2 = t.ti_hour*3600.0 + t.ti_min*60.0 + t.ti_sec + t.ti_hund/100.0;
	}while((bt2-bt1)<d);
}