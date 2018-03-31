UNAME:=	$(shell uname)
include ../template.$(UNAME)

# ---------------------------------------------------------------------
# RCSID		: 	$Id: Makefile,v 1.5 1999/06/04 23:50:43 cmalek Exp $
# Project	:	xdphys
# Filename	:	Makefile
# Desc		:	Makefile for iserver Matlab files
# ---------------------------------------------------------------------

		
DEBUG = -g

#-------------------------------------------------------------------------
# Includes
#-------------------------------------------------------------------------
ISINC = -I..

INCLUDES = $(ISINC)

#-------------------------------------------------------------------------
# Libraries
#-------------------------------------------------------------------------
ISLIB = ../$(UNAME)/.libs/libiserver.a

LIBS = $(ISLIB) -L$(NRLIB) -l$(NRVER)

#-------------------------------------------------------------------------
# Defines
#-------------------------------------------------------------------------
EXTRA =
DEFINES      = -DNR $(WORDS) $(OS) $(TEST) $(EXTRA) $(DEBUG)


MFILES = is_init.m \
	adtomv.m \
	is_play.m \
	is_load.m \
	is_dump.m \
	get_att.m \
	set_att.m \
	is_record.m \
	is_shutdown.m

#-------------------------------------------------------------------------
# Targets
#-------------------------------------------------------------------------
all: mex

mex:
	mex $(DEFINES) $(INCLUDES) -DMATLAB $(LIBS) is-matlab.c \
		-output iserver.mex$(MATEXT)

clean:
	$(RM) iserver.mex$(MATEXT)

install:
	$(INSTALL) -m 755 iserver.mex$(MATEXT) $(INSTALL_MATDIR)
	@for x in $(MFILES);\
		do\
			$(INSTALL) -m 0555 $$x $(INSTALL_MATDIR);\
		done 
