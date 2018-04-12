# tdt-driver should be built before iserver: it has libraries.
SUBDIRS = tdt-driver iserver

INSTALL_ROOT =	/export/software/barnowl2

MAKEVARS = INSTALL_ROOT="$(INSTALL_ROOT)"

all clean distclean depend install:
	for d in $(SUBDIRS); do \
		(cd $$d; $(MAKE) $(MAKEVARS) $@) \
	done
