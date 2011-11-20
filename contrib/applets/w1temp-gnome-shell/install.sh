#!/bin/sh

ID="$(id -u)"
if [ $ID = "0" ] 
then
  prefix=/usr
  icons="share/icons/gnome/scalable/status"
else
  prefix="$HOME/.local"
  icons="share/icons"
fi

extdir="${prefix}/share/gnome-shell/extensions/w1temp@daria.co.uk"
icondir=${prefix}/${icons}

mkdir -p ${extdir}
cp -a ext/* ${extdir}/
cp -a icons/* ${icondir}/
gtk-update-icon-cache
