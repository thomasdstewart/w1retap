#!/bin/sh

# Create the daily and monthly tables

sqlite3 /var/tmp/sensors.db <<EOF
create table daily
(
   udate int,
   rain0 real,
   rain1 real   
   
);

create table monthly
(
   udate int,
   rain0 real,
   rain1 real   
);

.import /tmp/d.sql daily
.import /tmp/m.sql monthly

EOF
