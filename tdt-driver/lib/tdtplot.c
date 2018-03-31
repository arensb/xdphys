#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifdef MSDOS
#include <graphics.h>
#include <conio.h>
#endif /* MSDOS */

#include "apos.h"
#include "tdtplot.h"

int g_driver, g_mode, g_error;

#ifdef __linux__
#include <unistd.h>
#include <errno.h>

#define GNUPLOT			"gnuplot"
#define TEMPDATA		"./.tmpdata"
#define TEMPDATA2		"./.tmpdata2"

FILE 	*fp = NULL;
int 	called_tdtplot1 = 0;
#endif /* __linux__ */

#ifdef MSDOS
void gon()
{
	detectgraph(&g_driver,&g_mode);
	if (g_driver<0) {
		printf("\n\n No Graphics Hardware Detected !!!\n\n");
		exit(1);
	}
	initgraph(&g_driver, &g_mode, "");
	g_error=graphresult();

	if (g_error<0) {
		printf("\n\n Initialization Error. \n\n");
		exit(1);
	}
}

void goff()
{
	closegraph();
}


#define FLUFF           2
#define RD_BLOCK       30


void initplotinfo(plotinfo *p)
{
  p->xx1  =   20;
  p->yy1  =   20;
  p->xx2  =  220;
  p->yy2  =  120;
  p->ymin = -100;
  p->ymax =  100;
  p->xmin =    0;
  p->xmax =  100;
	p->bord_col   = LIGHTBLUE;
  p->back_col   = DARKGRAY;
	p->num_col    = WHITE;
	p->title_col  = LIGHTGREEN;
	p->lab_col    = CYAN;
	p->lab_space  = 25;
	p->ppts       = 0;
	strcpy(p->title,"Data Plot");
	strcpy(p->lab_x,"");
	strcpy(p->lab_y,"");
}


void tdtplot1(plotinfo *p)
{
	int mx,my;

	mx = p->xx2-p->xx1;
	my = p->yy2-p->yy1;

	plotwith(FLUFF,FLUFF+1,mx-2*FLUFF,my-2*FLUFF+1,p->ymin,p->ymax);
	p->ppts = (int)topsize();
	pop16(p->pixy);
}


void tdtplot2(plotinfo *p, int col, int pmode)
{
	float xp,xs;
	int	i;
	int mx,my;
	int poly[10];
	int left,right;
	char labs[50];
	float labsep;
	int numlabs;
	int txt_w,txt_h;


	if(!p->ppts)
	{
		printf("\n\n Must call tdtplot1 before calling tdtplot2 !!!\n\n");
		exit(0);
	}

	mx = p->xx2-p->xx1;
	my = p->yy2-p->yy1;

	if(pmode==START)
	{
		settextstyle(DEFAULT_FONT, HORIZ_DIR, 1);
		setviewport(0,0,getmaxx(),getmaxy(),0);

		poly[0] = p->xx1;  poly[1] = p->yy1;
		poly[2] = p->xx2;  poly[3] = p->yy1;
		poly[4] = p->xx2;  poly[5] = p->yy2;
		poly[6] = p->xx1;  poly[7] = p->yy2;

		setcolor(p->bord_col);
		setfillstyle(SOLID_FILL,p->back_col);
		fillpoly(4,poly);

		setcolor(p->num_col);

		txt_w = 0;
		txt_h = textheight("X");
		settextjustify(RIGHT_TEXT,BOTTOM_TEXT);
		numlabs = (int)(my/p->lab_space);
		numlabs = ((numlabs >> 1)<<1)+1;
		if(numlabs<2)
			numlabs = 2;
		labsep = (p->ymax-p->ymin)/(numlabs-1);

		for(i=0; i<numlabs; i++)
		{
			sprintf(labs,"%1.3f",p->ymax-labsep*i);
			outtextxy(p->xx1-2*FLUFF,p->yy1+i*(my/(numlabs-1))+FLUFF,labs);
			if(textwidth(labs)>txt_w)
				txt_w = textwidth(labs);
		}

		settextjustify(CENTER_TEXT,TOP_TEXT);
		numlabs = (int)(mx/p->lab_space/4);
		numlabs = ((numlabs >> 1)<<1)+1;
		if(numlabs<2)
			numlabs = 2;
		labsep = (p->xmax-p->xmin)/(numlabs-1);

		for(i=0; i<numlabs; i++)
		{
			sprintf(labs,"%1.3f",p->xmin+i*labsep);
			outtextxy(p->xx1+i*(mx/(numlabs-1)),p->yy2+4*FLUFF,labs);
		}

		setcolor(p->title_col);
		settextjustify(CENTER_TEXT,BOTTOM_TEXT);
		outtextxy((p->xx1+p->xx2)>>1,p->yy1-2*FLUFF,p->title);

		setcolor(p->lab_col);
    settextjustify(CENTER_TEXT,TOP_TEXT);
		outtextxy((p->xx1+p->xx2)>>1,p->yy2+txt_h+6*FLUFF,p->lab_x);

		settextstyle(DEFAULT_FONT, VERT_DIR, 1);
		settextjustify(CENTER_TEXT,CENTER_TEXT);
		outtextxy(p->xx1-txt_w-4*FLUFF,(p->yy1+p->yy2)>>1,p->lab_y);
		settextstyle(DEFAULT_FONT, HORIZ_DIR, 1);
	}

	setviewport(p->xx1,p->yy1,p->xx2,p->yy2,0);

	poly[0] =     1;  poly[1] =    1;
	poly[2] =  mx-2;  poly[3] =    1;
	poly[4] =  mx-2;  poly[5] = my-2;
	poly[6] =     1;  poly[7] = my-2;

	xs = (float)(mx-2*FLUFF)/p->ppts;
	xp = (float)FLUFF;

	right = 0;
	setcolor(col);
	moveto(xp,p->pixy[0]);
	for(i=0; i<p->ppts; i++)
	{
		left = (int)xp;
		if(pmode==REFRESH)
		{
			if(left >= right)
			{
				right = (int)(xp + (float)RD_BLOCK*xs);
				if (right>mx-FLUFF)
					right = mx-FLUFF;
				poly[0] = poly[6] = left;
				poly[2] = poly[4] = right;
				setcolor(p->back_col);
				setfillstyle(SOLID_FILL,p->back_col);
				fillpoly(4,poly);
				setcolor(col);
			}
		}
		lineto(left,p->pixy[i]);
		xp = xp + xs;
	}
}

#endif /* MSDOS */

/* -------------------------------------------------------------------  */
#ifdef __linux__
/*                                                                      */
/* For Linux, we'll use gnuplot as our graphics engine, so the idea is  */
/* to open a pipe to gnuplot through which to issue commands.           */
/*                                                                      */
/* -------------------------------------------------------------------  */

/* -------------------------------------------------------------------  */
/* gon                                                                  */
/*                                                                      */
/*    Open a pipe to GNUPLOT, store the stream pointer in global        */
/*    variable fp.                                                      */
/*                                                                      */
/* -------------------------------------------------------------------  */

void gon(void)
{
	if ((fp = popen(GNUPLOT, "w")) == NULL) {
		perror("popen GNUPLOT");
		exit(1);
	}
}

/* -------------------------------------------------------------------  */
/* goff                                                                 */
/*                                                                      */
/*    Close the pipe to GNUPLOT.                                        */
/*                                                                      */
/* -------------------------------------------------------------------  */

void goff()
{
	if (fp != NULL)
		pclose(fp);

	unlink(TEMPDATA);
	unlink(TEMPDATA2);
}

/* -------------------------------------------------------------------  */
/* initplotinfo                                                         */
/*                                                                      */
/*    We'll use a plotinfo struct to store information about our        */
/*    current plot; stuff like title, axes titles, and axes limits.     */
/*                                                                      */
/*    Call this before tdtplot1() to set up some reasonable defaults.   */
/*                                                                      */
/* -------------------------------------------------------------------  */

void initplotinfo(plotinfo *p)
{
	p->ymin = -32767;
	p->ymax =  32768;
	p->xmin =    0;
	p->xmax =  1000;
	p->skip       = 1;
	p->autoscale  = 0;
	strcpy(p->title,"Data Plot");
	strcpy(p->lab_x,"");
	strcpy(p->lab_y,"");
}

/* -------------------------------------------------------------------  */
/* tdtplot1                                                             */
/*                                                                      */
/*    Plot the first waveform on the gnuplot graph.  Erases any         */
/*    previously existing waveform.  This is where we set the           */
/*    graph title, axes title, grid, x and y axes ranges.               */
/*                                                                      */
/*    We write the data to a file, TEMPDATA, and tell gnuplot to        */
/*    plot the data in the file.                                        */
/*                                                                      */
/*    If <buffer> is <= 0, we plot whatever buffer is on the AP2        */
/*	  stack.  If <buffer> is > 0, we plot the data from the DAMA        */ 
/*    buffer with that ID.                                              */
/*                                                                      */
/* -------------------------------------------------------------------  */

void tdtplot1(plotinfo  *p, int buffer)
{
	short	*data = NULL;
	int		ppts;


	if (buffer > 0)
		qpush16(buffer);

	ppts = (int)topsize();
	data = (short *) calloc(ppts, sizeof(short));
	pop16(data);

	tdtplotbuffer(p, data, ppts);

	free(data);
}


/* -------------------------------------------------------------------  */
/* tdtplotbuffer                                                        */
/* -------------------------------------------------------------------  */

void tdtplotbuffer(plotinfo *p, short *data, int ppts)
{
	FILE	*datafp = NULL;
	int		end_sample;
	int		start_sample;
	int		i;

	if (p->xmin < 0) p->xmin = 0;

	fprintf(fp, "set data style line\n");
	fprintf(fp, "set grid\n");
	fprintf(fp, "set tics out\n");
	fprintf(fp, "set ylabel \"%s\"\n", p->lab_y);
	if (p->autoscale)
		fprintf(fp, "set autoscale y\n");
	else
		fprintf(fp, "set yrange [%f:%f]\n", p->ymin, p->ymax);
	fprintf(fp, "set xlabel \"%s\"\n", p->lab_x);
	fprintf(fp, "set xrange [%f:%f]\n", p->xmin, p->xmax);
	fprintf(fp, "set title \"%s\"\n", p->title);
	fflush(fp);

	start_sample = (int) (p->xmin * 1000.0/p->samp_period);
	if (start_sample < 0) start_sample = 0;
	if (start_sample > ppts-1) start_sample = ppts-1;

	end_sample = (int) (p->xmax * 1000.0/p->samp_period);
	if (end_sample < 0) end_sample = start_sample+1;
	if (end_sample > ppts-1) end_sample = ppts-1;

	/* Write the data out to a temp file */
	if ((datafp = fopen(TEMPDATA, "w")) == NULL) {
		perror("fopen TEMPDATA");
		exit(1);
	}

	for (i=start_sample; i<end_sample; i+=p->skip) 
		fprintf(datafp, "%f %d\n", (i * p->samp_period / 1000.0), data[i]);

	fclose(datafp);

	fprintf(fp, "plot '%s' using 1:2\n", TEMPDATA);
	fflush(fp);

	called_tdtplot1 = 1;

}

/* -------------------------------------------------------------------  */
/* tdtplot2                                                             */
/*                                                                      */
/*    Plot the a second waveform on an existing graph.  Unfortunately,  */
/*    you can't add to graphs in gnuplot, so we have to load the        */
/*    data, which we assume is still in TEMPDATA.                       */
/*                                                                      */
/*    We write the new data to a file, TEMPDATA2, and tell gnuplot to   */
/*    plot the data in TEMPDATA and TEMPDATA2.                          */
/*                                                                      */
/*    If <buffer> is <= 0, we plot whatever buffer is on the AP2        */
/*	  stack.  If <buffer> is > 0, we plot the data from the DAMA        */ 
/*    buffer with that ID.                                              */
/*                                                                      */
/* -------------------------------------------------------------------  */

void tdtplot2(plotinfo *p, int buffer)
{
	short	*data = NULL;
	int		ppts;

	if (buffer > 0)
		qpush16(buffer);

	ppts = (int)topsize();
	data = (short *) calloc(ppts, sizeof(short));
	pop16(data);

	tdtplotbuffer2(p, data, ppts);

	free(data);

}

/* -------------------------------------------------------------------  */
/* tdtplotbuffer2                                                       */
/* -------------------------------------------------------------------  */

void tdtplotbuffer2(plotinfo *p, short* data, int ppts)
{
	FILE	*datafp = NULL;
	int		end_sample;
	int		start_sample;
	int	i;


	if(!called_tdtplot1)
	{
		printf("\n\n Must call tdtplot1 before calling tdtplot2 !!!\n\n");
		exit(0);
	}

	start_sample = (int) (p->xmin * 1000.0/p->samp_period);
	if (start_sample < 0) start_sample = 0;
	if (start_sample > ppts-1) start_sample = ppts-1;

	end_sample = (int) (p->xmax * 1000.0/p->samp_period);
	if (end_sample < 0) end_sample = start_sample+1;
	if (end_sample > ppts-1) end_sample = ppts-1;

	/* Write the data out to a temp file */
	if ((datafp = fopen(TEMPDATA2, "w")) == NULL) {
		perror("fopen TEMPDATA");
		exit(1);
	}

	for (i=start_sample; i<end_sample; i+=p->skip) 
		fprintf(datafp, "%f %d\n", (i * p->samp_period / 1000.0), data[i]);

	fclose(datafp);

	fprintf(fp, "plot '%s' using 1:2 title \"Plot 1\", "
		"'%s' using 1:2 title \"Plot 2\"\n", 
		TEMPDATA, TEMPDATA2);
	fflush(fp);

}


/* -------------------------------------------------------------------  */
/* tdtplotet1                                                           */
/*                                                                      */
/*    This is a special plotting routine for plotting recorded data,    */ 
/*    and spikes detected in the recorded data.                         */
/*                                                                      */
/*    <times> is a buffer containing timestamps in microseconds from    */
/*    an ET1 Event Timer.                                               */
/*                                                                      */
/* -------------------------------------------------------------------  */

void tdtplotet1(
	plotinfo 	*p, 
	int 		buffer,
	long		*times,
	int			ntimes)
{
	short	*data = NULL;
	int		ppts;

	if (buffer > 0)
		qpush16(buffer);

	ppts = (int)topsize();
	data = (short *) calloc(ppts, sizeof(short));
	pop16(data);

	tdtplotbufferet1(p, data, times, ppts, ntimes);

	free(data);

}

/* -------------------------------------------------------------------  */
/* tdtplotbufferet1                                                     */
/* -------------------------------------------------------------------  */

void tdtplotbufferet1(
	plotinfo 	*p, 
	short 		*data,
	long		*times,
	int			ppts,
	int			ntimes)
{
	FILE	*datafp = NULL;
	int		end_sample;
	int		start_sample;
	int		sample;
	short	*spikes = NULL;
	int	i;


	if (p->xmin < 0) p->xmin = 0;

	fprintf(fp, "set data style line\n");
	fprintf(fp, "set grid\n");
	fprintf(fp, "set tics out\n");
	fprintf(fp, "set ylabel \"%s\"\n", p->lab_y);
	if (p->autoscale)
		fprintf(fp, "set autoscale y\n");
	else
		fprintf(fp, "set yrange [%f:%f]\n", p->ymin, p->ymax);
	fprintf(fp, "set xlabel \"%s\"\n", p->lab_x);
	fprintf(fp, "set xrange [%f:%f]\n", p->xmin, p->xmax);
	fprintf(fp, "set title \"%s\"\n", p->title);
	fflush(fp);

	start_sample = (int) (p->xmin * 1000.0/p->samp_period);
	if (start_sample < 0) start_sample = 0;
	if (start_sample > ppts-1) start_sample = ppts-1;

	end_sample = (int) (p->xmax * 1000.0/p->samp_period);
	if (end_sample < 0) end_sample = start_sample+1;
	if (end_sample > ppts-1) end_sample = ppts-1;

	/* Write the data out to a temp file */
	if ((datafp = fopen(TEMPDATA, "w")) == NULL) {
		perror("fopen TEMPDATA");
		exit(1);
	}

	for (i=start_sample; i<end_sample; i+=p->skip) 
		fprintf(datafp, "%f %d\n", (i * p->samp_period / 1000.0), data[i]);

	fclose(datafp);

	spikes = (short *) calloc(ppts, sizeof(short));

	for (i=0; i<ntimes; i++) {
		/* p->samp_period is in microseconds */
		sample = (int) (((float) times[i])/p->samp_period);
		if ((sample >=0) && (sample <= ppts))
			spikes[sample] = 10000;
	}

	/* Write the spike data out to a temp file */
	if ((datafp = fopen(TEMPDATA2, "w")) == NULL) {
		perror("fopen TEMPDATA");
		exit(1);
	}

	for (i=start_sample; i<end_sample; i+=p->skip) 
		fprintf(datafp, "%f %d\n", (i * p->samp_period / 1000.0), spikes[i]);

	fclose(datafp);

	fprintf(fp, "plot '%s' using 1:2 title \"Plot 1\", "
		"'%s' using 1:2 title \"Plot 2\"\n", 
		TEMPDATA, TEMPDATA2);
	fflush(fp);

}
#endif /* __linux__ */
