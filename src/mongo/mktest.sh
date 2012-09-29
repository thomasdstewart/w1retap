#!/bin/bash

CFLAGS=$(pkg-config --cflags libmongo-client gmodule-no-export-2.0)
LDFLAGS=$(pkg-config --libs libmongo-client gmodule-no-export-2.0)

gcc -Wall -O2 -DTESTBIN=1 -D_GNU_SOURCE=1 -g -o mongotest  -I../libusblinux300/ -I../ $CFLAGS \
 w1mongo.c ../w1util.c  $LDFLAGS
