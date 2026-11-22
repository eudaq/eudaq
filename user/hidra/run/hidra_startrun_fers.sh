!/usr/bin/env sh
BINPATH=../../../bin
$BINPATH/euRun -n HidraRunControl &
sleep 1
#$BINPATH/hidraLog &
sleep 1
#$BINPATH/euCliCollector -n HidraDataCollector -t HidraDataCollector &
sleep 1
#$BINPATH/euCliProducer -n HidraQTPDProducer -t my_pd0 
$BINPATH/euCliProducer -n HidraFERS2Producer -t FERS2Producer 
