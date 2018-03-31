# ---------------------------------------------------------------------
# RCSID		: 	$Id: Makefile,v 1.2 1999/06/04 23:58:00 cmalek Exp $
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
INSTALL_MANDIR = $(INSTALL_ROOT)/man

MANFILES	=	apld.1 \
				xbcomini.1 \
				xbsysid.1 \
				xbidchck.1

all:

clean:
	@$(RM) *~ \#* core

distclean: clean

install: 
	@$(INSTALL) -d -g lab -m 0755 $(INSTALL_MANDIR)/man1
	@for x in $(MANFILES);\
		do\
			$(INSTALL) -g lab -m 0444 $$x $(INSTALL_MANDIR)/man1;\
			echo "           $$x.";\
		done
