# ---------------------------------------------------------------------
# RCSID		: 	$Id: Makefile,v 1.4 2000/07/12 22:55:05 cmalek Exp $
# Project	:	Tucker Davis device driver for Linux
# Filename	:	Makefile
# Author	:	cmalek
# Desc		:	Makefile for the APOS, XBDRV and tdtplot library (libtdt.a)
# ---------------------------------------------------------------------

# -------------------------------------------------------------
# Define some useful variables
# -------------------------------------------------------------
# various system tools
CC          = gcc 
CPP	     	= gcc - -E -P
LD	     	= gcc 
# apos.c and xbdrv.c MUST be compiled with one of the -O flags:
# otherwise, the compiler can't find the ioperm and inb/outb, etc.. 
# macros when you link with the library 
#
# This, of course, means you can't use debugging with these files.

#CDEBUG      = -g 
ifdef CDEBUG
LDEBUG      = -g 
endif
ifndef CDEBUG 
OPTIMIZE	= -O3
endif
RM	     	= /bin/rm 
INSTALL		= install
RANLIB		= ranlib

LIBNAME = libtdt.a
# -------------------------------------------------------------
# Install directories
# -------------------------------------------------------------
INSTALL_ROOT = /usr/local
INSTALL_LIBDIR = $(INSTALL_ROOT)/lib

# -------------------------------------------------------------
# these tell where various required libraries and include files
# can be found
# -------------------------------------------------------------
TDTINC	    = ../include

# ------------------------------------------------------------------
# Object and Source files
# ------------------------------------------------------------------
OBJS		=	apos.o \
				xbdrv.o \
				tdtplot.o

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

#CFLAGS  = 	-Wall -c -m486 -D_GNU_SOURCE $(CDEBUG) $(OPTIMIZE)
CFLAGS  = 	-Wall -c -D_GNU_SOURCE $(CDEBUG) $(OPTIMIZE)

all: $(LIBNAME)

$(LIBNAME): $(OBJS)
	$(AR) ru libtdt.a $(OBJS)
	$(RANLIB) libtdt.a

install: $(LIBNAME)
	$(INSTALL) -m 0555 $(LIBNAME) $(INSTALL_LIBDIR)

clean:
	@$(RM) -rf *.o $(LIBNAME) .libs

distclean:
	@$(RM) -rf *~ \#* core *.o $(LIBNAME) .libs

# ------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------
apos.o: $(TDTINC)/apos.h
xbdrv.o: $(TDTINC)/xbdrv.h
tdtplot.o: $(TDTINC)/tdtplot.h

# ----------------------------------------------------------------------
# Implicit rules
# ----------------------------------------------------------------------
%.o: %.c
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@
