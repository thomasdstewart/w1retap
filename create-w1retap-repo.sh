#!/bin/bash
#    Copyright (C) 2016 Thomas Stewart <thomas@stewarts.org.uk>
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.

set -eux

vers1="0.0.1 0.0.2"
vers2="0.0.3 0.0.4 0.0.5 0.0.6 0.0.9 0.0.10 0.0.11"
vers3="0.0.7 1.0.1 1.0.2 1.0.3 1.0.4 1.0.5 1.0.6 1.0.7 1.0.9 1.1.0 1.2.0 1.2.1 1.2.2 1.2.3 1.2.4 1.2.5 1.2.6 1.2.7 1.2.8 1.2.9 1.3.0 1.3.1 1.3.3 1.3.4 1.3.5 1.3.6 1.3.7 1.3.8"
vers4="1.3.9 1.4.0 1.4.1 1.4.2"
vers5="1.4.3 1.4.4"
vers="$(echo "$vers1 $vers2 $vers3 $vers4 $vers5" | sed 's/ /\n/g' | sort -t. -k 1,1n -k 2,2n -k 3,3n | xargs)"

download () {
        wget -c http://files.maximintegrated.com/sia_bu/public/libusblinux300.tar.gz
        for ver in $vers1; do
                wget -c "http://downloads.sourceforge.net/project/w1retap/w1retap/w1retap%20$ver/w1retap-$ver.tar.gz"
        done

        for ver in $vers2; do
                wget -c "http://downloads.sourceforge.net/project/w1retap/w1retap/w1retap-$ver/w1retap-$ver.tar.gz"
        done

        for ver in $vers3; do
                wget -c "http://downloads.sourceforge.net/project/w1retap/w1retap/w1retap-$ver/w1retap-$ver.tar.bz2"
        done

        for ver in $vers4; do
                wget -c "http://downloads.sourceforge.net/project/w1retap/w1retap/w1retap-$ver/w1retap-$ver.tar.xz"
        done

        for ver in $vers5; do
                sver="$(echo "$ver" | rev | sed 's/\.//' | rev)"
                wget -c "http://downloads.sourceforge.net/project/w1retap/w1retap/w1retap-$sver/w1retap-$ver.tar.xz"
        done
}

initrepo () {
        rm -r -f w1retap
        mkdir w1retap
        cd w1retap
        git init

        cat << EOF > .gitignore
#From https://github.com/github/gitignore/blob/master/Autotools.gitignore
# http://www.gnu.org/software/automake
Makefile.in
# http://www.gnu.org/software/autoconf
autom4te.cache
autoscan.log
autoscan-*.log
aclocal.m4
compile
config.h.in
configure
configure.scan
depcomp
install-sh
missing
stamp-h1

#From https://github.com/github/gitignore/blob/master/C.gitignore
# Object files
*.o
*.ko
*.obj
*.elf

# Precompiled Headers
*.gch
*.pch

# Libraries
*.lib
*.a
*.la
*.lo

# Shared objects (inc. Windows DLLs)
*.dll
*.so
*.so.*
*.dylib

# Executables
*.exe
*.out
*.app
*.i*86
*.x86_64
*.hex

# Debug files
*.dSYM/
*.su

#Others found
.deps
config.log
config.status
config.guess
config.sub
libtool
ltmain.sh
Makefile
mkinstalldirs
.svn

EOF

        cat << EOF > README.md
w1retap
=======
The w1retap program reads weather sensors on a 1-Wire bus and logs the
retrieved data to the configured databases or files. It supports a number of
different 1-Wire host bus adapters and 1-Wire sensors designed by Dallas
Semiconductor Corp (now Maxim Integrated) and compatible sensors including
those from AAG Electrónica and other hobby sensors.

It is an open source project writen by Jonathan Hudson
<jh+w1retap@daria.co.uk>, the project page is
http://www.zen35309.zen.co.uk/wx/tech.html and it is avaliable to download from
https://sourceforge.net/projects/w1retap/.

This is an unoffical repo created with [create-w1retap-repo.sh](create-w1retap-repo.sh) by Thomas Stewart
<thomas@stewarts.org.uk>.

EOF

        cp ../create-w1retap-repo.sh .
        git add .gitignore create-w1retap-repo.sh README.md
        git commit --date "$GIT_COMMITTER_DATE" --author="Thomas Stewart <thomas@stewarts.org.uk>" -m "gitignore and script"

        cd ..
}

addlibusb () {
        mkdir -p w1retap/src
        cd w1retap/src
        tar xf ../../libusblinux300.tar.gz
        file libusblinux300/* | grep ELF | awk -F: '{print $1}' | xargs rm
        rm libusblinux300/libusb-0.1.8.tar.gz
        git add .
        d=$(stat --format=%y ../../libusblinux300.tar.gz)
        fh="$(sha512sum ../../libusblinux300.tar.gz | awk '{print $1}')"
        git commit -a --date "$d" --author="Dalas Semiconductor <sales@maximintegrated.com>" -m "libusblinux300.tar.gz" -m "sha512sum $fh"
        cd ../../
        rm -r -f libusblinux300
}

download
if [ ! -d w1retap ]; then
        export GIT_COMMITTER_DATE="2016-10-01T00:00:00"
        initrepo
        addlibusb
fi

cd w1retap
for v in $vers; do
        if [ "$(git tag -l "$v" | wc -l)" -eq 1 ]; then
                continue
        fi

        f="$(ls ../w1retap-"$v".tar.*)"
        fn="$(basename "$f")"
        fh="$(sha512sum "$f" | awk '{print $1}')"

        tar xf "$f" --strip 1

        find . -type f -exec file {} \; | grep ELF | awk -F':' '{print $1}' | xargs rm -r -f -v
        find . -type f -name '*~*' -delete
        find . -type f -name 'trcpy*' -delete
        find . -type f -name 'hs_err_*' -delete

        git add .
        d="$(stat --format=%y "$f")"
        git commit -a --date "$d" --author="Jonathan Hudson <jh+w1retap@daria.co.uk>" -m "$fn" -m "sha512sum $fh"
        git tag -a "${v}" -m "w1retap $v"
done
