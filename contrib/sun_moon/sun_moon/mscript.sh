#!/bin/bash

for ((D=0;D<28;D+=1))
do
 echo $(date -d "2011-01-04+${D}day")
 ./lunar -t $(date -d "2011-01-04+${D}day" +%s) | egrep 'disk|bright'
done

