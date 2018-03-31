;
; Tim - here's the assembly language which does the same thing as the
;       inline stuff you have in the qapos.c file directly.  I didn't try
;       and figure out why the turbo versions of the inport/outport
;       functions didn't work for you, I simply rewrote them here.
;
;       Incidentally, this code will only run on a 286 or better
;       processor due to the use of the REPE INSW/OUTSW instructions
;       used in DataIn/DataOut.
;
	.MODEL LARGE
        .286
        .CODE

RETSZ   EQU 4

PAGE

;======================================================================
;   FUNCTIONS TO OUTPUT TO AN I/O PORT
;======================================================================

;----------------------------------------------------------------------
; C Prototype:
;       void OutPortB(int ioport, unsigned char val);
;----------------------------------------------------------------------
        PUBLIC  _OutPortB
_OutPortB PROC

ioport  EQU     [bp+2+RETSZ]
val     EQU     [bp+4+RETSZ]

        push    bp
        mov     bp, sp

        mov     dx, ioport
        mov     al, val
        out     dx, al

        pop     bp
        ret

_OutPortB ENDP


;----------------------------------------------------------------------
; C Prototype:
;       void OutPort(int ioport, int val);
;----------------------------------------------------------------------
        PUBLIC  _OutPort
_OutPort PROC

        push    bp
        mov     bp, sp

        mov     dx,ioport
        mov     ax,val
        out     dx,ax

        pop     bp
        ret

_OutPort ENDP


PAGE

;======================================================================
;   FUNCTIONS TO INPUT FROM AN I/O PORT
;======================================================================

;----------------------------------------------------------------------
; C Prototype:
;       unsigned char InPortB(int ioport);
;----------------------------------------------------------------------
        PUBLIC  _InPortB
_InPortB PROC

        push    bp
        mov     bp, sp

        mov     dx, ioport
        in      al, dx
        sub     ah, ah

        pop     bp
        ret

_InPortB ENDP


;----------------------------------------------------------------------
; C Prototype:
;       int InPort(int ioport);
;----------------------------------------------------------------------
        PUBLIC  _InPort
_InPort  PROC

        push    bp
        mov     bp, sp

        mov     dx, ioport
        in      ax, dx

        pop     bp
        ret

_InPort  ENDP

PAGE

;******************************************************************
;    Assembly language that reads data in from QAP.
;
;void DataIn(unsigned int ioport, unsigned int qFP_SEG,
;                       unsigned int qFP_OFF, unsigned int count)
;----------------------------------------------------------------**/
        PUBLIC  _DataIn
_DataIn  PROC

qioport EQU     [bp+2+RETSZ]
qFP_SEG EQU     [bp+4+RETSZ]
qFP_OFF EQU     [bp+6+RETSZ]
qcount  EQU     [bp+8+RETSZ]

        push    bp
        mov     bp, sp
        push    di

        mov     cx, qcount
        mov     di, qFP_OFF
        mov     es, qFP_SEG
        mov     dx, qioport
        pushf
        repe    insw
;  db 0xF3,0x6D
        popf

        pop     di
        pop     bp
        ret

_DataIn  ENDP

PAGE

;*******************************************************************
;    Assembly language that sends data to QAP.
;
;void DataOut(unsigned int qFP_SEG, unsigned int qFP_OFF,
;                       unsigned int ioport, unsigned int count)
;----------------------------------------------------------------**/
        PUBLIC  _DataOut
_DataOut PROC

pFP_SEG EQU     [bp+2+RETSZ]
pFP_OFF EQU     [bp+4+RETSZ]
pioport EQU     [bp+6+RETSZ]
pcount  EQU     [bp+8+RETSZ]

        push    bp
        mov     bp, sp
        push    si

        mov     cx, pcount
        mov     dx, pioport
        mov     si, pFP_OFF
        mov     es, pFP_SEG
        push    ds
        push    es
        pop     ds
        pushf
        repe    outsw
;  db 0xF3,0x6F
        popf
        pop     ds

        pop     si
        pop     bp
        ret

_DataOut ENDP

        END
