#!/usr/bin/env sh
BINPATH=../../../bin
#$BINPATH/euRun &
$BINPATH/euRun -n Ex0RunControl &
sleep 1
$BINPATH/euLog &
sleep 1
$BINPATH/euCliMonitor -n Ex0Monitor -t my_mon &
$BINPATH/euCliCollector -n Ex0TgDataCollector -t my_dc &
# The following data collectors are provided if you build user/eudet
$BINPATH/euCliCollector -n DirectSaveDataCollector -t direct_dc &
$BINPATH/euCliCollector -n EventIDSyncDataCollector -t event_dc &
$BINPATH/euCliCollector -n TriggerIDSyncDataCollector -t trig_dc &
$BINPATH/euCliProducer -n Ex0Producer -t my_pd0 &
$BINPATH/euCliProducer -n Ex0Producer -t my_pd1 &
