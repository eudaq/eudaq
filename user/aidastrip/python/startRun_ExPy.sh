#!/usr/bin/env sh
export EUDAQ=/opt/eudaq2/
export PATH=$PATH:$EUDAQ/bin

euRun -n kpixRunControl &
sleep 1

euCliProducer -n AidaTluProducer -t aida_tlu &
sleep 1

#$BINPATH/euCliCollector -n kpixDataCollector -t kpixDC -r tcp://192.168.200.1:44000 &
#euCliCollector -n kpixDataCollector -t kpixDC  &
#sleep 1

python3 kpixPyProducer.py  &
sleep 1

# Online Monitor:
#$BINPATH/StdEventMonitor -t StdEventMonitor &
