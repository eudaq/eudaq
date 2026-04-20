#!/usr/bin/env sh



BINPATH=../../../bin

"$BINPATH/euRun" -n HidraRunControl & 
sleep 1
$BINPATH/euLog &
sleep 1
$BINPATH/euCliCollector -n HidraDataCollector -t my_dc &
sleep 1
$BINPATH/euCliProducer -n HidraDryFERSProducer -t my_pd0  

