#!/usr/bin/env sh
BINPATH=../../../bin
$BINPATH/euRun -a tcp://44000 &
sleep 1
$BINPATH/euLog -r tcp://localhost:44000 &
sleep 1
$BINPATH/euCliMonitor -n Ex0Monitor -t my_mon -r tcp://localhost:44000 & 
$BINPATH/euCliCollector -n Ex0TgDataCollector -t my_dc -r tcp://localhost:44001-a tcp://45000 &
#$BINPATH/euCliCollector -n DirectSaveDataCollector -t my_dc -r tcp://localhost:44000 &
#$BINPATH/euCliCollector -n EventIDSyncDataCollector -t my_dc -r tcp://localhost:44000 &
#$BINPATH/euCliCollector -n TriggerIDSyncDataCollector -t my_dc -r tcp://localhost:44000 &
$BINPATH/euCliProducer -n Ex0Producer -t my_pd0 -r tcp://localhost:44000 &
$BINPATH/euCliProducer -n Ex0Producer -t my_pd1 -r tcp://localhost:44000 &
