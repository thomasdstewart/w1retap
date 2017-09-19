#
# Must specify target link file on the command line, i.e.:
#
#     make all LINKFILE=libusbwlnk.o
#     make atodtst LINKFILE=libusbllnk.o
#     make temp LINKFILE=myWeirdOSlnk.c
#
# or, call make with either of the following two targets
# specified, 'win32' or 'linux':
#
#     make win32 (libusb not ready for cygwin yet)
#     make linux
#
# This will cause the make file to use the default link
# file for the target platform and build all applications.
#

CC = gcc

# Cygwin flags
#CFLAGS = -mno-cygwin -DDEBUG -c
#LFLAGS = -mno-cygwin -DDEBUG -g -o $@

CFLAGS = -DDEBUG -c
LFLAGS = -DDEBUG -g -o $@

ONEWIREOBJS = $(LINKFILE) libusbds2490.o libusbnet.o libusbses.o libusbtran.o \
	owerr.o ioutil.o crcutil.o
WEATHEROBJS = weather.o atod20.o cnt1d.o swt12.o temp10.o $(ONEWIREOBJS)
MEMOBJS = mbappreg.o mbeprom.o mbnvcrc.o mbscrcrc.o mbscrex.o mbshaee.o \
	mbee.o mbee77.o mbnv.o mbscr.o mbscree.o mbscrx77.o mbsha.o pw77.o \
	rawmem.o $(ONEWIREOBJS)
FILEOBJS = owfile.o owpgrw.o owprgm.o owcache.o $(MEMOBJS)
SHAOBJS = sha18.o sha33.o shaib.o $(FILEOBJS)
PROGS = atodtst counter coupler fish gethumd memutil mweather shaapp swtloop \
	swtsngl temp thermodl thermoms tstfind debit debitvm humalog initcopr \
	initcoprvm initrov initrovvm jibtest jibload jibmodpow ps_check ps_init \
	sha_chck sha_init tm_check tm_init
EXTLIBS = -lusb


win32:
	make all LINKFILE=libusbwlnk.o

linux:
	make all LINKFILE=libusbllnk.o

all: $(PROGS)


atodtst:  atodtst.c atod20.o findtype.o $(ONEWIREOBJS)
	$(CC) $(LFLAGS) atodtst.c atod20.o findtype.o $(ONEWIREOBJS) $(EXTLIBS)

counter:  counter.c cnt1d.o findtype.o $(ONEWIREOBJS)
	$(CC) $(LFLAGS) counter.c cnt1d.o findtype.o $(ONEWIREOBJS) $(EXTLIBS)

coupler: coupler.c swt1f.o findtype.o $(ONEWIREOBJS)
	$(CC) $(LFLAGS) coupler.c swt1f.o findtype.o $(ONEWIREOBJS) $(EXTLIBS)

debit: shademo.c shadebit.o $(SHAOBJS)
	$(CC) $(LFLAGS) shademo.c shadebit.o $(SHAOBJS) $(EXTLIBS)

debitvm: shademo.c shadbtvm.o $(SHAOBJS)
	$(CC) -DCOPRVM $(LFLAGS) shademo.c shadbtvm.o $(SHAOBJS) $(EXTLIBS)

fish: fish.c $(FILEOBJS)
	$(CC) $(LFLAGS) fish.c $(FILEOBJS) $(EXTLIBS)

gethumd: gethumd.c atod26.o screenio.o findtype.o $(ONEWIREOBJS)
	$(CC) $(LFLAGS) gethumd.c atod26.o screenio.o findtype.o $(ONEWIREOBJS) $(EXTLIBS)

humalog: humalog.c findtype.o humutil.o mbee77.o mbscrx77.o pw77.o $(ONEWIREOBJS)
	$(CC) $(LFLAGS) -lm humalog.c findtype.o humutil.o mbee77.o mbscrx77.o pw77.o $(ONEWIREOBJS) $(EXTLIBS)

initcopr: shacopr.c shadebit.o $(SHAOBJS)
	$(CC) $(LFLAGS) shacopr.c shadebit.o $(SHAOBJS) $(EXTLIBS)

initcoprvm: shacopr.c shadbtvm.o $(SHAOBJS)
	$(CC) -DCOPRVM $(LFLAGS) shacopr.c shadbtvm.o $(SHAOBJS) $(EXTLIBS)

initrov: sharov.c shadebit.o $(SHAOBJS)
	$(CC) $(LFLAGS) sharov.c shadebit.o $(SHAOBJS) $(EXTLIBS)

initrovvm: sharov.c shadbtvm.o $(SHAOBJS)
	$(CC) -DCOPRVM $(LFLAGS) sharov.c shadbtvm.o $(SHAOBJS) $(EXTLIBS)

jibload: jibload.c jib96.o jib96o.o findtype.o $(ONEWIREOBJS)
	$(CC) $(LFLAGS) jibload.c jib96.o jib96o.o findtype.o $(ONEWIREOBJS) $(EXTLIBS)

jibmodpow: jibmodpw.c jib96.o jib96o.o findtype.o $(ONEWIREOBJS)
	$(CC) $(LFLAGS) jibmodpw.c jib96.o jib96o.o findtype.o $(ONEWIREOBJS) $(EXTLIBS)

jibtest: jibtest.c jib96.o jib96o.o findtype.o $(ONEWIREOBJS)
	$(CC) $(LFLAGS) jibtest.c jib96.o jib96o.o findtype.o $(ONEWIREOBJS) $(EXTLIBS)

memutil: memutil.c $(FILEOBJS)
	$(CC) $(LFLAGS) memutil.c $(FILEOBJS) $(EXTLIBS)

mweather: mweather.c findtype.o $(WEATHEROBJS)
	$(CC) $(LFLAGS) mweather.c findtype.o $(WEATHEROBJS) $(EXTLIBS)

ps_check: ps_check.c ps02.o findtype.o $(ONEWIREOBJS)
	$(CC) $(LFLAGS) ps_check.c ps02.o findtype.o $(ONEWIREOBJS) $(EXTLIBS)

ps_init: ps_init.c ps02.o findtype.o $(ONEWIREOBJS)
	$(CC) $(LFLAGS) ps_init.c ps02.o findtype.o $(ONEWIREOBJS) $(EXTLIBS)

sha_chck: sha_chck.c shadbtvm.o $(SHAOBJS)
	$(CC) $(LFLAGS) sha_chck.c shadbtvm.o $(SHAOBJS) $(EXTLIBS)

sha_init: sha_init.c shadbtvm.o $(SHAOBJS)
	$(CC) $(LFLAGS) sha_init.c shadbtvm.o $(SHAOBJS) $(EXTLIBS)

shaapp: shaapp.c ibsha33o.o $(ONEWIREOBJS)
	$(CC) $(LFLAGS) shaapp.c ibsha33o.o $(ONEWIREOBJS) $(EXTLIBS)

swtloop: swtloop.c swt12.o findtype.o $(ONEWIREOBJS)
	$(CC) $(LFLAGS) swtloop.c swt12.o findtype.o $(ONEWIREOBJS) $(EXTLIBS)

swtsngl: swtsngl.c swt05.o findtype.o $(ONEWIREOBJS)
	$(CC) $(LFLAGS) swtsngl.c swt05.o findtype.o $(ONEWIREOBJS) $(EXTLIBS)

temp:  temp.c temp10.o findtype.o $(ONEWIREOBJS)
	$(CC) $(LFLAGS) temp.c temp10.o findtype.o $(ONEWIREOBJS) $(EXTLIBS)

thermodl: thermodl.c thermo21.o findtype.o $(ONEWIREOBJS)
	$(CC) $(LFLAGS) thermodl.c thermo21.o findtype.o $(ONEWIREOBJS) $(EXTLIBS)

thermoms: thermoms.c thermo21.o findtype.o $(ONEWIREOBJS)
	$(CC) $(LFLAGS) thermoms.c thermo21.o findtype.o $(ONEWIREOBJS) $(EXTLIBS)

tm_check: tm_check.c time04.o findtype.o $(FILEOBJS)
	$(CC) $(LFLAGS) tm_check.c time04.o findtype.o $(FILEOBJS) $(EXTLIBS)

tm_init: tm_init.c time04.o findtype.o $(FILEOBJS)
	$(CC) $(LFLAGS) tm_init.c time04.o findtype.o $(FILEOBJS) $(EXTLIBS)

tstfind: tstfind.c $(ONEWIREOBJS)
	$(CC) $(LFLAGS) tstfind.c $(ONEWIREOBJS) $(EXTLIBS)

%.o: %.c
	$(CC) $(CFLAGS) $<

clean:
	rm -f *.o *.exe $(PROGS)

