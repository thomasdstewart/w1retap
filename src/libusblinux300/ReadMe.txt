                       LIBUSB Linux GCC Build
                      1-Wire Public Domain Kit
                         V3.00 beta 07/23/04

This is a build of all of the current (3.00) 1-Wire Public
Domain demo applications using interface modules to communicate
directly with the DS2490 based USB adapters (through libusb)
such as the DS9490R/DS9490B on Linux platforms.

This build will be of most interest to security conscience applications
such as Software Authorization that do not want to go through intermediate
software layers (DLLs) to access the USB adapters.

This build is not strictly a 'userial' or 'general' build as
defined in the 1-Wire Public Domain kit but is somewhere
in between.  The interface modules were written in a way to take
advantage of the features of the DS2490 chip which has built-in
1-Wire search capabilities. For example, instead of constructing
the 1-Wire search using single 'bit' operations, it translates
owFirst directly into calls to libusb.

The main interface modules are:

  libusbds2490.h
  libusbds2490.c
  libusbllnk.c
  libusbnet.c
  libusbtran.c
  libusbses.c
  usb.h
  libusb  (see included libusb distribution package)

Note that the port name to provide on the command line to the demos
is in the format DS2490-X where X is a number starting at 1.
This is the USB enumeration number.

These other files are also required for most applications:

  crcutil.c
  ioutil.c
  owerr.c
  ownet.h

The different demos are documented in the main 1-Wire Public Domain
3.00 kit that can be found here:
  http://www.ibutton.com/software/1wire/wirekit.html

The source and binaries for the demos can be found in this folder.
A makefile that can be used to either make individual sample programs or all
sample programs is also available in this folder.  Use the following
command at a terminal prompt to build all of the executables:

make linux


One of the pre-requisites of this Public Domain Kit build is
that "libusb" be present on the computer system where the programs
will be run.  If "libusb" has not already been installed, then use
the libusb distribution archive contained in this folder
(libusb-0.1.8.tar.gz).  This archive contains all the necessary
information on how to build and install the libusb shared library.

The libusb sourceforge project can be found here (support for
Linux, MacOSX, FreeBSD, etc):
  http://libusb.sourceforge.net

The Win32 port of libusb can be found here:
  http://libusb-win32.sourceforge.net

