#!/bin/sh

# Requires libnova <http://libnova.sourceforge.net/index.html>
# Must be installed in /usr/local/lib (the default)
gcc -Wall -O2 -s -osun_moon sun_moon.c  -Wl,-rpath,/usr/local/lib -lnova
[ -d ~/bin ] && cp sun_moon ~/bin/
echo "Copy sun_moon somewhere useful!!"
