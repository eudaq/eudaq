#!/usr/bin/env sh
BINPATH=../../../bin
#$BINPATH/euRun -a tcp://44000 &
$BINPATH/euRun -n Ex0RunControl -a tcp://44000 &
sleep 1
$BINPATH/euLog -r tcp://localhost:44000 &
sleep 1
$BINPATH/euCliMonitor -n Ex0Monitor -t my_mon -r tcp://localhost:44000 & 
$BINPATH/euCliCollector -n Ex0TgDataCollector -t my_dc -r tcp://localhost:44000 &
# The following data collectors are provided if you build user/eudet
#$BINPATH/euCliCollector -n DirectSaveDataCollector -t my_dc -r tcp://localhost:44000 &
#$BINPATH/euCliCollector -n EventIDSyncDataCollector -t my_dc -r tcp://localhost:44000 &
#$BINPATH/euCliCollector -n TriggerIDSyncDataCollector -t my_dc -r tcp://localhost:44000 &
$BINPATH/euCliProducer -n Ex0Producer -t my_pd0 -r tcp://localhost:44000 &
$BINPATH/euCliProducer -n Ex0Producer -t my_pd1 -r tcp://localhost:44000 &
