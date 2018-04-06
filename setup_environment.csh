# Set DAQ_ROOT to the directory where iserver and tdt-driver are installed.
setenv DAQ_ROOT "/usr/local"
setenv DAQPROG ${DAQ_ROOT}/bin/tdtproc
setenv DAQ_AP2_PORT apb
setenv XDPHYSDIR ${DAQ_ROOT}/lib/xdphys
setenv XAPPLRESDIR ${DAQ_ROOT}/lib/X11/app-defaults
setenv LD_LIBRARY_PATH ${DAQ_ROOT}/lib
setenv PATH ${DAQ_ROOT}/bin/xdphys:${DAQ_ROOT}/bin:$PATH
