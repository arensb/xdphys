#ifdef COMP32
#define qint short int
#else
#define qint int
#endif

/* Overhead */
#ifdef INTMOVE
void DataIn(unsigned qint ioport, unsigned qint qseg,
  unsigned qint qofs, unsigned int count);
void DataOut(unsigned qint qseg, unsigned qint qofs,
  unsigned qint ioport, unsigned int count);
#else
void far DataIn(unsigned qint ioport, unsigned qint qseg,
  unsigned qint qofs, unsigned qint count);
void far DataOut(unsigned qint qseg, unsigned qint qofs,
  unsigned qint ioport, unsigned qint count);
#endif
void GetErr( void );
void QAPerror( char *emess );
qint CmdOut(qint cmd);
long rec24(void);
void send24(long val);
void sendflt(float val);
float recflt( void );
void SendData(qint buf[], long adr, long nwds);
void ReceiveData(qint buf[], long adr, long nwds);
void P0(qint cmd, char caller[]);
void P1(qint cmd, float arg1, char caller[]);
void P2(qint cmd, float arg1, float arg2, char caller[]);

/* Startup */

/* Data Handlers and Startup */
qint apinit(qint devtype);
void push16(qint buf[], long npts);
void pushf(float buf[], long npts);
void pop16(qint buf[]);
void popf(float buf[]);
void dpush(long npts);
void block(long sp,long ep);
void noblock( void );
void swap( void );
void qdup( void );
void allotf(long bid, long npts);
void trash( void );
void qpopf( long bid);
void qpushf( long bid);
void drop( void );
void dropall( void );
void makefast(qint pid);
void popport16(qint pp);
void popportf(qint pp);
void pushport16(qint pp, long npts);
void pushportf(qint pp, long npts);
void popdisk16(char *fname);
void popdiskf(char *fname);
void popdiska(char *fname);
void pushdisk16(char *fname);
void pushdiskf(char *fname);
void pushdiska(char *fname);
void poppush(qint s ,qint d);
void pushpoptest(qint buf[], long adr, long npts);
void extact(void);
void dupn(qint n);
void cat(void);
void catn(qint n);
void allot16( long bid, long npts);
void qpop16( long bid);
void qpush16( long bid);
void dama2disk16( long bid, char *fname, qint catflag);
void disk2dama16( long bid, char *fname, long seekpos);
void extract( void );
void reduce( void );
void totop( long sn );
void qpushpart16( long bid, long spos, long npts);
void qpushpartf( long bid, long spos, long npts);
void qpoppart16( long bid, long spos);
void qpoppartf( long bid, long spos);
void makedama16(qint bid, long ind, qint v);
void makedamaf(qint bid, long ind, float v);
void deallot(long bid);
long getaddr(qint bid);
void setaddr(qint bid, long addr);
long _allotf(long npts);
long _allot16(long npts);

/* Generators */
void fill(float start, float step);
void gauss( void );
void qrand( void );
void flat( void );
void seed( long sval );
void value( float  v);
void tone(float f,float sr);
void make(long ind, float v);
float whatis(long ind);
void parse(char s[]);

/* Complex Operations */
void xreal( void );
void ximag( void );
void polar( void );
void rect( void );
void shuf( void );
void split( void );
void cmult( void );
void cadd( void );
void cinv( void );

/* Basic math */
void scale(float sf);
void shift(float sf );
void logten( void );
void loge( void );
void logn(float base);
void alogten( void );
void aloge( void );
void add( void );
void subtract( void );
void mult( void );
void divide( void );
void sqroot( void );
void square( void );
void power(float pw);
void inv( void );
void maxlim(float max);
void minlim(float min);
void maglim(float max);
void radd( void );
void absval( void );

/* FFT */
void cfft( void );
void cift( void );
void hann( void );
void hamm( void );
void rfft( void );
void rift( void );
void seperate( void );

/* Trig */
void cosine( void );
void sine( void );
void tangent( void );
void acosine( void );
void asine( void );
void atangent( void );
void atantwo( void );
void qwind(float trf, float sr);

/* Sumation */
float sum( void );
float average( void );
float maxval( void );
float minval( void );
float maxmag( void );
long maxval_( void );
long minval_( void );
long maxmag_( void );

/* Status */
long freewords( void );
qint ap_present(qint dn);
qint ap_active(qint dn);
void ap_select(qint dn);
long topsize( void );
long stackdepth( void );
void optest( void );
long lowblock( void );
long hiblock( void );
long tsize(long bufn);

/* Digital Filtering */
void iir(void);
void fir(void);
void _iir(void);
void _fir(void);
void tdt_reverse( void );

/* Play/Record	   */
void play(qint dbn);
void record(qint dbn);
void dplay(qint dbn1, qint dbn2);
void drecord(qint dbn1, qint dbn2);
void fastrecord(qint dbn);
void seqplay(qint dbn);
void seqrecord(qint dbn);
void mrecord(qint dbn);
void mplay(qint dbn);
qint playseg( long chan);
qint recseg( long chan);
long playcount( long chan);
long reccount( long chan);
void chgplay( qint dbn);
void pfireone( qint dbn);
void pfireall(void);
qint ppausestat( qint dbn);

/* Misc Calls  */
void plotmap( long xx1, long yy1, long xx2, long yy2);
void plotwith( long xx1, long yy1, long xx2, long yy2, float ymin, float ymax);
void plotwithCS( long xx1, long yy1, long xx2, long yy2, float ymin, float ymax);
void usercall( long cid, float argf, long arg24);
float userfunc( long cid, float argf, long arg24);

/* More Math Calls  */
void cumsum( void );
void decimate( qint fact);
void interpol( qint fact);
void foldnadd( qint artflag);
qint getnarts( void);


/* Macro Calls	*/
void recordmac(qint bid);
void runmac(qint bid);
void endmac(void);
qint whatmac(void);
qint whatstep(void);
void stopmac(void);
void ls(qint labn);
long stepval24(qint macn, qint stepn);
float stepvalf(qint macn, qint stepn);
void if24(qint vstep1, qint cc, qint vstep2, qint desstep);
void iff(qint vstep1, qint cc, qint vstep2, qint desstep);
void loopn(qint stepn, qint n);
void jump(qint stepn);
void usestepval(qint argn);
void usedamalist(qint argn);
void counter24(long start, long step);
void counterf(float start, float step);
void constant(long lv, float fv);


/*   Handshake Codes  */
#define	      READY	 1110
#define	      RECEIVED	 1120
#define	      DATARDY	 1130
#define	      ERRFLAG	 1140
#define	      ERRACK	 1150
#define	      CHARRDY	 1160
#define	      XFERERR	 1170
#define	      DONE	 1190
#define	      STOPMAC	 1310
#define	      MACACT	 1320

#define	      LOW_UP	 0x111
#define	      HIGH_UP	 0x222
#define	      ACK	 0x333

/* Data Handlers and Startup */
#define	   INITx      1
#define	   PUSHx      2
#define	   POPx	      3
#define	   QPOPFx     4
#define	   QPUSHFx    5
#define	   SNG_DSPx   6
#define	   DSP_SNGx   7
#define	   INT_DSPx   8
#define	   DSP_INTx   9
#define	   BLOCKx     10
#define	   NOBLOCKx   11
#define	   SWAPx      12
#define	   QDUPx      13
#define	   DROPx      14
#define	   DROPALLx   15
#define	   ALLOTFx    16
#define	   TRASHx     17
#define	   EXTRACTx   18
#define	   CATx	      19
#define	   CATNx      20
#define	   DUPNx      21
#define	   ALLOT16x   22
#define	   QPOP16x    23
#define	   QPUSH16x   24
#define	   TOTOPx     25
#define	   QPUSHPARTFx	26
#define	   QPUSHPART16x 27
#define	   QPOPPARTFx	28
#define	   QPOPPART16x	29
#define	   DPUSHx			30
#define	   MAKEDAMA16x	31
#define	   MAKEDAMAFx	32
#define	   DAMA2DAMAx	33
#define	   DEALLOTx	34
#define	   REDUCEx	35
#define	   GETADDRx	36
#define	   SETADDRx	37
#define	   _ALLOT16x	38
#define	   _ALLOTFx	39


/* Complex Operators */
#define	       XREALx	  41
#define	       XIMAGx	  42
#define	       POLARx	  43
#define	       RECTx	  44
#define	       SHUFx	  45
#define	       SPLITx	  46
#define	       CMULTx	  47
#define	       CADDx	  48
#define	       CINVx	  49

/* Basic Math */
#define	       SCALEx	  61
#define	       SHIFTx	  62
#define	       LOG10x	  63
#define	       LOGEx	  64
#define	       LOGNx	  65
#define	       ALOG10x	  66
#define	       ALOGEx	  67
#define	       SQUAREx	  68
#define	       ADDx	  69
#define	       SUBTRACTx  70
#define	       MULTx	  71
#define	       DIVIDEx	  72
#define	       SQROOTx	  73
#define	       POWERx	  74
#define	       INVx	  75
#define	       MAXLIMx	  76
#define	       MINLIMx	  77
#define	       MAGLIMx	  78
#define	       RADDx	  79
#define	       ABSVALx	  80

/* FFT */
#define	       CFFTx	  82
#define	       HANNx	  83
#define	       HAMMx	  84
#define	       CIFTx	  85
#define	       RFFTx	  86
#define	       RIFTx	  87
#define	       SEPERATEx  88

/* TRIG */
#define	       COSINEx	  101
#define	       SINEx	  102
#define	       TANGENTx	  103
#define	       ACOSINEx	  104
#define	       ASINEx	  105
#define	       ATANGENTx  106
#define	       ATANTWOx	  107
#define	       QWINDx	  108

/* SUM */
#define	       MAXVALx	  121
#define	       MINVALx	  122
#define	       MAXMAGx	  123
#define	       MAXVAL_x	  124
#define	       MINVAL_x	  125
#define	       MAXMAG_x	  126
#define	       SUMx	  127
#define	       AVERAGEx	  128

/* Digital Filtering */
#define	       IIRx	  141
#define	       FIRx	  142
#define	       _IIRx	  143
#define	       _FIRx	  144
#define	       REVERSEx	  145

/* Status */
#define	       FREEWORDSx  161
#define	       TOPSIZEx	   162
#define	       STACKDEPTHx 163
#define	       DAMAINFOx   164
#define	       OPTESTx	   165
#define	       LOWBLOCKx   166
#define	       HIBLOCKx	   167
#define	       TSIZEx	   168

/* Generators */
#define	       FILLx	  181
#define	       GAUSSx	  182
#define	       RANDx	  183
#define	       FLATx	  184
#define	       SEEDx	  185
#define	       VALUEx	  186
#define	       TONEx	  187
#define	       MAKEx	  188
#define	       WHATISx	  189

/* Play/Record		       */
#define	       PLAYx	   201
#define	       RECORDx	   202
#define	       SEQPLAYx	   203
#define	       SEQRECORDx  204
#define	       PLAYSEGx	   205
#define	       RECSEGx	   206
#define	       FASTRECORDx 207
#define	       DPLAYx	   208
#define	       DRECORDx	   209
#define	       MRECORDx	   210
#define	       PLAYCOUNTx  211
#define	       RECCOUNTx   212
#define	       MPLAYx	   213
#define	       CHGPLAYx	   214
#define	       PFIREONEx   215
#define	       PFIREALLx   216
#define	       PPAUSESTATx 217


/* Misc			     */
#define	       PLOTMAPx	   221
#define	       PLOTWITHx   222
#define	       PLOTWITHCSx 223
#define				 USERCALLx	 230
#define	       USERFUNCx   231

/* More Math		     */
#define	       CUMSUMx	   241
#define	       DECIMATEx   242
#define	       INTERPOLx   243
#define	       FOLDNADDx   244
#define	       GETNARTSx   245

/* Macro Calls		     */
#define	       RECORDMACx   261
#define	       RUNMACx	    262
#define	       ENDMACx	    263
#define	       LABSTEPx	    264
#define	       STEPVAL24x   265
#define	       STEPVALFx    266
#define	       IF24x	    267
#define	       IFFx	    268
#define	       LOOPNx	    269
#define	       USESTEPVALx  270
#define	       USEDAMALISTx 271
#define	       COUNTER24x   272
#define	       COUNTERFx    273
#define	       CONSTANTx    274
#define	       JUMPx	    275

#define	       EQ	   1
#define	       NEQ	   2
#define	       LT	   3
/* #define	       GT	   4 */
#define			   LT_EQ       5
#define	       GT_EQ	   6

#define	       STEP1	     1
#define	       STEP2	     2
#define	       STEP3	     3
#define	       STEP4	     4
#define	       STEP5	     5
#define	       STEP6	     6
#define	       STEP7	     7
#define	       STEP8	     8
#define	       STEP9	     9
#define	       STEP10	  10

#define	       ARG1	     1
#define	       ARG2	       2
#define	       ARG3	   3
#define	       ARG4	       4
#define	       ARG5	       5

/* Misc. Constants */
#define	       TIMEOUT	   10000
#define	       TBUFSIZE	   5000
#define	       PERROR	   1
#define	       MAXCMDS	   300
#define	       APa	   0x311
#define	       APb	   0x312

extern	       qint	   qapsel;
extern	       qint	   ap_eflag,ap_emode;
extern	       char	   ap_err[10][100];
