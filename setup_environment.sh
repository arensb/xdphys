# Set DAQ_ROOT to the directory where iserver and tdt-driver are installed.
DAQ_ROOT="/usr/local"
DAQPROG=${DAQ_ROOT}/bin/tdtproc; export DAQPROG
DAQ_AP2_PORT=apb; export DAQ_AP2_PORT
XDPHYSDIR=${DAQ_ROOT}/lib/xdphys; export XDPHYSDIR
XAPPLRESDIR=${DAQ_ROOT}/lib/X11/app-defaults; export XAPPLRESDIR
LD_LIBRARY_PATH=${DAQ_ROOT}/lib; export LD_LIBRARY_PATH
PATH=${DAQ_ROOT}/bin/xdphys:${DAQ_ROOT}/bin:$PATH; export PATH


