%define Version 1.0.2
%define Release 1

Summary		: User space hardware driver for TDT equipment
Name		: tdt-driver
Version		: %{Version}
Release		: %{Release}
Group		: Applications/Scientific

Copyright	: freeware
Packager	: cmalek@etho.caltech.edu (Chris Malek)

BuildRoot	: /tmp/tdt-driver-%{Version}

Source		: ftp.etho.caltech.edu:/pub/xdphys/SOURCES/tdt-driver-%{Version}.tar.gz

%description
The tdt-driver package provides a user space driver and configuration
utilites for DAQ equipment from Tucker Davis technologies.  

%prep
%setup -n tdt-driver

%build
make 

%install
DESTDIR=$RPM_BUILD_ROOT; export DESTDIR
[ -n "`echo $DESTDIR | sed -n 's:^/tmp/[^.].*$:OK:p'`" ] && rm -rf $DESTDIR ||
(echo "Invalid BuildRoot: '$DESTDIR'! Check this .spec ..."; exit 1) || exit 1

for dir in usr/local/lib usr/local/include usr/local/bin usr/local/man; do
	install -m 755 -d $DESTDIR/$dir
done

INSTALL_ROOT=$DESTDIR/usr/local; export INSTALL_ROOT
make -e install

%files
%defattr(-,root,root)
/usr/local/lib/ap.obj
/usr/local/lib/libtdt.a
/usr/local/bin/apld
/usr/local/bin/aptst
/usr/local/bin/xbcomini
/usr/local/bin/xbsysid
/usr/local/bin/xbidchck
/usr/local/bin/tonetest
/usr/local/bin/dualtone
/usr/local/bin/rectest
/usr/local/bin/dualrec
/usr/local/bin/dualplayrec
/usr/local/bin/tg6test
/usr/local/bin/et1test
/usr/local/bin/sd1test
/usr/local/bin/tg6att
/usr/local/bin/playrec
/usr/local/bin/setpriority
/usr/local/include/tdt/apos.h
/usr/local/include/tdt/conio.h
/usr/local/include/tdt/tdtplot.h
/usr/local/include/tdt/dos.h
/usr/local/include/tdt/xbdrv.h
/usr/local/man/man1/apld.1
/usr/local/man/man1/xbcomini.1
/usr/local/man/man1/xbsysid.1
/usr/local/man/man1/xbidchck.1

