#!/usr/bin/env sh
BINPATH=../../../bin
$BINPATH/euRun -n Ex0RunControl -a tcp://44000 &
sleep 1
$BINPATH/euLog -a tcp://44001&
sleep 1
$BINPATH/euCliMonitor -n Ex0Monitor -t my_mon  -a tcp://45001 &
$BINPATH/euCliCollector -n Ex0TgDataCollector -t my_dc -a tcp://45002 &
$BINPATH/euCliProducer -n Ex0Producer -t my_pd0 &
$BINPATH/euCliProducer -n Ex0Producer -t my_pd1 &
