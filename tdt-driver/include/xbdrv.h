#ifdef COMP32
#define qint short int
#else
#define qint int
#endif

/**********************************************************
  User programmable control constants
 **********************************************************/
#define XB_MAX_DEV_TYPES      32
#define XB_MAX_OF_EACH_TYPE   16
#define XB_MAX_NUM_RACKS     120
#define XB_TIMEOUT	    3000
#define XB_MAX_TRYS	       3
#define XB_WAIT_FOR_ID	      40

/**********************************************************
  Device ID codes
 **********************************************************/
#define PA4_CODE       0x01
#define SW2_CODE       0x02
#define CG1_CODE       0x03
#define SD1_CODE       0x04
#define ET1_CODE       0x05
#define PI1_CODE       0x06
#define UI1_CODE       0x07
#define WG1_CODE       0x08
#define PF1_CODE       0x09
#define TG6_CODE       0x0a
#define PI2_CODE       0x0b
#define WG2_CODE       0x0c
#define XXX_CODE       0x0d
#define SS1_CODE       0x0e
#define PM1_CODE       0x0f
#define HTI_CODE       0X1a

#define DA1_CODE       0x10
#define AD1_CODE       0x11
#define DD1_CODE       0x12
#define DA2_CODE       0x13
#define AD2_CODE       0x14
#define AD3_CODE       0x15
#define DA3_CODE       0x16
#define PD1_CODE       0x17

/**********************************************************
  Global communication/control constants
 **********************************************************/
#define COM1	 1
#define COM2	 2
#define COM3	 3
#define COM4	 4
#define COM_AP2	 5
#define COM_AP2x 6
#define COM_APa	 5
#define COM_APb	 6
#define USE_DOS	 0

#define COM1base 0x3f8
#define COM2base 0x2f8
#define COM3base 0x3e8
#define COM4base 0x2e8
#define APabase	 0x230
#define APbbase	 0x250

#define SNOP	      0x00
#define VER_REQUEST   0x06
#define XTRIG		      0x07
#define IDENT_REQUEST 0x08
#define HOST_RTR      0x09

#define ARB_ERR	      0xC0
#define HOST_ERR      0xC1
#define ERR_ACK	      0xC2
#define SLAVE_ACK     0xC3
#define HARD_RST      0xC5
#define SLAVE_ERR     0xC6
#define BAUD_LOCK     0xCA
#define ARB_ACK	      0xCB
#define ARB_ID	      0xCC
#define ARB_RST	      0xCD
#define GTRIG	      0xD2
#define LTRIG	      0xD3
#define ARB_VER_REQ   0xD4

/**********************************************************
  Misc. Stuff.
 **********************************************************/
void processerr( qint li, qint ne);
void showerr( char eee[]);
void getxln(qint din, qint dtype, char descrip[]);

extern qint cc,cpa[2],xbsel;
extern long xbtimeout;
typedef char xbstring[5];
extern xbstring xbname[XB_MAX_DEV_TYPES+1];
extern unsigned char xbcode[XB_MAX_DEV_TYPES+1][XB_MAX_OF_EACH_TYPE+1][2];
extern qint xb_eflag, xb_emode;
extern char xb_err[10][50];
extern qint dadev,addev;

void iosend(qint datum);
qint iorec( void );

/**********************************************************
  XB1 Drivers
 **********************************************************/
qint XB1init( qint cptr );
void XB1flush( void );
qint XB1device( qint devcode, qint dn);
void XB1gtrig( void );
void XB1ltrig( qint rn );
qint XB1version( qint devcode, qint dn );
void XB1select( qint cptr );


/**********************************************************
  PA4 Drivers
 **********************************************************/
#define PA4_AC	    0x11
#define PA4_DC	    0x12
#define PA4_AUTO		0x13
#define PA4_MAN	    0x14
#define PA4_MUTE    0x15
#define PA4_NOMUTE  0x16
#define PA4_SETUP   0x17
#define PA4_READ    0x18
#define PA4_ATT	    0x20

void PA4atten(qint din, float atten);
void PA4setup(qint din, float base, float step);
void PA4mute(qint din);
void PA4nomute(qint din);
void PA4ac(qint din);
void PA4dc(qint din);
void PA4man(qint din);
void PA4auto(qint din);
float PA4read(qint din);


/**********************************************************
  SW2 Drivers
 **********************************************************/
#define SW2_ON	    0x11
#define SW2_OFF	    0x12
#define SW2_TON	    0x13
#define SW2_TOFF    0x14
#define SW2_RFTIME  0x15
#define SW2_SHAPE   0x16
#define SW2_TRIG    0x17
#define SW2_DUR	    0x18
#define SW2_STATUS  0x19
#define SW2_CLEAR   0x20

void SW2on(qint din);
void SW2off(qint din);
void SW2ton(qint din);
void SW2toff(qint din);
void SW2rftime(qint din, float rftime);
void SW2shape(qint din, qint shcode);
  #define ONOFF	     0
  #define COS2	     1
  #define COS4	     2
  #define COS6	     3
  #define RAMP	     4
  #define RAMP2	     5
  #define RAMP4	     6
  #define RAMP6	     7
void SW2trig(qint din, qint tcode);
  #define MANUAL     0
  #define COMPUTER   0
  /* see CG1 const #5 not valid */
void SW2dur(qint din, qint dur);
qint SW2status(qint din);
void SW2clear(qint din);

/**********************************************************
  ET1 Drivers
 **********************************************************/
#define ET1_CLEAR     0x11
#define ET1_MULT      0x12
#define ET1_COMPARE   0x13
#define ET1_EVCOUNT   0x14
#define ET1_GO	      0x15
#define ET1_STOP      0x16
#define ET1_ACTIVE    0x17
#define ET1_DROP      0x18
#define ET1_BLOCKS    0x19
#define ET1_XLOGIC    0x1a
#define ET1_REPORT    0x20
#define ET1_READ32    0x21
#define ET1_READ16    0x22

int ET1readall32(qint din, unsigned long **times);
int ET1readall16(qint din, qint **times);
void ET1clear(qint din);
void ET1mult(qint din);
void ET1compare(qint din);
void ET1evcount(qint din);
void ET1go(qint din);
void ET1stop(qint din);
qint ET1active(qint din);
void ET1blocks(qint din,qint nblocks);
void ET1xlogic(qint din,qint lmask);
long ET1report(qint din);
unsigned long ET1read32(qint din);
qint ET1read16(qint din);
void ET1drop(qint din);
void ET1up(qint din, char cbuf[]);
void ET1down(qint din, char cbuf[]);

/**********************************************************
  SD1 Drivers
 **********************************************************/
#define SD1_GO	      0x11
#define SD1_STOP      0x12
#define SD1_ENABLE    0x13
#define SD1_NOENABLE  0x14
#define SD1_HOOP      0x15
#define SD1_NUMHOOPS  0x16
#define SD1_COUNT     0x17
#define SD1_UP	      0x18
#define SD1_DOWN      0x19

void SD1go(qint din);
void SD1stop(qint din);
void SD1use_enable(qint din);
void SD1no_enable(qint din);
void SD1hoop(qint din, qint num,
	qint slope, float dly, float width, float upper, float lower);
    #define RISE   1
    #define FALL   2
    #define PEAK   3
    #define VALLEY 4
    #define ANY	   5
void SD1numhoops(qint din, qint nh);
qint SD1count(qint din);
void SD1up(qint din, char cbuf[]);
void SD1down(qint din, char cbuf[]);

/**********************************************************
  CG1 Drivers
 **********************************************************/
#define CG1_GO	      0x11
#define CG1_STOP      0x12
#define CG1_REPS      0x13
#define CG1_TRIG      0x14
#define CG1_PERIOD    0x15
#define CG1_PULSE     0x16
#define CG1_ACTIVE    0x17
#define CG1_PATCH     0x18
#define CG1_TGO	      0x19

void CG1go( qint din);
void CG1stop( qint din);
void CG1reps( qint din, qint reps);
void CG1trig( qint din, qint ttype);
    #define POS_EDGE 1
    #define NEG_EDGE 2
    #define POS_ENABLE 3
    #define NEG_ENABLE 4
    #define FREE_RUN 5
    #define NONE 5
void CG1period( qint din, float period);
void CG1pulse( qint din, float on_t, float off_t);
char CG1active( qint din);
void CG1patch( qint din, qint pcode);
    #define XTRG1 1
    #define XTRG2 2
    #define XCLK1 3
    #define XCLK2 4
    #define EXT	  5
void CG1tgo( qint din);


/**********************************************************
  PI1 Drivers
 **********************************************************/
#define PI1_CLEAR     0x11
#define PI1_OUTS      0x12
#define PI1_LOGIC     0x13
#define PI1_WRITE     0x14
#define PI1_READ      0x15
#define PI1_AUTOTIME  0x16
#define PI1_MAP	      0x17
#define PI1_LATCH      0x18
#define PI1_DEBOUNCE  0x19
#define PI1_STROBE    0x1a
#define PI1_OPTREAD    0x1b
#define PI1_OPTWRITE  0x1c

void PI1clear( qint din);
void PI1outs( qint din, qint omask);
void PI1logic( qint din, qint logout, qint login);
void PI1write( qint din, qint bitcode);
qint PI1read( qint din);
void PI1autotime( qint din, qint atmask, float dur);
void PI1map( qint din, qint incode, qint mapoutmask);
void PI1latch( qint din, qint latchmask);
void PI1debounce( qint din);
void PI1strobe( qint din);
qint PI1optread( qint din);
void PI1optwrite( qint din, qint bitcode);


/**********************************************************
  PI2 Drivers
 **********************************************************/
#define PI2_CLEAR     0x11
#define PI2_OUTS      0x12
#define PI2_LOGIC     0x13
#define PI2_WRITE     0x14
#define PI2_READ      0x15
#define PI2_DEBOUNCE  0x16
#define PI2_AUTOTIME  0x17
#define PI2_SETBIT    0x18
#define PI2_CLRBIT    0x19
#define PI2_ZEROTIME  0x1a
#define PI2_GETTIME   0x1b
#define PI2_LATCH     0x1c
#define PI2_MAP	      0x1d
#define PI2_OUTSX     0x1e
#define PI2_WRITEX    0x1f
#define PI2_READX     0x20
#define PI2_TOGGLE    0x21

void PI2clear( qint din);
void PI2outs( qint din, qint omask);
void PI2logic( qint din, qint logout, qint login);
void PI2write( qint din, qint bitcode);
qint PI2read( qint din);
void PI2debounce( qint din, qint dbtime);
void PI2autotime(qint din, qint bitn, qint dur);
void PI2setbit( qint din, qint bitmask);
void PI2clrbit( qint din, qint bitmask);
void PI2zerotime( qint din, qint bitmask);
qint PI2gettime( qint din, qint bitn);
void PI2latch( qint din, qint lmask);
void PI2map( qint din, qint bitn, qint mmask);
void PI2outsX( qint din, qint pnum);
void PI2writeX( qint din, qint pnum, qint val);
qint PI2readX( qint din, qint pnum);
void PI2toggle( qint din, qint tmask);


/**********************************************************
  WG1 Drivers
 **********************************************************/
#define WG1_ON	      0x11
#define WG1_OFF	      0x12
#define WG1_CLEAR     0x13
#define WG1_AMP	      0x14
#define WG1_FREQ      0x15
#define WG1_SWRT      0x16
#define WG1_PHASE     0x17
#define WG1_DC	      0x18
#define WG1_SHAPE     0x19
#define WG1_DUR	      0x1a
#define WG1_RF	      0x1b
#define WG1_TRIG      0x1c
#define WG1_SEED      0x1d
#define WG1_DELTA     0x1e
#define WG1_WAVE      0x1f
#define WG1_STATUS    0x20
#define WG1_TON	      0x21


void WG1on( qint din);
void WG1off( qint din);
void WG1clear( qint din);
void WG1amp( qint din, float amp);
void WG1freq(qint din, float freq);
void WG1swrt(qint din, float swrt);
void WG1phase(qint din, float phase);
void WG1dc(qint din, float dc);
void WG1shape(qint din, qint scon);
  #define GAUSS	  1
  #define UNIFORM 2
  #define SINE	  3
  #define WAVE	  4
void WG1dur(qint din, float dur);
void WG1rf(qint din, float rf);
void WG1trig(qint din, qint tcode);
  /* See CG1 Constants for triggering */
void WG1seed(qint din, long seed);
void WG1delta(qint din, long delta);
void WG1wave(qint din, qint wave[], qint npts);
qint WG1status(qint din);
  #define OFF	  0
  #define ON	  1
  #define RISING  2
  #define FALLING 3
void WG1ton( qint din);


/**********************************************************
  WG2 Drivers
 **********************************************************/
#define WG2_ON	      0x11
#define WG2_OFF	      0x12
#define WG2_CLEAR     0x13
#define WG2_AMP	      0x14
#define WG2_FREQ      0x15
#define WG2_SWRT      0x16
#define WG2_PHASE     0x17
#define WG2_DC	      0x18
#define WG2_SHAPE     0x19
#define WG2_DUR	      0x1a
#define WG2_RF	      0x1b
#define WG2_TRIG      0x1c
#define WG2_SEED      0x1d
#define WG2_DELTA     0x1e
#define WG2_WAVE      0x1f
#define WG2_STATUS    0x20
#define WG2_TON	      0x21


void WG2on( qint din);
void WG2off( qint din);
void WG2clear( qint din);
void WG2amp( qint din, float amp);
void WG2freq(qint din, float freq);
void WG2swrt(qint din, float swrt);
void WG2phase(qint din, float phase);
void WG2dc(qint din, float dc);
void WG2shape(qint din, qint scon);
  #define GAUSS	  1
  #define UNIFORM 2
  #define SINE	  3
  #define WAVE	  4
void WG2dur(qint din, float dur);
void WG2rf(qint din, float rf);
void WG2trig(qint din, qint tcode);
  /* See CG1 Constants for triggering */
void WG2seed(qint din, long seed);
void WG2delta(qint din, long delta);
void WG2wave(qint din, qint wave[], qint npts);
qint WG2status(qint din);
  #define OFF	  0
  #define ON	  1
  #define RISING  2
  #define FALLING 3
void WG2ton( qint din);




/**********************************************************
  DD1 Drivers
 **********************************************************/
#define DD1_GO	      0x11
#define DD1_STOP      0x12
#define DD1_ARM	      0x13
#define DD1_MODE      0x14
#define DD1_SRATE     0x15
#define DD1_CLKIN     0x16
#define DD1_CLKOUT    0x17
#define DD1_NPTS      0x18
#define DD1_MTRIG     0x19
#define DD1_STRIG     0x1a
#define DD1_STATUS    0x1b
#define DD1_REPS      0x1c
#define DD1_CLIP      0x1d
#define DD1_CLEAR     0x1e
#define DD1_CLIPON    0x1f
#define DD1_ECHO      0x20
#define DD1_TGO	      0x21
#define DD1_SPERIOD	  0x22

void DD1clear( qint din);
void DD1go( qint din);
void DD1stop( qint din);
void DD1arm( qint din);
void DD1mode( qint din, qint mcode);
  #define ALL	    0x0f
  #define DUALDAC   0x03
  #define DAC1	    0x01
  #define DAC2	    0x02
	#define DUALADC	  0x0c
	#define ADC1	  0x04
	#define ADC2	  0x08
	#define FASTDAC	  0x10
void DD1srate( qint din, float srate);
float DD1speriod( qint din, float sper);
void DD1clkin( qint din, qint scode);
	#define INTERNAL  1
	#define EXTERNAL  2
	#define XCLK1	  3
	#define XCLK2	  4
void DD1clkout( qint din, qint dcode);
/* See above for XCLK1 and XCLK2 and NONE */
void DD1npts( qint din, long npts);
void DD1mtrig( qint din);
void DD1strig( qint din);
qint DD1status( qint din);
void DD1reps( qint din, unsigned qint nreps);
qint DD1clip( qint din);
void DD1clipon(qint din);
void DD1echo(qint din);
void DD1tgo(qint din);


/**********************************************************
	DA1 Drivers
 **********************************************************/
#define DA1_GO	      0x11
#define DA1_STOP      0x12
#define DA1_ARM	      0x13
#define DA1_MODE      0x14
#define DA1_SRATE     0x15
#define DA1_CLKIN     0x16
#define DA1_CLKOUT    0x17
#define DA1_NPTS      0x18
#define DA1_MTRIG     0x19
#define DA1_STRIG     0x1a
#define DA1_STATUS    0x1b
#define DA1_REPS      0x1c
#define DA1_CLIP      0x1d
#define DA1_CLEAR     0x1e
#define DA1_CLIPON    0x1f
#define DA1_TGO	      0x21
#define DA1_SPERIOD	  0x22

void DA1clear( qint din);
void DA1go( qint din);
void DA1stop( qint din);
void DA1arm( qint din);
void DA1mode( qint din, qint mcode);
void DA1srate( qint din, float srate);
float DA1speriod( qint din, float sper);
void DA1clkin( qint din, qint scode);
void DA1clkout( qint din, qint dcode);
void DA1npts( qint din, long npts);
void DA1mtrig( qint din);
void DA1strig( qint din);
qint DA1status( qint din);
void DA1reps( qint din, unsigned qint nreps);
qint DA1clip( qint din);
void DA1clipon(qint din);
void DA1tgo(qint din);


/**********************************************************
  AD1 Drivers
 **********************************************************/
#define AD1_GO	      0x11
#define AD1_STOP      0x12
#define AD1_ARM	      0x13
#define AD1_MODE      0x14
#define AD1_SRATE     0x15
#define AD1_CLKIN     0x16
#define AD1_CLKOUT    0x17
#define AD1_NPTS      0x18
#define AD1_MTRIG     0x19
#define AD1_STRIG     0x1a
#define AD1_STATUS    0x1b
#define AD1_REPS      0x1c
#define AD1_CLIP      0x1d
#define AD1_CLEAR     0x1e
#define AD1_CLIPON    0x1f
#define AD1_TGO	      0x21
#define AD1_SPERIOD	  0x22

void AD1clear( qint din);
void AD1go( qint din);
void AD1stop( qint din);
void AD1arm( qint din);
void AD1mode( qint din, qint mcode);
void AD1srate( qint din, float srate);
float AD1speriod( qint din, float sper);
void AD1clkin( qint din, qint scode);
void AD1clkout( qint din, qint dcode);
void AD1npts( qint din, long npts);
void AD1mtrig( qint din);
void AD1strig( qint din);
qint AD1status( qint din);
void AD1reps( qint din, unsigned qint nreps);
qint AD1clip( qint din);
void AD1clipon(qint din);
void AD1tgo(qint din);


/**********************************************************
	AD2 Drivers
 **********************************************************/
#define AD2_GO	      0x11
#define AD2_STOP      0x12
#define AD2_ARM	      0x13
#define AD2_MODE      0x14
#define AD2_SRATE     0x15
#define AD2_CLKIN     0x16
#define AD2_CLKOUT    0x17
#define AD2_NPTS      0x18
#define AD2_MTRIG     0x19
#define AD2_STRIG     0x1a
#define AD2_STATUS    0x1b
#define AD2_REPS      0x1c
#define AD2_CLIP      0x1d
#define AD2_CLEAR     0x1e
#define AD2_GAIN      0x1f
#define AD2_SH	      0x20
#define AD2_SAMPSEP   0x21
#define AD2_XCHANS    0x22
#define AD2_TGO	      0x23
#define AD2_SPERIOD	  0x24

void AD2clear( qint din);
void AD2go( qint din);
void AD2stop( qint din);
void AD2arm( qint din);
void AD2mode( qint din, qint mcode);
	#define XMUX	  0x01
	/* See above for ADC1 and ADC2 */
	#define ADC3	    0x10
	#define ADC4	  0x20
void AD2srate( qint din, float srate);
float AD2speriod( qint din, float sper);
void AD2clkin( qint din, qint scode);
void AD2clkout( qint din, qint dcode);
void AD2npts( qint din, long npts);
void AD2mtrig( qint din);
void AD2strig( qint din);
qint AD2status( qint din);
void AD2reps( qint din, unsigned qint nreps);
qint AD2clip( qint din);
void AD2gain( qint din, qint chan, qint gain);
	#define	  x1  0
	#define	  x2  1
	#define	  x4  2
	#define	  x8  3
	#define	 x16  4
	#define	 x32  5
	#define	 x64  6
	#define x128  7
void AD2sh(qint din, qint oocode);
	/* See ON/OFF codes above */
void AD2sampsep(qint din, float sep);
void AD2xchans(qint din, qint nchans);
void AD2tgo(qint din);


/**********************************************************
  DA3 Drivers
 **********************************************************/
#define DA3_GO	      0x11
#define DA3_STOP      0x12
#define DA3_ARM	      0x13
#define DA3_MODE      0x14
#define DA3_SRATE     0x15
#define DA3_CLKIN     0x16
#define DA3_CLKOUT    0x17
#define DA3_NPTS      0x18
#define DA3_MTRIG     0x19
#define DA3_STRIG     0x1a
#define DA3_STATUS    0x1b
#define DA3_REPS      0x1c
#define DA3_CLIP      0x1d
#define DA3_CLEAR     0x1e
#define DA3_CLIPON    0x1f
#define DA3_TGO	      0x21
#define DA3_SETSLEW   0x22
#define DA3_ZERO      0x23
#define DA3_SPERIOD	  0x24

void DA3clear( qint din);
void DA3go( qint din);
void DA3stop( qint din);
void DA3arm( qint din);
void DA3mode(qint din, qint cmask);
  #define DAC3	    0x04
  #define DAC4	    0x08
  #define DAC5	    0x10
  #define DAC6	    0x20
  #define DAC7	    0x40
  #define DAC8	    0x80
  #define FASTDAC3  0x0
void DA3srate( qint din, float srate);
float DA3speriod( qint din, float sper);
void DA3clkin( qint din, qint scode);
void DA3clkout( qint din, qint dcode);
void DA3npts( qint din, long npts);
void DA3mtrig( qint din);
void DA3strig( qint din);
qint DA3status( qint din);
void DA3reps( qint din, unsigned qint nreps);
qint DA3clip( qint din);
void DA3clipon(qint din);
void DA3tgo(qint din);
void DA3setslew(qint din, qint slcode);
	#define AUTOSLEW  0x00
void DA3zero(qint din);



/**********************************************************
  PF1 Drivers
 **********************************************************/
#define PF1_TYPE    0x11
#define PF1_BYPASS  0x12
#define PF1_NOPASS  0x13
#define PF1_BEGIN   0x14
#define PF1_B16	    0x15
#define PF1_B32	    0x16
#define PF1_A16	    0x17
#define PF1_A32	    0x18
#define PF1_FREQ    0x19
#define PF1_GAIN    0x1a

void PF1type(qint din, qint type, qint ntaps);
  #define FIR16	    1
  #define FIR32	    2
  #define IIR32	    3
  #define BIQ16	    4
  #define BIQ32	    5
void PF1begin(qint din);
void PF1bypass(qint din);
void PF1nopass(qint din);
void PF1b16(qint din, qint bcoe);
void PF1a16(qint din, qint acoe);
void PF1b32(qint din, long bcoe);
void PF1a32(qint din, long acoe);
void PF1freq(qint din, qint lpfreq, qint hpfreq);
void PF1gain(qint din, qint lpgain, qint hpgain);
  #define  _0DB	     1
  #define  _6DB	     2
  #define _12DB	     3
  #define _18DB	     4
  #define _24DB	     5

void PF1fir16( qint din, float bcoes[], qint ntaps);
void PF1fir32( qint din, float bcoes[], qint ntaps);
void PF1iir32( qint din, float bcoes[], float acoes[], qint ntaps);
void PF1biq16( qint din, float bcoes[], float acoes[], qint nbiqs);
void PF1biq32( qint din, float bcoes[], float acoes[], qint nbiqs);


/**********************************************************
  TG6 Drivers
 **********************************************************/
#define TG6_CLEAR     0x11
#define TG6_ARM	      0x12
#define TG6_GO	      0x13
#define TG6_TGO	      0x14
#define TG6_STOP      0x15
#define TG6_BASERATE  0x16
#define TG6_NEW	      0x17
#define TG6_HIGH      0x18
#define TG6_LOW	      0x19
#define TG6_VALUE     0x1a
#define TG6_DUP	      0x1b
#define TG6_REPS      0x1c
#define TG6_STATUS    0x1d

void TG6clear( qint din);
void TG6arm( qint din, qint snum);
void TG6go( qint din);
void TG6tgo( qint din);
void TG6stop( qint din);
void TG6baserate( qint din, qint brcode);
	#define _100ns 0
	#define _1us   1
	#define _10us  2
	#define _100us 3
	#define _1ms   4
	#define _EXT   7
void TG6new( qint din, qint snum, qint lgth, qint dmask);
void TG6high( qint din, qint snum, qint _beg, qint _end, qint hmask);
void TG6low( qint din, qint snum, qint _beg, qint _end, qint lmask);
void TG6value( qint din, qint snum, qint _beg, qint _end, qint val);
void TG6dup( qint din, qint snum, qint s_beg, qint s_end, qint d_beg, qint ndups, qint dmask);
void TG6reps( qint din, qint rmode, qint rcount);
	#define CONTIN_REPS  0
	#define TRIGGED_REPS 1
qint TG6status(qint din);
	#define LAST 3


/**********************************************************
	Auto-Detecting D/A Drivers
 **********************************************************/
void DAclear( qint din);
void DAgo( qint din);
void DAtgo( qint din);
void DAstop( qint din);
void DAarm( qint din);
void DAmode( qint din, qint mcode);
void DAsrate( qint din, float srate);
float DAsperiod( qint din, float sper);
void DAclkin( qint din, qint scode);
void DAclkout( qint din, qint dcode);
void DAnpts( qint din, long npts);
void DAmtrig( qint din);
void DAstrig( qint din);
qint DAstatus(qint din);
void DAreps( qint din, unsigned qint nreps);


/********************************************************************
	Auto-Detecting A/D Calls
 ********************************************************************/
void ADclear( qint din);
void ADgo( qint din);
void ADtgo( qint din);
void ADstop( qint din);
void ADarm( qint din);
void ADmode( qint din, qint mcode);
void ADsrate( qint din, float srate);
float ADsperiod( qint din, float sper);
void ADclkin( qint din, qint scode);
void ADclkout( qint din, qint dcode);
void ADnpts( qint din, long npts);
void ADmtrig( qint din);
void ADstrig( qint din);
qint ADstatus(qint din);
void ADreps( qint din, unsigned qint nreps);


/**********************************************************
	PD1 Drivers
 **********************************************************/
#define PD1_GO	      0x11
#define PD1_STOP      0x12
#define PD1_ARM	      0x13
#define PD1_NSTRMS    0x14
#define PD1_SRATE     0x15
#define PD1_CLKIN     0x16
#define PD1_CLKOUT    0x17
#define PD1_NPTS      0x18
#define PD1_MTRIG     0x19
#define PD1_STRIG     0x1a
#define PD1_STATUS    0x1b
#define PD1_REPS      0x1c
#define PD1_CLEAR     0x1e
#define PD1_TGO	      0x1f
#define PD1_ZERO      0x20
#define PD1_XCMD      0x21
#define PD1_XDATA     0x22
#define PD1_XBOOT     0x23
#define PD1_CHECKDSPS 0x24
#define PD1_WHAT      0x25
#define PD1_SPERIOD	  0x26

void PD1clear( qint din);
void PD1go( qint din);
void PD1stop( qint din);
void PD1arm( qint din);
void PD1nstrms( qint din, qint nDAC, qint nADC);
void PD1srate( qint din, float srate);
float PD1speriod( qint din, float sper);
void PD1clkin( qint din, qint scode);
void PD1clkout( qint din, qint dcode);
void PD1npts( qint din, long npts);
void PD1mtrig( qint din);
void PD1strig( qint din);
qint PD1status( qint din);
void PD1reps( qint din, unsigned qint nreps);
void PD1tgo(qint din);
void PD1zero(qint din);
void PD1xcmd(qint din, unsigned qint v[], qint n, char *caller);
void PD1xdata(qint din, qint data_id);
void PD1xboot(qint din);
long PD1checkDSPS(qint din);
qint PD1what(qint din, qint dcode, qint dnum, char *caller);
void PD1mode( qint din, qint mode);


/**********************************************************
	PD1 Supplimental Calls etc
 **********************************************************/
#define	 RTE_INTRST    0x0101
#define	 RTE_NSTRMS    0x0102
#define	 RTE_CLRDEL    0x0103
#define	 RTE_SETDEL    0x0104
#define	 RTE_CLRSCHED  0x0106
#define	 RTE_ADDSIMP   0x0107
#define	 RTE_ADDMULT   0x0108
#define	 RTE_SPECIB    0x0109
#define	 RTE_FLUSH     0x010a
#define	 RTE_RESETDSP  0x010b
#define	 RTE_BYPASSDSP 0x010c
#define	 RTE_CHECKDSPS 0x010d
#define	 RTE_LOADDEL   0x010e
#define	 RTE_FLUSHDEL  0x010f
#define	 RTE_SYNCALL   0x0110
#define	 RTE_INTDEL    0x0111
#define	 RTE_SPECOB    0x0112
#define	 RTE_WHATDEL   0x0113
#define	 RTE_WHATIO    0x0114
#define	 RTE_WHATDSP   0x0115
#define	 RTE_IDLEDSP   0x0116
#define	 RTE_CLRIO     0x0117
#define	 RTE_SETIO     0x0118
#define	 RTE_LOCKDSP   0x0119
#define	 RTE_INTERPDSP 0x011a
#define	 RTE_BOOTDSP   0x011b
#define	 RTE_WRITEDSP  0x011c

#define	 RTE_LOADCOES  0x0180
#define	 RTE_ERROR     0x9999

void PD1resetRTE( qint din);
void PD1nstrmsRTE(qint din, qint nIC, qint nOG);
void PD1flushRTE(qint din);

void PD1clrIO(qint din);
void PD1setIO(qint din, float dt1, float dt2, float at1, float at2);

void PD1clrDEL(qint din, qint ch1, qint ch2, qint ch3, qint ch4);
void PD1setDEL(qint din, qint tap, qint dly);
void PD1latchDEL(qint din);
void PD1flushDEL(qint din);
void PD1interpDEL(qint din, qint ifact);

void PD1clrsched(qint din);
void PD1addsimp(qint din, qint src, qint des);
void PD1addmult(qint din, qint src[], float sf[], qint nsrcs, qint des);
void PD1specIB(qint din, qint IBnum, qint desaddr);
void PD1specOB(qint din, qint OBnum, qint srcaddr);

void PD1idleDSP(qint din, long dmask);
void PD1resetDSP(qint din, long dmask);
void PD1bypassDSP(qint din, long dmask);
void PD1lockDSP(qint din, long dmask);
void PD1interpDSP(qint din, qint ifact, long dmask);
void PD1bootDSP(qint din, long dmask, char *fname);

void PD1syncall(qint din);

qint PD1whatDEL(qint din);
qint PD1whatIO(qint din);
qint PD1whatDSP(qint din, qint dn);

#define	 MONO	      0x01
#define	 STEREO	      0x02
#define	 MONSTER      0x03
#define	 _START	      0x7fff
#define	 LSYNC	      0x7ffe
#define	 _STOP	      0x7ffd
#define	 GSYNC	      0x7ffc

#define	 DSPID_BASE   0x0
#define	 DSPID_IND    0x1

#define	 DSPINL_BASE  0x49E8
#define	 DSPINL_IND   0x200

#define	 DSPINR_BASE  0x49C8
#define	 DSPINR_IND   0x200

#define	 DSPOUTL_BASE 0x49C0
#define	 DSPOUTL_IND  0x200

#define	 DSPOUTR_BASE 0x49C4
#define	 DSPOUTR_IND  0x200

#define	 COEF_BASE    0x49F0
#define	 COEF_IND     0x200

#define	 DELOUT_BASE  0x0400
#define	 DELOUT_IND1  0x20
#define	 DELOUT_IND2  0x1

#define	 DELIN_BASE   0x0480
#define	 DELIN_IND    0x1

#define	 TAP_BASE     0x0500
#define	 TAP_IND1     0x20
#define	 TAP_IND2     0x1

#define	 DAC_BASE     0x0800
#define	 DAC_IND      0x1

#define	 ADC_BASE     0x0810
#define	 ADC_IND      0x1

#define	 SYNC_ALL     0x45F8

#define	 IB_BASE      0x0
#define	 IB_IND	      0x1

#define	 OB_BASE      0x0
#define	 OB_IND	      0x1

#define	 IREG_BASE    0x01e0
#define	 IREG_IND     0x1

extern qint DSPid[32];
extern qint DSPin[32];
extern qint DSPinL[32];
extern qint DSPinR[32];
extern qint DSPout[32];
extern qint DSPoutL[32];
extern qint DSPoutR[32];
extern qint COEF[32];
extern qint DELin[4];
extern qint DELout[32][4];
extern qint TAP[32][4];
extern qint DAC[4];
extern qint ADC[4];
extern qint IB[32];
extern qint OB[32];
extern qint IREG[32];



/**********************************************************
	SS1 Drivers
 **********************************************************/
#define SS1_CLEAR     0x11
#define SS1_GAINON    0x12
#define SS1_GAINOFF   0x13
#define SS1_MODE      0x14
		#define QUAD_2_1     0
		#define DUAL_4_1     1
		#define SING_8_1     2
#define SS1_SELECT    0x15

void SS1clear( qint din);
void SS1gainon( qint din);
void SS1gainoff( qint din);
void SS1mode( qint din, qint mcode);
void SS1select( qint din, qint chan, qint inpn);
		#define OUTA	     0
		#define OUTB	     1
		#define OUTC	     2
		#define OUTD	     3

		#define INP1	     1
		#define INP2	     2
		#define INP3	     3
		#define INP4	     4
		#define INP5	     5
		#define INP6	     6
		#define INP7	     7
		#define INP8	     8


/**********************************************************
	PM1 Drivers
 **********************************************************/
#define PM1_CLEAR     0x11
#define PM1_CONFIG    0X12
		#define PM1_STEREO    0
		#define PM1_MONO      1
#define PM1_SPKON     0X13
#define PM1_SPKOFF    0X14
#define PM1_MODE      0x15
		#define COMMON	      0
		#define EXCLUSIVE     1

void PM1clear( qint din);
void PM1config( qint din, qint config);
void PM1mode( qint din, qint cmode);
void PM1spkon( qint din, qint sn);
void PM1spkoff( qint din, qint sn);

#define SN1	     1
#define SN2	     2
#define SN3	     3
#define SN4	     4
#define SN5	     5
#define SN6	     6
#define SN7	     7
#define SN8	     8
#define SN9	     9
#define SN10	     10
#define SN11	     11
#define SN12	     12
#define SN13	     13
#define SN14	     14
#define SN15	     15
#define SN16	     16


/**********************************************************
	HTI Drivers
 **********************************************************/
#define HTI_CLEAR     0x11
#define HTI_GO	      0x12
#define HTI_STOP      0x13
#define HTI_READAER   0x14
#define HTI_READXYZ   0x15
#define HTI_WRITERAW  0x16
#define HTI_READRAW   0x17
#define HTI_SETRAW    0x18
#define HTI_BORESIGHT 0x19
#define HTI_SHOWPARAM 0x1a
#define HTI_RESET     0x1b
#define HTI_READONE   0x1c
#define HTI_FASTAER   0x1d
#define HTI_FASTXYZ   0x1e
#define HTI_GETECODE  0x17
#define HTI_ISISO     0x20

void HTIclear( qint din);
void HTIgo( qint din);
void HTIstop( qint din);
void HTIreadAER( qint din, float *az, float *el, float *roll);
void HTIreadXYZ( qint din, float *x, float *y, float *z);
void HTIwriteraw( qint din, char cmdstr[]);
void HTIsetraw( qint din, qint nbytes, char c1, char c2);
void HTIreadraw( qint din, qint maxchars, char *buf);
void HTIboresight( qint din);
void HTIreset( qint din);
void HTIshowparam( qint din, qint pid);
		 #define P_AZ	 1
		 #define P_EL	 2
		 #define P_ROLL	 3
		 #define P_X	 4
		 #define P_Y	 5
		 #define P_Z	 6
float HTIreadone( qint din, qint pid);
     /* Use const from HTIshowparam */
void HTIfastAER( qint din, qint *az, qint *el, qint *roll);
void HTIfastXYZ( qint din, qint *x, qint *y, qint *z);
int  HTIgetecode( qint din);
void HTIisISO( qint din);

