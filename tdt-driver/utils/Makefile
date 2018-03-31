# -------------------------------------------------------------
# RCSID		: 	$Id: Makefile,v 1.5 2000/07/12 22:55:06 cmalek Exp $
# Project	:	sslib library
# Filename	:	Makefile
# Author	:	cmalek
# Desc		:	Makefile for sslib library
# -------------------------------------------------------------

# -------------------------------------------------------------
# Define some useful variables
# -------------------------------------------------------------
# various system tools
CC          = gcc 
CPP	     	= gcc - -E -P
LD	     	= gcc 
OPTIMIZE	= -O2
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
TARGETS		=	aptst \
				xbcomini \
				xbsysid \
				xbidchck


# ------------------------------------------------------------
# Defines
# ------------------------------------------------------------
OSDEF	=	-DLINUX

DEFINES		=	$(OSDEF) -DCOMP32 -DINTMOVE -DNCURSES

# ------------------------------------------------------------
# Includes
# ------------------------------------------------------------
INCLUDES	=	-I$(TDTINC)

# ------------------------------------------------------------
# Compiler flags
# ------------------------------------------------------------
CFLAGS  = 	-Wall -c -D_GNU_SOURCE $(CDEBUG) $(OPTIMIZE) 


all: xbidchck xbcomini

aptst: aptst.o
	$(LD) -o aptst $(LDFLAGS) -L$(TDTLIB) aptst.o -ltdt -lncurses

xbidchck: xbidchck.o
	$(LD) -g -o xbidchck $(LDFLAGS) -L$(TDTLIB) xbidchck.o -ltdt -lncurses

xbcomini: xbcomini.o
	$(LD) -g -o xbcomini $(LDFLAGS) -L$(TDTLIB) xbcomini.o  -ltdt

xbsysid: xbsysid.o
	$(LD) -o xbsysid $(LDFLAGS) -L$(TDTLIB) xbsysid.o -ltdt -lncurses

install: $(TARGETS)
	@echo   Installing tdt util files to $(INSTALL_BINDIR)
	$(INSTALL) -d -m 0755  $(INSTALL_BINDIR)
	for x in $(TARGETS); \
			do\
				$(INSTALL) -m 04555 $$x $(INSTALL_BINDIR);\
				echo "           $$x.";\
			done

clean: 
	@$(RM) -rf *.o core $(TARGETS) ./.libs

distclean: 
	@$(RM) -rf *~ \#* core *.o $(TARGETS) ./.libs


# ------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------
aptst.o: $(TDTINC)/dos.h $(TDTINC)/apos.h 
xbcomini.o: $(TDTINC)/dos.h $(TDTINC)/xbdrv.h 
xbsysid.o: $(TDTINC)/dos.h $(TDTINC)/xbdrv.h 
xbidchck.o: $(TDTINC)/dos.h $(TDTINC)/xbdrv.h 

.c.o:
		$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) $<

