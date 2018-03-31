%define Version 1.2.12
%define Release 1

summary		: The iserver package provides an interface to TDT equipment
name		: iserver-tdt
version		: %{Version}
release		: %{Release}
copyright	: GPL
group		: Applications/Scientific
source		: ftp.etho.caltech.edu:/pub/xdphys/SOURCES/iserver-%{Version}.tar.gz
buildroot	: /tmp/iserver-%{Version}
%description
The iserver package provides interfaces to DAQ equipment from 
Tucker Davis technologies.  A C library for linking with C
programs and a MEX file for use with MATLAB are both provided.
Both the C library and MEX file control tdtproc, a process
which interacts with the DAQ equipment.

%prep
%setup -n iserver

%build
make 
(cd matlab; make)

%install
DESTDIR=$RPM_BUILD_ROOT; export DESTDIR
[ -n "`echo $DESTDIR | sed -n 's:^/tmp/[^.].*$:OK:p'`" ] && rm -rf $DESTDIR ||
(echo "Invalid BuildRoot: '$DESTDIR'! Check this .spec ..."; exit 1) || exit 1

for dir in usr/local/bin usr/local/lib usr/local/include usr/local/local-matlab ; do
	install -m 755 -d $DESTDIR/$dir
done

INSTALL_ROOT=$DESTDIR/usr/local; export INSTALL_ROOT
make -e install
(cd matlab; make -e install)

%post
/sbin/ldconfig /usr/local/lib

%files

/usr/local/bin/tdtproc
/usr/local/bin/iserver_test
/usr/local/lib/libiserver.so.1.0.0
/usr/local/lib/libiserver.a
/usr/local/include/iserver/iserver.h
/usr/local/include/iserver/daq.h
/usr/local/include/iserver/att.h
/usr/local/local-matlab/iserver.mexglx
/usr/local/local-matlab/is_init.m
/usr/local/local-matlab/is_play.m
/usr/local/local-matlab/is_load.m
/usr/local/local-matlab/is_dump.m
/usr/local/local-matlab/is_record.m
/usr/local/local-matlab/is_shutdown.m
/usr/local/local-matlab/adtomv.m
/usr/local/local-matlab/get_att.m
/usr/local/local-matlab/set_att.m
