#!/usr/bin/env sh
BINPATH=../../../bin
#$BINPATH/euRun &
$BINPATH/euRun &
sleep 1
$BINPATH/euLog &
sleep 1
$BINPATH/euCliMonitor -n CorryMonitor -t my_mon &
$BINPATH/euCliCollector -n Ex0TgDataCollector -t my_dc0 &
# The following data collectors are provided if you build user/eudet
$BINPATH/euCliProducer -n Ex0Producer -t my_pd0 &
