#include <stdio.h>
#include <stdlib.h>
#ifdef __linux__
#include <unistd.h>
#else
#include <sys/types.h>
#endif /* __linux__ */
#include <sys/stat.h>
#include <sys/param.h>
#include <string.h>
#ifdef sparc
#include <strings.h>
#else
#include <string.h>
#endif
#include <assert.h>
#include <errno.h>
#include <mex.h>
#include "att.h"
#include "iserver.h"
#include "iserverInt.h"

/*-----------------------------------------------------------------------
  Typedefs
  ----------------------------------------------------------------------- */
typedef struct ad_to_mv_struct {
	double		intercept[AD_MAX_NCHANS];
	double		slope[AD_MAX_NCHANS];
	double		tomv[AD_MAX_NCHANS];
	double		offset[AD_MAX_NCHANS];
	int			loaded;
} AdToMv;

/*-----------------------------------------------------------------------
  Global Variables
  ----------------------------------------------------------------------- */
static int  max_duration = 0;
static AdToMv ad_to_mv;
int ad_inited = 0;
int atexit_registered = 0;
int debugflag=0;


/*-----------------------------------------------------------------------
  Prototypes
  ----------------------------------------------------------------------- */
static short *deinterlace(int, int, int *);
static void iserverExitFcn(void);
static void init_ad_to_mv_struct(AdToMv *data);
static int load_addacal_data(void);
static FILE *open_addacal_file(void);
static void read_addacal_data(FILE *);
static void usage(void);
static void init(int, mxArray *plhs[], int, const mxArray *prhs[]);
static void load(int, mxArray *plhs[], int, const mxArray *prhs[]);
static void play(int, mxArray *plhs[], int, const mxArray *prhs[]);
static void record(int, mxArray *plhs[], int, const mxArray *prhs[]);
static void shutdown(int, mxArray *plhs[], int, const mxArray *prhs[]);
static void att(int, mxArray *plhs[], int, const mxArray *prhs[]);
static void ad_convert(int, mxArray *plhs[], int, const mxArray *prhs[]);
static void dump(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);

static void export_globals(void);
static void update_local_from_globals(void);
static double get_field_double(mxArray *, int, char *);
static int get_field_int(mxArray *, int, char *, int *);
static void set_field_double(mxArray *, int, char *, double);
static void set_field_int(mxArray *, int, char *, int);
static char *get_field_string(mxArray *, int, char *);
static void set_field_string(mxArray *, int, char *, char *);

/*----------------------------------------------------------------------- */
/*----------------------------------------------------------------------- */

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
	char command[30];

	if (nrhs < 1)  
		usage();

	if (!atexit_registered) {
		if (mexAtExit(iserverExitFcn))
			mexPrintf("iserver: iserverExitFcn not registered properly\n");
		atexit_registered=1;
	}

	if (!mxIsChar(prhs[0])) 
		usage();

	if (!ad_inited) {
		init_ad_to_mv_struct(&ad_to_mv);
		ad_inited = 1;
	}

	mxGetString(prhs[0], command, 29);
	if (!strncmp(command,"init",4)) init(nlhs, plhs, nrhs, prhs);

	if (!strncmp(command,"load",4)) load(nlhs, plhs, nrhs, prhs);
	if (!strncmp(command,"play",4)) play(nlhs, plhs, nrhs, prhs);
	if (!strncmp(command,"shutdown",8)) shutdown(nlhs, plhs, nrhs, prhs);
	if (!strncmp(command,"record",6)) record(nlhs, plhs, nrhs, prhs);
	if (!strncmp(command,"ad_convert",10)) ad_convert(nlhs, plhs, nrhs, prhs);
	if (!strncmp(command,"att",3)) att(nlhs, plhs, nrhs, prhs);
	if (!strncmp(command,"dump",3)) dump(nlhs, plhs, nrhs, prhs);


}

/* ------------------------------------------------------------------------
   iserverExitFcn
   ------------------------------------------------------------------------ */
static void iserverExitFcn(void) {

	mexPrintf("iserver: WARNING: MEX-file is being unloaded\n");

	if (is_inited) {
#ifdef __tdtproc__
		mexPrintf("iserver: shutting down tdtproc\n");
#endif /* __tdtproc__ */
#ifdef __drpmproc__
		mexPrintf("iserver: shutting down drpmproc\n");
#endif /* __drpmproc__ */

		if (is_shutdown() != IS_OK) 
			mexPrintf("iserver: is_shutdown() failed.");
	}

	mexUnlock();
	
}


/* ------------------------------------------------------------------------
   usage
   ------------------------------------------------------------------------ */
static void usage(void) 
{
	mexPrintf("usage: fc=iserver('init', <fc>, <len>);\n");
	mexPrintf("       iserver('load', <sound matrix>);\n");
	mexPrintf("       time=iserver('play');\n");
	mexPrintf("       time=iserver('play','left');\n");
	mexPrintf("       time=iserver('play','right');\n");
	mexPrintf("       x=iserver('record');\n");
	mexPrintf("       iserver('shutdown');\n");
	mexPrintf("       [tomv offset]=iserver('ad_convert');\n");
	mexErrMsgTxt("       iserver('att', <att>);\n");
}

/* ------------------------------------------------------------------------
   init
   ------------------------------------------------------------------------ */
static void init(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) 
{
	double fc;
	double *new_fc;
	int epoch;

	if (nrhs != 3)
		mexErrMsgTxt("usage: fc = iserver('init', <fc>, <len>);\n");
	if (nlhs > 1)
		mexErrMsgTxt("usage: fc = iserver('init', <fc>, <len>);\n");

	fc = mxGetScalar(prhs[1]);
	epoch = (int) mxGetScalar(prhs[2]);
#ifdef __tdtproc__
	if (is_init((float) fc, (float) fc, -1.0, epoch, epoch)!=IS_OK) {
#else /* __tdtproc__ */
	if (is_init((float) fc, (float) fc, IS_NONE, epoch, epoch)!=IS_OK) {
#endif /* __tdtproc__ */
		max_duration = 0;
		mexErrMsgTxt("iserver: is_init() failed.");
	} else 
		max_duration = epoch;

	plhs[0]=mxCreateDoubleMatrix(1,1,mxREAL);
	new_fc=mxGetPr(plhs[0]);
	*new_fc = (double) is_daFc;

	export_globals();

	mexLock();
}

/* ------------------------------------------------------------------------
   load
   ------------------------------------------------------------------------ */
static void load(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) 
{
	int m,n,i,len;
	double *sound_data1 = NULL;
	double *sound_data2 = NULL;
	int outsize;
	xword *outbuf;

	if (!is_inited) {
		mexPrintf("iserver: daqprog not started yet!\n");
		mexPrintf("iserver: you must use\n");
		mexPrintf("           iserver('init', <fc>, <len>)\n");
		mexErrMsgTxt("          before calling this function\n");
	}
		
	if (nrhs != 2)
		mexErrMsgTxt("usage: iserver('load', <sound matrix>);\n");
	if (nlhs != 0)
		mexErrMsgTxt("usage: iserver('load', <sound matrix>);\n");

	if ((n=mxGetN(prhs[1])) != DA_MAX_NCHANS) {
		mexPrintf("iserver: <sound matrix> must be a Mx%d matrix\n", 
			DA_MAX_NCHANS);
		mexErrMsgTxt("iserver: unable to load sounds\n");
	}

	update_local_from_globals();

	outbuf = is_getDAbuffer(&outsize);

	m=mxGetM(prhs[1]);
	if (m<(outsize))
		len=m;
	else
		len=outsize;

	sound_data1 = mxGetPr(prhs[1]);
	sound_data2 = sound_data1+m; 

	is_clearOutput(IS_LEFT | IS_RIGHT);

	for (i=0; i<len; i++) {
		outbuf[2*i] = (short) sound_data1[i];
		outbuf[2*i+1] = (short) sound_data2[i];
	}

#ifdef __tdtproc__
	if (is_load(0) != IS_OK) 
		mexErrMsgTxt("iserver: error during is_load!\n");
#endif /* __tdtproc__ */

	export_globals();

}

/* ------------------------------------------------------------------------
   play: returns scalar containing timestamp of playback
   ------------------------------------------------------------------------ */
static void play(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) 
{
	double *timestamp;
	char side[30];
	int flags = 0;

	if (!is_inited) {
		mexPrintf("iserver: daqprog not started yet!\n");
		mexPrintf("iserver: you must use\n");
		mexPrintf("           iserver('init', <fc>, <len>)\n");
		mexErrMsgTxt("          before calling this function\n");
	}
		
	if (nrhs >= 2) {
		mexPrintf("usage: time=iserver('play');\n");
		mexPrintf("  <or> time=iserver('play', 'left');\n");
		mexErrMsgTxt("  <or> time=iserver('play', 'right');\n");
	}
	if (nrhs == 1) {
		flags = 0;
	} else {
		if (mxIsChar(prhs[1])) {
			mxGetString(prhs[1], side, 29);
			if ((strncmp(side,"left",4) != 0) && (strncmp(side,
				"right",5) != 0)) {
				mexPrintf("usage: iserver('play');\n");
				mexPrintf("  <or> iserver('play', 'left');\n");
				mexErrMsgTxt("  <or> iserver('play', 'right');\n");
			} else {
				if (strncmp(side,"left",4) == 0) 
					flags != X_LEFT;
				if (strncmp(side,"right",5) == 0) 
					flags != X_RIGHT;
			}
		} else {
			mexPrintf("usage: time=iserver('play', 'left');\n");
			mexErrMsgTxt("  <or> time=iserver('play', 'right');\n");
		}
	}
	update_local_from_globals();

	plhs[0]=mxCreateDoubleMatrix(1,1,mxREAL);
	timestamp = mxGetPr(plhs[0]);

	if (is_sample(flags, max_duration, timestamp) != IS_OK) {
		*timestamp = -1.0;
		mexErrMsgTxt("iserver: error during is_play!\n");
	}

	export_globals();
}

/* ------------------------------------------------------------------------
   record: returns a (npts)x(ad_nchans) matrix 
           for SunOS, this must be done right after is_play
   ------------------------------------------------------------------------ */
static void record(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) 
{
	double *output;
	short 	*buf = NULL;
	int i,j, size, insize;
	xword *inbuf;


	if (!is_inited) {
		mexPrintf("iserver: daqprog not started yet!\n");
		mexPrintf("iserver: you must use\n");
		mexPrintf("           iserver('init', <fc>, <len>)\n");
		mexErrMsgTxt("          before calling this function\n");
	}
		
	if (nrhs != 1)
		mexErrMsgTxt("usage: X = iserver('record');\n");
	if (nlhs > 1)
		mexErrMsgTxt("usage: X = iserver('record');\n");

#ifdef __tdtproc__
	if (is_record() != IS_OK) 
		mexErrMsgTxt("iserver: error during record!\n");
#endif /* __tdtproc__ */

	update_local_from_globals();

	inbuf = is_getADbuffer(&insize);
	plhs[0]=mxCreateDoubleMatrix(insize,ad_nchans,mxREAL);
	output=mxGetPr(plhs[0]);

	for (i = 0; i < ad_nchans; i++) {
		buf = deinterlace(X_AD, i, &size);
		mxAssert(buf != NULL, "record: buf == NULL!");

		for (j=0; j<size; j++)
			output[j+(i*size)] = buf[j];

		mxFree(buf);
	}

	export_globals();
}


/* ------------------------------------------------------------------------
   shutdown
   ------------------------------------------------------------------------ */
static void shutdown(int nlhs, mxArray *plhs[], int nrhs, 
	const mxArray *prhs[]) 
{

	if (!is_inited) {
		mexPrintf("iserver: daqprog not started yet!\n");
		mexPrintf("iserver: you must use\n");
		mexPrintf("           iserver('init', <fc>, <len>)\n");
		mexErrMsgTxt("          before calling this function\n");
	}
		
	if (nrhs != 1)
		mexErrMsgTxt("usage: iserver('shutdown');\n");
	if (nlhs != 0)
		mexErrMsgTxt("usage: iserver('shutdown');\n");

	if (is_shutdown() != IS_OK) 
		mexErrMsgTxt("iserver: is_shutdown() failed.");

	is_inited=0;

	mexUnlock();
}

/* ------------------------------------------------------------------------
   ad_convert
   ------------------------------------------------------------------------ */
static void ad_convert(int nlhs, mxArray *plhs[], int nrhs, 
	const mxArray *prhs[]) 
{
	int i;
	double *tomv, *offset;
	char reload_s[30];
	int reload = 0;

	if (nrhs > 2) {
		mexPrintf("usage: [tomv offset]=iserver('ad_convert');\n");
		mexErrMsgTxt("  <or> [tomv offset]=iserver('ad_convert', 'reload');\n");
	}
	if (nlhs != 2) {
		mexPrintf("usage: [tomv offset]=iserver('ad_convert');\n");
		mexErrMsgTxt("  <or> [tomv offset]=iserver('ad_convert', 'reload');\n");
	}

	if (nrhs == 2) {
		if (!mxIsChar(prhs[1])) 
			mexErrMsgTxt("usage: [tomv offset]=iserver('ad_convert', 'reload');\n");
		mxGetString(prhs[1], reload_s, 29);
		if (!strncmp(reload_s, "reload", 6))
			mexErrMsgTxt("usage: [tomv offset]=iserver('ad_convert', 'reload');\n");
		reload = 1;
	}

	if (!ad_to_mv.loaded || reload)
		if (!load_addacal_data()) 
			mexErrMsgTxt("iserver: unable to load addacal data\n");

	plhs[0] = mxCreateDoubleMatrix(1, ad_nchans, mxREAL);
	tomv = mxGetPr(plhs[0]);
	plhs[1] = mxCreateDoubleMatrix(1, ad_nchans, mxREAL);
	offset = mxGetPr(plhs[1]);

	for (i=0; i<ad_nchans; i++) {
		tomv[i] = ad_to_mv.tomv[i];
		offset[i] = ad_to_mv.offset[i];
	}
}
	
/* ------------------------------------------------------------------------
   att
   ------------------------------------------------------------------------ */
static void att(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) 
{
	int			side = IS_NONE;
	double 		latten, ratten;
	double 		*att;
	char		side_string[30];

	if ((nrhs > 2) ) {
		mexPrintf("usage: iserver('att',<val>);\n");
		mexErrMsgTxt("       val=iserver('att');\n");
	}
	if ((nrhs > 2) && (nlhs==0)) {
		mexErrMsgTxt("usage:   iserver('att',<val>);\n");
	} else {
		if ((nrhs > 1) && (nlhs > 1)) {
			mexErrMsgTxt("       val=iserver('att');\n");
		}
	}

	if (nlhs == 1) {
		plhs[0] = mxCreateDoubleMatrix(1,2,mxREAL);
		att = mxGetPr(plhs[0]);
		att[0] = latten;
		att[1] = ratten;
	} else {
		if (mxGetN(prhs[1]) != 2)
			mexErrMsgTxt("    iserver('att',<val>): <val> must be 1x2\n");
		att = mxGetPr(prhs[1]);
		latten = att[0];
		ratten = att[1];
		if (!setRack(0.0, (float )latten, (float )ratten)) 
			mexErrMsgTxt("iserver: unable to set attenuators!");
	} 

}

/* ------------------------------------------------------------------------
   dump
   ------------------------------------------------------------------------ */
static void dump(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) 
{
	int i,j,size;
	double *input, *output;
	short *buf;


	if (!is_inited) {
		mexPrintf("iserver: daqprog not started yet!\n");
		mexPrintf("iserver: you must use\n");
		mexPrintf("           iserver('init', <fc>, <len>)\n");
		mexErrMsgTxt("          before calling this function\n");
	}
		
	if (nrhs != 1)
		mexErrMsgTxt("usage: [dabuf, adbuf] = iserver('dump');\n");
	if (nlhs != 2)
		mexErrMsgTxt("usage: [dabuf, adbuf] = iserver('dump');\n");

	update_local_from_globals();

	buf=is_getDAbuffer(&size);
	plhs[0]=mxCreateDoubleMatrix(size,is_getDAnchans(),mxREAL);
	output=mxGetPr(plhs[0]);

	buf=is_getADbuffer(&size);
	plhs[1]=mxCreateDoubleMatrix(size,is_getADnchans(),mxREAL);
	input=mxGetPr(plhs[1]);

	for (i = 0; i < da_nchans; i++) {
		buf = deinterlace(X_DA, i, &size);

		mxAssert((buf != NULL), "dump: buf == NULL!");

		for (j=0; j<size; j++)
			output[j+(i*size)] = buf[j];

		mxFree(buf);
	}

	for (i = 0; i < ad_nchans; i++) {
		buf = deinterlace(X_AD, i, &size);
		mxAssert((buf != NULL), "dump: buf == NULL!");

		for (j=0; j<size; j++)
			input[j+(i*size)] = buf[j];

		mxFree(buf);
	}


	export_globals();

}

/* -------------------------------------------------------------------- 
   init_ad_to_mv_struct:  initialize local A/D tick to mv struct
   -------------------------------------------------------------------- */
static void init_ad_to_mv_struct(AdToMv *data)
{
	int i;

	for (i=0; i<AD_MAX_NCHANS; i++) {
		data->slope[i] = 0.0;
		data->intercept[i] = 0.0;
		data->tomv[i] = 0.0;
		data->offset[i] = 0.0;
	}

	data->loaded=0;
}

/* -------------------------------------------------------------------- 
   load_addacal_data:  load A/D tick to mv conversion data from an xdphys 
   						addacal file
   -------------------------------------------------------------------- */
static int load_addacal_data(void)
{
	FILE *fp = NULL;
	int i;

	if ((fp = open_addacal_file()) != NULL) {
		read_addacal_data(fp);
		fclose(fp);

		for (i=0; i<AD_MAX_NCHANS; i++) {
			if (ad_to_mv.slope[i] != 0.0) {
				ad_to_mv.tomv[i] = 1.0/ad_to_mv.slope[i];
				ad_to_mv.offset[i] = -(ad_to_mv.intercept[i]/ad_to_mv.slope[i]);
			} else {
				ad_to_mv.tomv[i] = 0.0;
				ad_to_mv.offset[i] = 0.0;
			}
		}
		ad_to_mv.loaded = 1;
		return(1);
	}

	return(0);
}
 
/* -------------------------------------------------------------------- 
   open_addacal_file:  open an xdphys addacal file
   -------------------------------------------------------------------- */
static FILE *open_addacal_file(void) 
{
	char addacal_path[MAXPATHLEN];
	char *host;
	char *xdphysdir;
	struct stat buf;
	FILE  *fp = NULL;
	char msgbuf[255];

	if ((host = getenv("HOST")) == NULL) 
		mexErrMsgTxt("iserver: please set the HOST environment variable\n");

	if ((xdphysdir = getenv("XDPHYSDIR")) == NULL)
		mexErrMsgTxt("iserver: please set the XDPHYSDIR environment variable");

	sprintf(addacal_path, "%s/calib/%s.addacal.data",xdphysdir,host);

	if (stat(addacal_path, &buf) != 0) {
		mexPrintf("iserver: %s does not exist\n", addacal_path);
		mexErrMsgTxt("iserver: unable to read addacal data\n");
	}

	if ((fp = fopen(addacal_path, "r")) == NULL) {
#ifdef __linux__
		mexPrintf("iserver: error opening %s: %s", addacal_path, 
			strerror(errno));
#else /* __linux__ */
		mexPrintf("iserver: error opening %s: %d", addacal_path, 
			errno);
#endif /* __linux__ */
		mexErrMsgTxt("iserver: unable to read addacal data\n");
	}

	return(fp);
}

/* -------------------------------------------------------------------- 
   read_addacal_data:  load A/D tick to mv conversion data from an xdphys 
   						addacal file
   -------------------------------------------------------------------- */
#define get_value(l,x) if (strcmp(line,l)==0) sscanf(value,"%lf",&x);
static void read_addacal_data(FILE *fp) 
{
	char line[1000], *value, *eol;
	char label[80];
	int i;

	mxAssert(fp != NULL, "read_addacal_data: fp == NULL!");
	mxAssert((AD_MAX_NCHANS >= 2), "read_addacal_data: AD_MAX_NCHANS < 2");

	while (fgets(line, sizeof(line), fp) != NULL) {
		/* Skip comments */
		/*mexPrintf("%s\n",line); */
		if ((value = index(line, '#')) != NULL)
			continue;

		if ((value = index(line, '=')) != NULL) {
			*value = '\0';
			if ((eol = index(++value, '\n')) != NULL)
				*eol = '\0';

			/* Handle old labels */
			get_value("ad.left.slope", ad_to_mv.slope[0]);
			get_value("ad.left.intercept", ad_to_mv.intercept[0]);
			get_value("ad.right.slope", ad_to_mv.slope[1]);
			get_value("ad.right.intercept", ad_to_mv.intercept[1]);

			/* Handle new channel labels */
			for (i=0; i<AD_MAX_NCHANS; i++) {
				sprintf(label, "ad.%d.slope", i);
				get_value(label, ad_to_mv.slope[i]);
				sprintf(label, "ad.%d.intercept", i);
				get_value(label, ad_to_mv.intercept[i]);
			}
		}
	}
}
#undef get_value

/* ------------------------------------------------------------------------
   deinterlace
   ------------------------------------------------------------------------ */
static short *deinterlace(int data, int offset, int *size) 
{
	int  i, j=0;
	short *buf = NULL;
	xword *buffer;
	int nchans;

	mxAssert(((data == X_DA) || (data == X_AD)), 
		"deinterlace: data != X_DA && data != X_AD.");

	if (data == X_DA) {
		buffer = is_getDAbuffer(size);
		nchans = is_getDAnchans();
	} else {
		buffer = is_getADbuffer(size);
		nchans = is_getADnchans();
	} 

	mxAssert((buffer != NULL), "deinterlace: buffer == NULL!");
	mxAssert(((buf = (short *) mxCalloc(*size, sizeof(short))) != NULL), 
		"deinterlace: mxCalloc returns NULL!");

	for (i=offset; i<(nchans*(*size)); i+=nchans, j++)
		buf[j] = buffer[i];

	return(buf);
}
 

/* ------------------------------------------------------------------------
   export_globals
   ------------------------------------------------------------------------ */
static void export_globals(void)
{
	mxArray *globals = NULL;
	char *field_names[] = {"adFc", "daFc", "ad_nchans", "da_nchans", "epoch",
		"attMaxAtten", "debug"};
	int nfields = 7;
	int dims[2] = {1,1};
	int ndims = 2;

	globals = mxCreateStructArray(ndims, dims, nfields, 
		(const char **)field_names);
	mxSetName(globals, "is_info");

	mxAssert((globals != NULL), "iserver: unable to get or allocate is_info!");

	set_field_int(globals, 0, "adFc", is_adFc);
	set_field_int(globals, 0, "daFc", is_daFc);
	set_field_int(globals, 0, "ad_nchans", is_getADnchans());
	set_field_int(globals, 0, "da_nchans", is_getDAnchans());
	set_field_int(globals, 0, "epoch", max_duration);
	set_field_int(globals, 0, "attMaxAtten", attMaxAtten);
	set_field_int(globals, 0, "debug", debugflag);

	mexPutArray(globals, "global");
}

/* ------------------------------------------------------------------------
   set_field_double
   ------------------------------------------------------------------------ */
static void set_field_double(mxArray *data, int index, char *name, double val)
{
	double *new = NULL;
	mxArray *mat = NULL;
	mxArray *old = NULL;

	mat = mxCreateDoubleMatrix(1,1,mxREAL);
	new = mxGetPr(mat);
	*new = val;
	old = mxGetField(data, 0, name);
	if (old != NULL)
		mxDestroyArray(old);
	mxSetField(data, 0, name, mat);
}

/* ------------------------------------------------------------------------
   set_field_string
   ------------------------------------------------------------------------ */
static void set_field_string(mxArray *data, int index, char *name, char *val)
{
	mxArray *mat = NULL;
	mxArray *old = NULL;
	char test[256];

	mat = mxCreateString(val);
	mxGetString(mat, test, 256);
	old = mxGetField(data, 0, name);
	if (old != NULL)
		mxDestroyArray(old);
	mxSetField(data, 0, name, mat);
}

/* ------------------------------------------------------------------------
   set_field_int
   ------------------------------------------------------------------------ */
static void set_field_int(mxArray *data, int index, char *name, int val)
{
	double *new = NULL;
	mxArray *mat = NULL;
	mxArray *old = NULL;
	int field_num;

	mat = mxCreateDoubleMatrix(1,1,mxREAL);
	new = mxGetPr(mat);
	*new = (double) val;
	old = mxGetField(data, 0, name);
	if (old != NULL)
		mxDestroyArray(old);
	mxSetField(data, 0, name, mat);
}

/* ------------------------------------------------------------------------
   update_local_from_globals
   ------------------------------------------------------------------------ */
static void update_local_from_globals(void)
{
	mxArray *globals = NULL;

	if ((globals = mexGetArray("is_info", "global")) == NULL) {
		export_globals();
		return;
	}

	mxAssert((globals != NULL), 
		"iserver: unable to get or allocate is_info!");

	get_field_int(globals, 0, "debug", &debugflag);
}

/* ------------------------------------------------------------------------
   get_field_int
   ------------------------------------------------------------------------ */
static int get_field_int(mxArray *data, int index, char *name, int *val)
{
	mxArray *old = NULL;
	double *tmp;

	old = mxGetField(data, 0, name);
	if (old) {
		tmp = mxGetPr(old);
		*val = (int) *tmp;
		return(1);
	}

	return(0);
}

/* ------------------------------------------------------------------------
   get_field_double
   ------------------------------------------------------------------------ */
static double get_field_double(mxArray *data, int index, char *name)
{
	mxArray *old = NULL;
	double *val;

	old = mxGetField(data, 0, name);
	val = mxGetPr(old);
	return(*val);
}

/* ------------------------------------------------------------------------
   get_field_string: remember to free the string when you're done
   ------------------------------------------------------------------------ */
static char *get_field_string(mxArray *data, int index, char *name)
{
	mxArray *old = NULL;
	char *val;

	mxAssert(((val = (char *) mxCalloc(255, sizeof(char))) != NULL),
		"get_field_string: mxCalloc returns NULL!");
	old = mxGetField(data, 0, name);
	mxGetString(old, val, 255);
	return(val);
}
