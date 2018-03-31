#ifdef MSDOS

#define START          0x01
#define REFRESH        0x02
#define ADD            0x03

typedef struct
{
	int xx1,xx2,yy1,yy2;
	float ymin,ymax;
	float xmin,xmax;
	int bord_col,back_col,num_col,title_col,lab_col;
	int lab_space;
	char title[50];
	char lab_x[50];
	char lab_y[50];
	int pixy[1000];
	int ppts;
} plotinfo;

void gon(void);
void goff(void);
void initplotinfo(plotinfo *p);
void tdtplot1(plotinfo *p);
void tdtplot2(plotinfo *p, int col, int pmode);

#endif /* MSDOS */

#ifdef __linux__

typedef struct
{
	float 	ymin,ymax;
	float 	xmin,xmax;
	char 	title[50];
	char 	lab_x[50];
	char 	lab_y[50];
	float	samp_period;
	int		skip;
	int		autoscale;
} plotinfo;

void gon(void);
void goff(void);
void initplotinfo(plotinfo *p);
void tdtplot1(plotinfo *p, int buffer);
void tdtplotbuffer(plotinfo *p, short *data, int);
void tdtplot2(plotinfo *p, int buffer);
void tdtplotbuffer2(plotinfo *p, short *data, int);
void tdtplotet1(plotinfo *p, int buffer, long *, int);
void tdtplotbufferet1(plotinfo *p, short *data, long *, int, int);

#endif /* __linux__ */
