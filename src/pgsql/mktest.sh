#!/bin/bash

gcc -o pgtest -D PGV=7  -D TESTBIN  -I  ../libusblinux300/ -I../ \
 -I $(pg_config --includedir)  \
 $(pkg-config gmodule-2.0 --cflags) \
 w1pgsql.c ../w1util.c  -lpq $(pkg-config gmodule-no-export-2.0 --libs)
