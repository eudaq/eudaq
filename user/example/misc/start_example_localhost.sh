#!/usr/bin/env sh
HOST=localhost
RCPORT=44000
BINPATH=../../../bin

#$BINPATH/euRun -a tcp://$RCPORT &
$BINPATH/euRun -n Ex0RunControl -a tcp://$RCPORT &
sleep 1
$BINPATH/euLog -r tcp://$HOST:$RCPORT &
sleep 1
$BINPATH/euCliMonitor -n Ex0Monitor -t my_mon -r tcp://$HOST:$RCPORT & 
$BINPATH/euCliCollector -n Ex0TgDataCollector -t my_dc -r tcp://$HOST:$RCPORT &
# The following data collectors are provided if you build user/eudet
#$BINPATH/euCliCollector -n DirectSaveDataCollector -t my_dc -r tcp://$HOST:$RCPORT &
#$BINPATH/euCliCollector -n EventIDSyncDataCollector -t my_dc -r tcp://$HOST:$RCPORT &
#$BINPATH/euCliCollector -n TriggerIDSyncDataCollector -t my_dc -r tcp://$HOST:$RCPORT &
$BINPATH/euCliProducer -n Ex0Producer -t my_pd0 -r tcp://$HOST:$RCPORT &
$BINPATH/euCliProducer -n Ex0Producer -t my_pd1 -r tcp://$HOST:$RCPORT &
