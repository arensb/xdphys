UNAME = $(shell uname)
include ../template.$(UNAME)

# ---------------------------------------------------------------------
# RCSID		: 	$Id: Makefile,v 1.5 1999/05/12 18:45:00 cmalek Exp $
# Project	:	dowl
# Filename	:	Makefile
# Desc		:	Makefile for tdtproc, data acquisition prog for tucker
#				davis equipment
# ---------------------------------------------------------------------

# define -DSAUDIO here if you want an audible click from the computer speaker
# at the start of stimulus

CFLAGS = $(COPTS) $(OS) $(DAQINC) -I..  -DSAUDIO


# ---------------------------------------------------------------------

OBJS =	drpmproc.o \
		saudio.o
	
DRPMPROC = drpmproc

TARGETS = $(DRPMPROC)

all: $(DRPMPROC)


$(DRPMPROC): $(OBJS)
	$(LD) $(LDFLAGS) -static -o $@ $(OBJS) \
		-L$(DRPLIB) -ldrp -lqckMon -L/usr/demo/SOUND -laudio -lm

reset_audio: reset_audio.o
	$(LD) $(LDFLAGS) -o $@ $@.o -L/usr/demo/SOUND -laudio -lm

clean:
	$(RM) *.o *.a core

binclean:
	$(RM) $(DRPMPROC)

install: $(DRPMPROC)
	@$(INSTALL) -d -m 0755 $(INSTALL_BINDIR)
	@echo -n "Installing drpmproc ... "
	@$(INSTALL) -m 04555 $(DRPMPROC) $(INSTALL_BINDIR)
	@echo "Done."

distclean: clean binclean
	$(RM) *~ \#* TAGS

