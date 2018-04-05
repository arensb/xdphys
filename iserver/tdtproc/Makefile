UNAME := $(shell uname)
include ../template.$(UNAME)

# ---------------------------------------------------------------------
# RCSID     :   $Id: Makefile,v 1.11 2001/03/27 07:02:51 cmalek Exp $
# Project   :   iserver
# Filename  :   Makefile
# Desc      :   Makefile for tdtproc
# ---------------------------------------------------------------------

OSFULL	:=	$(shell uname -a)
DATE :=	$(shell date)

# -------------------------------------------------------------------------
# Compiler flags
#-------------------------------------------------------------------------
CFLAGS  = 	-Wall -c -m64 -D_GNU_SOURCE -DCOMPILE_DATE="\"$(DATE)\"" \
				-DOSINFO="\"$(OSFULL)\"" $(CDEBUG) 
LDFLAGS = $(LDDEBUG) -m64

# -------------------------------------------------------------------------
# Includes
#-------------------------------------------------------------------------
INCLUDES = $(DAQINC) -I.. -I/usr/include/ncurses

# -------------------------------------------------------------------------
# Defines
#-------------------------------------------------------------------------
DEFINES = 	-DLINUX -DCOMP32 -DINTMOVE $(OS) $(RIG) $(EXTRA) $(WORDS)

# ------------------------------------------------------------------------

TDTPROC = tdtproc

TARGETS = $(TDTPROC) iserver_test
TEST_TARGETS = init_test tdt_client

all: $(TARGETS)

test: $(TEST_TARGETS)


$(TDTPROC): tdtproc.o tdtproc.h 
	$(LD) $(LDFLAGS) -o $@ tdtproc.o -L$(TDTLIB) -ltdt -lm

tdt_client: tdt_client.o 
	$(LD) $(LDFLAGS) -o $@ tdt_client.o -L$(TDTLIB) -ltdt -lncurses -lm

init_test: init_test.o is-tdt.o 
	$(LD) $(LDFLAGS) -o $@ init_test.o is-tdt.o -lncurses -lm

iserver_test: iserver_test.o 
	$(LIBTOOL) $(LD) $(LDFLAGS) -o $@ iserver_test.o -lncurses -lm -L$(TDTLIB) -ltdt ../Linux/.libs/libiserver.a

# ------------------------------------------------------------------------
# You have to do a "make perms" as root to run tdtproc from the bin directory:
# it sets the suid root permissions necessary to use the tdt equipment
# ------------------------------------------------------------------------

perms:
	chown root $(TDTPROC)
	chmod 04555 $(TDTPROC)

install: $(TDTPROC) iserver_test
	@$(INSTALL) -d -m 0755 $(INSTALL_BINDIR)
	@echo -n "Installing tdtproc ... "
	$(INSTALL) $(STRIP) -m 04555 $(TDTPROC) $(INSTALL_BINDIR)
	@echo -n "Installing iserver_test ... "
	$(INSTALL) $(STRIP) -m 0555 iserver_test $(INSTALL_BINDIR)
	@echo "Done."

clean: binclean
	$(RM) *.o *.a core 

distclean: clean binclean
	$(RM) *~ \#* TAGS

binclean:
	$(RM) $(TDTPROC) tdt_client init_test iserver_test

tdtproc.o: tdtproc.h ../iserverInt.h
iserver_test.o: ../iserver.h ../iserverInt.h ../att.h

.c.o:
		$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) $<

