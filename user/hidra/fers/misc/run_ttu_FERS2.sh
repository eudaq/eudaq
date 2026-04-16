#!/usr/bin/env sh

killall -9 euCliProducer
killall -9 euCliCollector
killall -9 euCliMonitor
killall -9 euLog
killall -9 euRun

BINPATH=/home/daq/eudaq_flib42/bin

$BINPATH/euRun &
sleep 1
$BINPATH/euLog &
sleep 1

$BINPATH/euCliMonitor -n FERSROOTMonitor -t my_mon0 &
sleep 1

$BINPATH/euCliCollector -n FERSDataCollector -t my_dc0 &

sleep 1

$BINPATH/euCliProducer -n FERSProducer -t my_fers0 &
sleep 1

$BINPATH/euCliProducer -n FERSProducer -t my_fers1 &
sleep 1


#$BINPATH/euCliProducer -n DRSProducer -t my_drs0 &
#sleep 1

#$BINPATH/euCliProducer -n QTPDProducer -t my_qtp0 &

