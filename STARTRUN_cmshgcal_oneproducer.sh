#!/bin/bash

if [ -f KILLRUN.local ]
then
    sh KILLRUN.local
else
    sh KILLRUN.sh
fi

export RCPORT=44000
export HOSTIP=192.168.222.1
export HOSTNAME=192.168.222.1

#################  Run control ###################
./bin/euRun -n RunControl -a tcp://${RCPORT} &
sleep 2
#################  Log collector #################
./bin/euLog -r tcp://${HOSTIP}:${RCPORT} &
sleep 1
#################  Data collector #################
xterm -r -sb -sl 100000 -T "EventnumberSyncData collector" -e 'bin/euCliCollector -n EventnumberSyncDataCollector -t dc1 -r tcp://${HOSTIP}:${RCPORT}; read' &

#################  Producer #################
#gnome-terminal --geometry=80x600-280-900 -t "CMS HGCal Producer" -e "bash -c \"source ../setup_eudaq_cmshgcal.sh; ./HGCalProducer -r tcp://$HOSTIP:$RCPORT\" " &
#xterm -r -sb -sl 100000 -geometry 160x30 -T "Ex0-Producer" -e 'bin/euCliProducer -n Ex0Producer -t exo -r tcp://$HOSTIP:$RCPORT |tee -a logs/ex0.log ; read'&
xterm -r -sb -sl 100000 -geometry 160x30 -T "CMS-HGCAL2345" -e 'bin/euCliProducer -n HGCalProducer -t cms-hgcal2345 -r tcp://$HOSTIP:$RCPORT '&

xterm -r -sb -sl 100000 -geometry 200x40+300+750 -T "OnlineMon" -e 'bin/CMSHGCalMonitor  --monitor_name CMSHGCalMonitor --config_file user/cmshgcal/conf/onlinemon.conf --reduce 10 --reset -r tcp://$HOSTIP:$RCPORT |tee -a logs/mon.log ; read' &
#xterm -r -sb -sl 100000 -T "OnlineMon" -e 'bin/StdEventMonitor  --monitor_name StdEventMonitor --reset -r tcp://$HOSTIP:$RCPORT |tee -a logs/mon.log ; read' &
#xterm -r -sb -sl 100000 -T "OnlineMon" -e './bin/euCliMonitor -n Ex0Monitor -t StdEventMonitor  -a tcp://45001 | tee -a logs/euclimon.log ; read'&
