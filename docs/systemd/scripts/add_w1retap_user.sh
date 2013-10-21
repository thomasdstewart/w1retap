#!/bin/sh

# As root
groupadd w1retap
useradd -d /var/lib/w1retap -g w1retap -m -s /bin/false w1retap
mkdir -p ~w1retap/.config/w1retap
chown -R w1retap:w1retap /var/lib/w1retap/
