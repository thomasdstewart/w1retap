#!/bin/bash

for M in {0..55}
do
 ID=$(printf "%02d" $M)
 POS=$(( 48 * M ))
 convert moon_56frames.png -crop 48x50+${POS}+0 m$ID.png
done
