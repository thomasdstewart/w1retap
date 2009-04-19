#!/bin/bash

set -x

DATE=$(date +%F)
NAME=ds192xtest
PROG=$NAME.$DATE

if  [ -e  /usr/lib/w1retap/libw1common.so.0 ]
then 
 LDPATH=/usr/lib/w1retap
elif [ -e  /usr/local/lib/w1retap/libw1common.so.0 ]
then
 LDPATH=/usr/local/lib/w1retap/
else
 echo "No obvious library path"
 exit
fi

rm -f $NAME.*

gcc -DHAVE_CONFIG_H -I. -I../..  -Wall -D_GNU_SOURCE=1 -I . \
 -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include  -Werror \
 -DTESTMAIN=1 -g -O2 -c -o ${NAME}.o ds192x.c

gcc  -g -O2 -rdynamic  -Wl,--export-dynamic -lgmodule-2.0 -lglib-2.0 \
  -o $PROG $NAME.o findtype.o -L . -L $LDPATH -lowfat -lw1common -lrt -lm \
  -Wl,-rpath -Wl,$LDPATH
