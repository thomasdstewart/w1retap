#!/bin/bash

gcc -O2 -g -o mongotest  -DMONGO_HAVE_STDINT=1 -I /usr/include/mongoc -D TESTBIN  -I  ../libusblinux300/ -I../ \
 $(pkg-config gmodule-2.0 --cflags) \
 w1mongo.c ../w1util.c  -lmongoc $(pkg-config gmodule-no-export-2.0 --libs)
