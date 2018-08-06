#!/usr/bin/env sh
BINPATH=../../../bin
$BINPATH/euRun &
sleep 1
$BINPATH/euLog &
sleep 1
$BINPATH/euCliMonitor -n Ex0Monitor -t my_mon &
$BINPATH/euCliCollector -n Ex0TgDataCollector -t my_dc &
#$BINPATH/euCliCollector -n DirectSaveDataCollector -t my_dc &
#$BINPATH/euCliCollector -n EventIDSyncDataCollector -t my_dc &
#$BINPATH/euCliCollector -n TriggerIDSyncDataCollector -t my_dc &
$BINPATH/euCliProducer -n Ex0Producer -t my_pd0 &
$BINPATH/euCliProducer -n Ex0Producer -t my_pd1 &
