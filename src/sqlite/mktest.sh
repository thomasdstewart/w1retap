#!/bin/bash
CC=clang
$CC -O2 -g -o sqltest -D_GNU_SOURCE=1  -D TESTBIN  -I  ../libusblinux300/ -I../ \
 -I $(pg_config --includedir)  \
 $(pkg-config gmodule-2.0 --cflags) \
 w1sqlite.c ../w1util.c ../w1conf.c  -lsqlite3 $(pkg-config gmodule-no-export-2.0 --libs)
rm -f  w1sqlite.o
