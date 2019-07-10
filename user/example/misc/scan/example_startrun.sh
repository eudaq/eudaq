#!/usr/bin/env sh
BINPATH=../../../../bin
#$BINPATH/euRun &
$BINPATH/euRun -n Ex0RunControl &
sleep 2
$BINPATH/euLog &
sleep 2
$BINPATH/euCliMonitor -n Ex0Monitor -t my_mon & 
$BINPATH/euCliCollector -n Ex0TgDataCollector -t my_dc &
$BINPATH/euCliProducer -n Ex0Producer -t my_pd0 &
$BINPATH/euCliProducer -n Ex0Producer -t my_pd1 &
