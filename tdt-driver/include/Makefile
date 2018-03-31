# ---------------------------------------------------------------------
# RCSID		: 	$Id: Makefile,v 1.2 1999/06/04 23:57:59 cmalek Exp $
# Project	:	Tucker Davis device driver for Linux
# Filename	:	Makefile
# Author	:	cmalek
# Desc		:	Makefile for installation of the include files
# ---------------------------------------------------------------------

# -------------------------------------------------------------
# Define some useful variables
# -------------------------------------------------------------
# various system tools
RM	     	= /bin/rm -f
INSTALL		= install

# -------------------------------------------------------------
# Install directories
# -------------------------------------------------------------
INSTALL_ROOT = /usr/local
INSTALL_INCDIR = $(INSTALL_ROOT)/include/tdt

INCFILES	=	apos.h \
				conio.h \
				tdtplot.h \
				dos.h \
				xbdrv.h

all:

clean:
	@$(RM) *~ \#* core

distclean: clean

install: 
	@$(INSTALL) -d -g lab -m 0755 $(INSTALL_INCDIR)
	@for x in $(INCFILES);\
		do\
			$(INSTALL) -g lab -m 0444 $$x $(INSTALL_INCDIR);\
			echo "           $$x.";\
		done
