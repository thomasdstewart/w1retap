#!/bin/bash

gcc -O2 -g -o pgtest -D PGV=8 -D_GNU_SOURCE=1  -D TESTBIN  -I  ../libusblinux300/ -I../ \
 -I $(pg_config --includedir)  \
 $(pkg-config gmodule-2.0 --cflags) \
 w1pgsql.c ../w1util.c  -lpq $(pkg-config gmodule-no-export-2.0 --libs)
