
If you get a segmentation violation while trying to use a MEX file in
MATLAB (e.g. using is_init, which lives in iserver.mexlx), first check
to see how the MEX file was linked:

	cd /usr/local/local-matlab
	ldd iserver.mexlx

You should see this:

	> ldd iserver.mexlx
		statically linked

You should NOT see this:

	> ldd iserver.mexlx
	libc.so.6 => /lib/libc.so.6 (0x4000c000)
	/lib/ld-linux.so.2 => /lib/ld-linux.so.2 (0x80000000)

It seems that MATLAB cannot %@#$#$ deal with MEX files that were linked
against glibc2.  They must be linked against libc5, even though glibc2
has been around for 1.5 #$@! years.

You have to make sure that mex, the program we use to compile C code
into MEX files is using the proper compiler.

You will not have this problem unless you are using Redhat 6.x or greater,
or Debian 2.0 or greater.  Here's the fix for Redhat 6.x:

Go to the following place:

	ftp://ftp.valinux.com/pub/support/hjl/gcc/gcc-2.95.x

and download:

	gcc-libc5-2.95.3-0.20000413.1.i386.rpm

Install this file.

Now, let's say you have MATLAB installed in /usr/local/matlab.  Edit 
/usr/local/matlab/bin/mexopts.sh, and look for the "lnx86" section.

Look for

		if [ "$version" = "5" ]; then
		    #CC='i486-linuxlibc5-gcc'
			CC=gcc
		fi


and change it to

		if [ "$version" = "5" ]; then
		    #CC='i486-linuxlibc5-gcc'
			CC=gcc
		elif [ "$version" = "6" ]; then
		    CC='/opt/i386-pc-linux-gnulibc1/bin/i386-pc-linux-gnulibc1-gcc'
		fi

and make your MEX file again.  It should be fine now.

As for Debian, here's a link to the Mathworks site:

	http://www.mathworks.com/support/solutions/data/22576.shtml


-- cmalek, Oct 5, 2000 
