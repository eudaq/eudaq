#!/usr/bin/env sh
BINPATH=../../../bin
#$BINPATH/euRun &
$BINPATH/euRun -n Ex0RunControl &
sleep 2
$BINPATH/euCliCollector -n QTPDPaviaDataCollector -t my_dc &
#$BINPATH/euCliProducer -n Ex0Producer -t my_pd0 &
$BINPATH/euCliProducer -n QTPDPaviaProducer -t my_pd0 
