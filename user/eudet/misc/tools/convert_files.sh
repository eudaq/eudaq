#!/bin/bash
# convert all raw files to slcio without date-suffix 

CONVERTER=/opt/eudaq2/bin/euCliConverter 

for i in *.raw; do
    #echo `basename $i .raw`
    echo 'processing' $i
    $CONVERTER -i $i -o ${i:0:9}.slcio
done
