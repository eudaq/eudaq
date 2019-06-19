#!/usr/bin/env sh                                                                                         
BINPATH=../../../bin
$BINPATH/euRun -n kpixRunControl &

sleep 1

$BINPATH/euCliCollector -n kpixDataCollector -t kpixDC &

#$BINPATH/euCliCollector  -t kpixDC &

sleep 1

python3 kpixPyProducer.py
