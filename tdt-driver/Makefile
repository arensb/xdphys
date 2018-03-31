# ---------------------------------------------------------------------
# RCSID		: 	$Id: Makefile,v 1.5 2000/07/12 22:55:04 cmalek Exp $
# Project	:	Tucker Davis device driver for Linux
# Filename	:	Makefile
# Author	:	cmalek
# Desc		:	Master makefile for Tucker Davis driver
# ---------------------------------------------------------------------

DIR := $(shell basename `pwd`)

VERSION = 1.0.2

# Increment INTERFACE if the library interface has 
# changed since last update, and then set REVISION to 0
INTERFACE = 1
# Increment REVISION if the library code has 
# changed since last update, but the interface has not
REVISION = 0
# Increment AGE if interfaces have been added since last public release
# Set AGE to 0 if interfaces have been removed since last public release
AGE = 0

# -------------------------------------------------------------
# Define some useful variables
# -------------------------------------------------------------
# various system tools
CC          = gcc 
CPP	     	= gcc - -E -P
LD	     	= gcc 
#CDEBUG      = -g 
ifdef CDEBUG
LDEBUG      = -g 
endif
ifndef CDEBUG 
OPTIMIZE	= -O3
endif
RM	     	= /bin/rm -f
AR	     	= ar
RANLIB      = ranlib
LINT		= lclint
INSTALL		= install
export LIBTOOL		= /usr/bin/libtool

RPMFILE = $(DIR)-$(VERSION)-1.i386.rpm
SRPMFILE = $(DIR)-$(VERSION)-1.src.rpm

# -------------------------------------------------------------
# Install directories
# -------------------------------------------------------------
INSTALL_ROOT = /usr/local
INSTALL_INCDIR = $(INSTALL_ROOT)/include/tdt
INSTALL_LIBDIR = $(INSTALL_ROOT)/lib
INSTALL_BINDIR = $(INSTALL_ROOT)/bin
INSTALL_MANDIR = $(INSTALL_ROOT)/man
LIBC6RPMDIR	= /usr/local/Archives/dist/RPMS/libc6
LIBC5RPMDIR	= /usr/local/Archives/dist/RPMS/libc5
SRPMDIR	= /usr/local/Archives/dist/SRPMS

SUBDIRS	=		lib \
				apld \
				utils \
				examples \
				include \
				man

all:
	for dir in $(SUBDIRS) ; do \
		(cd $$dir; $(MAKE) -e $@) \
	done;

clean: 
	$(RM) *~ \#* core
	for dir in $(SUBDIRS) ; do \
		(cd $$dir; $(MAKE) -e $@) \
	done;

distclean: 
	for dir in $(SUBDIRS) ; do \
		(cd $$dir; $(MAKE) -e $@) \
	done;


install: 
	for dir in $(SUBDIRS) ; do \
		(cd $$dir; $(MAKE) -e $@) \
	done;

tags:
	ctags -idefgstuvx lib/*.[ch] include/*.[ch] utils/*.[ch] apld/*.[ch]


dist:
	(cd ..; tar -X $(DIR)/.exclude -czvf $(DIR)-$(VERSION).tar.gz $(DIR); mv $(DIR)-$(VERSION).tar.gz $(DIR))

rpm:
	cp $(DIR)-$(VERSION).tar.gz /usr/src/redhat/SOURCES
	rpm -ba --clean $(DIR).spec
	mv /usr/src/redhat/SRPMS/$(SRPMFILE) .
	mv /usr/src/redhat/RPMS/i386/$(RPMFILE) .

update6:
	mv $(SRPMFILE) $(SRPMDIR)
	mv $(RPMFILE) $(LIBC6RPMDIR)

update5:
	mv $(SRPMFILE) $(SRPMDIR)
	mv $(RPMFILE) $(LIBC5RPMDIR)
