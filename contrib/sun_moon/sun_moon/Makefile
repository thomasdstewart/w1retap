CC=gcc
LDFLAGS=-lnova
CFLAGS=-Wall -O2 -s

all: solar lunar sun_moon

solar: solar.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

lunar: lunar.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

sun_moon: sun_moon.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
 
clean: 
	rm -f solar lunar sun_moon *.o

install:
	cp solar lunar sun_moon ~/bin/

dist:
	rm -f solar lunar sun_moon *.o
	tar -C .. -czf ../sun_moon.tar.gz sun_moon
