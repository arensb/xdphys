SUBDIRS = iserver tdt-driver

INSTALL_ROOT =	/tmp/barnowl3

MAKEVARS = INSTALL_ROOT="$(INSTALL_ROOT)"

all clean distclean depend install:
	for d in $(SUBDIRS); do \
		(cd $$d; $(MAKE) $(MAKEVARS) $@) \
	done
