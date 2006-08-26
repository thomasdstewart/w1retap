#!/bin/bash

psql sensors <<EOF
update w1sensors set type='DS1820' where type='Temperature';
update w1sensors set type='TAI8540' where type='Humidity';
update w1sensors set type='TAI8570' where type='Pressure';
update w1sensors set type='TAI8575' where type='RainGauge';
EOF
