#!/usr/bin/env sh
BINPATH=../../../bin
$BINPATH/euRun &
#$BINPATH/euRun -n DualROCaloRunControl &
sleep 1
$BINPATH/euLog &
sleep 1
$BINPATH/euCliCollector -n DualROCaloDataCollector -t my_dc &
$BINPATH/euCliProducer -n DualROCaloProducer -t my_pd0 &
