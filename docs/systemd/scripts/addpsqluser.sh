#!/bin/sh

createuser w1retap
psql wx <<_EOF
grant SELECT,INSERT on readings to w1retap;
grant SELECT on w1sensors to w1retap;
grant SELECT,INSERT on replog to w1retap;
grant SELECT on ratelimit to w1retap;
_EOF
