# -------------------------------------------------------------
# RCSID		: 	$Id: Makefile,v 1.5 2000/07/12 22:55:04 cmalek Exp $
# Project	:	Tucker Davis device driver for Linux
# Filename	:	Makefile
# Author	:	cmalek
# Desc		:	Makefile for the examples	
# -------------------------------------------------------------

# -------------------------------------------------------------
# Define some useful variables
# -------------------------------------------------------------
# various system tools
CC          = gcc 
CPP	     	= gcc - -E -P
LD	     	= gcc 
CDEBUG      = -g 
ifdef CDEBUG
LDEBUG      = -g 
endif
ifndef CDEBUG 
OPTIMIZE	= -O2
endif
RM	     	= /bin/rm -f
INSTALL		= install

# -------------------------------------------------------------
# Directories used by install
# -------------------------------------------------------------
INSTALL_ROOT = /usr/local
INSTALL_BINDIR	= $(INSTALL_ROOT)/bin

# -------------------------------------------------------------
# these tell where various required libraries and include files
# can be found
# -------------------------------------------------------------

TDTINC	    = ../include
TDTLIB	    = ../lib

# ------------------------------------------------------------------
# Object and Source files
# ------------------------------------------------------------------
TARGETS		=	tonetest \
				dualtone \
				rectest \
				dualrec \
				dualplayrec \
				tg6test \
				et1test \
				sd1test \
				tg6att \
				playrec \
				setpriority

# ------------------------------------------------------------
# Defines
# ------------------------------------------------------------

DEFINES		=	-DCOMP32 -DINTMOVE -DNCURSES

# ------------------------------------------------------------
# Includes
# ------------------------------------------------------------
INCLUDES	=	-I$(TDTINC)

# ------------------------------------------------------------
# Compiler flags
# ------------------------------------------------------------

CFLAGS  = 	-Wall -c -D_GNU_SOURCE $(CDEBUG) $(OPTIMIZE)


all: $(TARGETS)

tonetest: tonetest.o
	$(LD) -o tonetest $(LDFLAGS) -L$(TDTLIB) tonetest.o  -ltdt -lncurses

dualtone: dualtone.o
	$(LD) -o dualtone $(LDFLAGS) -L$(TDTLIB) dualtone.o -ltdt -lncurses

rectest: rectest.o
	$(LD) -o rectest $(LDFLAGS) -L$(TDTLIB) rectest.o -ltdt -lncurses

dualrec: dualrec.o
	$(LD) -o dualrec $(LDFLAGS) -L$(TDTLIB) dualrec.o -ltdt -lncurses

playrec: playrec.o
	$(LD) -o playrec $(LDFLAGS) -L$(TDTLIB) playrec.o -ltdt -lncurses

dualplayrec: dualplayrec.o
	$(LD) -o dualplayrec $(LDFLAGS) -L$(TDTLIB) dualplayrec.o -ltdt -lncurses

tg6test: tg6test.o
	$(LD) -o tg6test $(LDFLAGS) -L$(TDTLIB) tg6test.o -ltdt -lncurses

tg6att: tg6att.o
	$(LD) -o tg6att $(LDFLAGS) -L$(TDTLIB) tg6att.o -ltdt -lncurses

sd1test: sd1test.o
	$(LD) -o sd1test $(LDFLAGS) -L$(TDTLIB) sd1test.o -ltdt -lncurses

et1test: et1test.o
	$(LD) -o et1test $(LDFLAGS) -L$(TDTLIB) et1test.o  -ltdt -lncurses

# Real-time system calls test programs
rt-demo: rt-demo.o
	$(LD) -o rt-demo $(LDFLAGS) rt-demo.o 

setpriority: setpriority.o
	$(LD) -o setpriority $(LDFLAGS) setpriority.o 


install: $(TARGETS)
	@echo   Installing tdt example files to $(INSTALL_BINDIR)
	@$(INSTALL) -d -g lab -m 0755  $(INSTALL_BINDIR)
	@for x in $(TARGETS); \
			do\
				$(INSTALL) -g lab -m 04555 $$x $(INSTALL_BINDIR);\
				echo "           $$x.";\
			done

clean: 
	@$(RM) *.o core $(TARGETS) rt-demo setpriority 

distclean: 
	@$(RM) *~ \#* core *.o $(TARGETS) rt-demo setpriority

# ------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------
tonetest.o: $(TDTINC)/dos.h $(TDTINC)/apos.h $(TDTINC)/xbdrv.h
dualtone.o: $(TDTINC)/dos.h $(TDTINC)/apos.h $(TDTINC)/xbdrv.h
rectest.o:	$(TDTINC)/dos.h $(TDTINC)/apos.h $(TDTINC)/xbdrv.h \
			$(TDTINC)/tdtplot.h

# ----------------------------------------------------------------------
# Implicit rules
# ----------------------------------------------------------------------
.c.o:
		$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) $<

