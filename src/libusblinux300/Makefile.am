EXTRA_DIST=$(wildcard *.c) $(wildcard *.h) makefile.shrlib README ReadMe.txt mkds192xtest.sh

AM_CPPFLAGS = -Wall -D_GNU_SOURCE=1 -I . $(GLIB_CFLAGS) $(LIBUSB_CFLAGS)

bin_PROGRAMS=w1find braybaro temp gethumd humids setds2409 coupler counter mweather lcdtest atodtst humalog thermoms thermodl ds192xtest
 
pkglib_LTLIBRARIES = libw1common.la libw1serial.la libw1usb.la
noinst_LIBRARIES = libowfat.a

libw1common_la_SOURCES = owerr.c ioutil.c crcutil.c acquire.c
libw1common_la_LIBADD = $(GLIB_LIBS)
libw1common_la_LDFLAGS = -rdynamic

libw1serial_la_SOURCES = ds2480ut.c linuxlnk.c owllu.c ownetu.c owsesu.c owtrnu.c
libw1serial_la_LIBADD = -L. -lw1common
libw1serial_la_DEPENDENCIES = libw1common.la

libw1usb_la_SOURCES = libusbllnk.c libusbds2490.c libusbnet.c libusbses.c libusbtran.c
libw1usb_la_LIBADD = -L. -lw1common $(LIBUSB_LIBS)
libw1usb_la_DEPENDENCIES = libw1common.la

libowfat_a_SOURCES = mbappreg.c mbeprom.c	     \
	mbnvcrc.c mbscrcrc.c mbscrex.c	     \
	mbshaee.c mbee.c mbee77.c mbnv.c \
	mbscr.c mbscree.c mbscrx77.c		     \
	mbsha.c pw77.c rawmem.c owfile.c \
	owpgrw.c owprgm.c owcache.c		     \
	temp10.c atod26.c screenio.c		     \
	cnt1d.c swt12.c pressure.c sht11_humid.c \
	swt1f.c atod20.c ds2408lcd.c ds2760.c humutil.c \
	ds192x.c

w1find_SOURCES = w1find.c findtype.c
w1find_DEPENDENCIES = libw1common.la
w1find_LDADD = -L. -lowfat -lw1common -lm
w1find_LDFLAGS = -rdynamic  $(GLIB_LIBS)

braybaro_SOURCES = braybaro.c atod26.c screenio.c findtype.c
braybaro_DEPENDENCIES = libw1common.la
braybaro_LDADD = -L. -lowfat -lw1common -lm
braybaro_LDFLAGS = -rdynamic  $(GLIB_LIBS)

temp_SOURCES = temp.c findtype.c
temp_DEPENDENCIES = libw1common.la
temp_LDADD = -L. -lowfat -lw1common -lm
temp_LDFLAGS = -rdynamic  $(GLIB_LIBS)

gethumd_SOURCES = gethumd.c findtype.c
gethumd_DEPENDENCIES = libw1common.la
gethumd_LDADD = -L. -lowfat -lw1common -lm
gethumd_LDFLAGS = -rdynamic  $(GLIB_LIBS)

coupler_SOURCES = coupler.c findtype.c
coupler_DEPENDENCIES = libw1common.la
coupler_LDADD = -L. -lowfat -lw1common -lm
coupler_LDFLAGS = -rdynamic  $(GLIB_LIBS)

humids_SOURCES = humid.c
humids_DEPENDENCIES = libw1common.la
humids_LDADD = -L. -lowfat -lw1common -lm
humids_LDFLAGS = -rdynamic  $(GLIB_LIBS)

setds2409_SOURCES = setds2409.c
setds2409_DEPENDENCIES = libw1common.la
setds2409_LDADD = -L. -lowfat -lw1common -lm
setds2409_LDFLAGS = -rdynamic  $(GLIB_LIBS)

counter_SOURCES = counter.c findtype.c
counter_DEPENDENCIES = libw1common.la
counter_LDADD = -L. -lowfat -lw1common -lm
counter_LDFLAGS = -rdynamic  $(GLIB_LIBS)

mweather_SOURCES = mweather.c  findtype.c  weather.c 
mweather_DEPENDENCIES = libw1common.la
mweather_LDADD = -L. -lowfat -lw1common -lm
mweather_LDFLAGS = -rdynamic  $(GLIB_LIBS)

lcdtest_SOURCES = lcdtest.c findtype.c
lcdtest_DEPENDENCIES = libw1common.la 
lcdtest_LDADD = -L. -lowfat -lw1common -lm 
lcdtest_LDFLAGS = -rdynamic  $(GLIB_LIBS) 

atodtst_SOURCES = atodtst.c findtype.c
atodtst_DEPENDENCIES = libw1common.la 
atodtst_LDADD = -L. -lowfat -lw1common -lm 
atodtst_LDFLAGS = -rdynamic  $(GLIB_LIBS) 

humalog_SOURCES = humalog.c findtype.c
humalog_DEPENDENCIES = libw1common.la 
humalog_LDADD = -L. -lowfat -lw1common -lm 
humalog_LDFLAGS = -rdynamic  $(GLIB_LIBS) 

thermoms_SOURCES = thermoms.c findtype.c thermo21.c
thermoms_DEPENDENCIES = libw1common.la 
thermoms_LDADD = -L. -lowfat -lw1common -lm 
thermoms_LDFLAGS = -rdynamic  $(GLIB_LIBS) 

thermodl_SOURCES = thermodl.c findtype.c thermo21.c
thermodl_DEPENDENCIES = libw1common.la 
thermodl_LDADD = -L. -lowfat -lw1common -lm 
thermodl_LDFLAGS = -rdynamic  $(GLIB_LIBS) 

ds192xtest_SOURCES = ds192x.c 
ds192xtest_DEPENDENCIES = libw1common.la
ds192xtest_LDADD = -L. -lowfat -lw1common -lm 
ds192xtest_LDFLAGS = -rdynamic  $(GLIB_LIBS) 
ds192xtest_CPPFLAGS = -DTESTMAIN=1 