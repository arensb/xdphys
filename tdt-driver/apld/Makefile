# -------------------------------------------------------------
# RCSID		: 	$Id: Makefile,v 1.4 2000/07/12 22:55:04 cmalek Exp $
# Project	:	Tucker Davis device driver for Linux
# Filename	:	Makefile 
# Author	:	cmalek
# Desc		:	Makefile for the AP2 EPROM loader
# -------------------------------------------------------------
# -------------------------------------------------------------
# Define some useful variables
# -------------------------------------------------------------
# various system tools
CC          = gcc 
LD	     	= gcc 
#CDEBUG      = -g 
ifdef CDEBUG
LDEBUG      = -g 
endif
ifndef CDEBUG 
OPTIMIZE	= -O2
endif
RM	     	= /bin/rm -f
INSTALL		= install
LIBTOOL		= /usr/bin/libtool

# -------------------------------------------------------------
# Install directories
# -------------------------------------------------------------
INSTALL_ROOT = /usr/local
INSTALL_BINDIR	= $(INSTALL_ROOT)/bin
INSTALL_LIBDIR	= $(INSTALL_ROOT)/lib

# -------------------------------------------------------------
# these tell where various required libraries and include files
# can be found
# -------------------------------------------------------------

TDTINC	    = ../include
TDTLIB	    = ../lib

# ------------------------------------------------------------------
# Object and Source files
# ------------------------------------------------------------------
TARGETS		=	apld

# ------------------------------------------------------------
# Defines
# ------------------------------------------------------------

DEFINES		=	-DCOMP32 -DINTMOVE -DNCURSES -DOBJDIR="\"$(INSTALL_LIBDIR)\""

# ------------------------------------------------------------
# Includes
# ------------------------------------------------------------
INCLUDES	=	-I$(TDTINC)

# ------------------------------------------------------------
# Compiler flags
# ------------------------------------------------------------

CFLAGS  = 	-Wall -c -D_GNU_SOURCE $(CDEBUG) $(OPTIMIZE)

all: $(TARGETS)

apld: apld.o
	$(LD) -o apld apld.o 

install: $(TARGETS)
	@echo   Installing AP2 EPROM object file to $(INSTALL_LIBDIR) ...
	@$(INSTALL) -d -g lab -m 0755  $(INSTALL_BINDIR)
	@$(INSTALL) -d -g lab -m 0755  $(INSTALL_LIBDIR)
	@$(INSTALL) -g lab -m 0444  ap.obj $(INSTALL_LIBDIR)
	@echo   Installing AP2 EPROM loader to $(INSTALL_BINDIR) ...
	@for x in $(TARGETS); \
			do\
				$(INSTALL) -m 04555 $$x $(INSTALL_BINDIR);\
			done

clean: 
	@$(RM) *.o $(TARGETS) .lib

distclean: 
	@$(RM) *~ \#* core *.o $(TARGETS) .lib

# ------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------
apld.o: $(TDTINC)/dos.h $(TDTINC)/apos.h $(TDTINC)/xbdrv.h

# ----------------------------------------------------------------------
# Implicit rules
# ----------------------------------------------------------------------
.c.o:
		$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) $<

