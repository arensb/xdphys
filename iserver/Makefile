UNAME:=	$(shell uname)
include ./template.$(UNAME)

# ---------------------------------------------------------------------
# RCSID     :   $Id: Makefile,v 1.21 2001/03/27 07:02:50 cmalek Exp $
# Project   :   iserver
# Filename  :   Makefile
# Desc      :   Makefile for iserver library
# ---------------------------------------------------------------------

DIR:=	$(shell basename `pwd`)

VERSION = 1.2.12
RELEASE = 1

# Increment INTERFACE if the library interface has 
# changed since last update, and then set REVISION to 0
INTERFACE = 1
# Increment REVISION if the library code has 
# changed since last update, but the interface has not
REVISION = 0
# Increment AGE if interfaces have been added since last public release
# Set AGE to 0 if interfaces have been removed since last public release
AGE = 0

LIBNAME = libiserver.la

DEFINES = $(WORDS) $(OS) $(EXTRA) $(TEST)

#-------------------------------------------------------------------------

OBJS = \
	att.lo \
	iserver.lo 
#	is-bind.lo

INCFILES = 	\
	iserver.h \
	daq.h \
	att.h

INCLUDES   = $(DAQINC)

ARCHOBJS = $(addprefix $(UNAME)/,$(OBJS))
ARCHLIB = $(addprefix $(UNAME)/,$(LIBNAME))
RPMFILE = $(DIR)-tdt-$(VERSION)-$(RELEASE).i386.rpm
SRPMFILE = $(DIR)-tdt-$(VERSION)-$(RELEASE).src.rpm

TARGETS = libiserver.la $(UNAME)/$(DAQ_PROG)
#-------------------------------------------------------------------------
# Targets
#-------------------------------------------------------------------------
all: do-it-all

ifeq (.depend.$(UNAME),$(wildcard .depend.$(UNAME)))
include .depend.$(UNAME)
do-it-all:	$(LIBNAME) $(UNAME)/$(DAQ_PROG)
else
do-it-all:	depend $(TARGETS)
endif

$(LIBNAME): $(ARCHLIB)
.PHONY: $(LIBNAME)

$(ARCHLIB):  $(ARCHOBJS)
	$(LIBTOOL) $(CC) $(DEFINES) $(CFLAGS) -o $(ARCHLIB) \
		-rpath $(INSTALL_LIBDIR) \
		-version-info $(INTERFACE):$(REVISION):$(AGE) \
		$(ARCHOBJS)

$(UNAME)/$(DAQ_PROG): 
	(cd $(DAQ_PROG); $(MAKE) -e $(DAQ_PROG))

depend: 
	$(RM) -f .depend.$(UNAME)
	$(AWK) -v UNAME=$(UNAME) -f depend.awk *.[ch]  > .depend.$(UNAME)
	chmod g+w .depend.$(UNAME)

install:
	@mkdirhier $(INSTALL_INCDIR)
	@for x in $(INCFILES);\
		do\
			$(INSTALL) $(STRIP) -m 0444 $$x $(INSTALL_INCDIR);\
			echo "  $$x.";\
		done
	@mkdirhier $(INSTALL_LIBDIR)
	$(LIBTOOL) $(INSTALL) -m 0444 $(ARCHLIB) $(INSTALL_LIBDIR)
	(cd $(DAQ_PROG); $(MAKE) -e install)

clean: 
	-$(RM) -f $(UNAME)/* core $(UNAME)/.libs/*
	(cd $(DAQ_PROG); $(MAKE) -e clean)

distclean: clean 
	$(RM) *~ \#* 
	$(RM) .depend.$(UNAME)
	$(RM) temp.*
	$(RM) tags
	$(RM) $(UNAME)/* core $(UNAME)/.libs/*
	(cd tdtproc; $(MAKE) -e distclean)
	(cd drpmproc; $(MAKE) -e distclean)


tags:
	ctags -idefgstuvx *.[ch]

dist: 
	(cd ..; tar zvc  -X $(DIR)/.exclude -f $(DIR)-$(VERSION).tar.gz $(DIR); mv $(DIR)-$(VERSION).tar.gz $(DIR))

rpm:
	cp $(DIR)-$(VERSION).tar.gz /usr/src/redhat/SOURCES
	rpm -ba --clean iserver-tdt.spec
	mv /usr/src/redhat/SRPMS/$(SRPMFILE) .
	mv /usr/src/redhat/RPMS/i386/$(RPMFILE) .

update6:
	mv $(SRPMFILE) $(SRPMDIR)
	mv $(RPMFILE) $(LIBC6RPMDIR)

update5:
	mv $(SRPMFILE) $(SRPMDIR)
	mv $(RPMFILE) $(LIBC5RPMDIR)

ftp5:
	cp $(DIR)-$(VERSION).tar.gz $(FTPDIR)
	cp $(SRPMDIR)/$(SRPMFILE) $(FTPSRPMDIR)
	cp $(LIBC5RPMDIR)/$(RPMFILE) $(FTPLIBC5RPMDIR)

ftp6:
	cp $(DIR)-$(VERSION).tar.gz $(FTPDIR)
	cp $(SRPMDIR)/$(SRPMFILE) $(FTPSRPMDIR)
	cp $(LIBC6RPMDIR)/$(RPMFILE) $(FTPLIBC6RPMDIR)

iserver.lo: is-tdt.c is-drpm.c iserver.c
	$(LIBTOOL) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

	
# ----------------------------------------------------------------------
# Implicit rules
# ----------------------------------------------------------------------
$(UNAME)/%.lo: %.c
	$(LIBTOOL) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@


