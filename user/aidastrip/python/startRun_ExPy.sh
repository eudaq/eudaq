#!/usr/bin/env sh                                                                                         
BINPATH=/opt/eudaq2/bin
#$BINPATH/euRun -n kpixRunControl &
#sleep 1

#$BINPATH/euCliCollector -n kpixDataCollector -t kpixDC -r tcp://192.168.200.1:44000 &
$BINPATH/euCliCollector -n kpixDataCollector -t kpixDC  &
sleep 1

python3 kpixPyProducer.py &
sleep 1

# Online Monitor:
#$BINPATH/StdEventMonitor -t StdEventMonitor &
