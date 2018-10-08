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
./bin/euCliLogger -r tcp://${HOSTIP}:${RCPORT} -a tcp:44002 &
sleep 1
#################  Data collector #################
xterm -r -sb -sl 100000 -T "EventnumberSyncData collector" -e 'bin/euCliCollector -n EventnumberSyncDataCollector -t dc1 -r tcp://${HOSTIP}:${RCPORT}; read' &

#################  Producer #################
xterm -r -sb -sl 100000 -geometry 160x30 -T "CMS-HGCAL0" -e 'bin/euCliProducer -n HGCalProducer -t cms-hgcal0 -r tcp://$HOSTIP:$RCPORT '&
xterm -r -sb -sl 100000 -geometry 160x30 -T "CMS-HGCAL1" -e 'bin/euCliProducer -n HGCalProducer -t cms-hgcal1 -r tcp://$HOSTIP:$RCPORT '&
xterm -r -sb -sl 100000 -geometry 160x30 -T "CMS-HGCAL2" -e 'bin/euCliProducer -n HGCalProducer -t cms-hgcal2 -r tcp://$HOSTIP:$RCPORT '&
xterm -r -sb -sl 100000 -geometry 160x30 -T "CMS-HGCAL3" -e 'bin/euCliProducer -n HGCalProducer -t cms-hgcal3 -r tcp://$HOSTIP:$RCPORT '&
xterm -r -sb -sl 100000 -geometry 160x30 -T "CMS-HGCAL4" -e 'bin/euCliProducer -n HGCalProducer -t cms-hgcal4 -r tcp://$HOSTIP:$RCPORT '&
xterm -r -sb -sl 100000 -geometry 160x30 -T "CMS-HGCAL5" -e 'bin/euCliProducer -n HGCalProducer -t cms-hgcal5 -r tcp://$HOSTIP:$RCPORT '&
xterm -r -sb -sl 100000 -geometry 160x30 -T "CMS-HGCAL6" -e 'bin/euCliProducer -n HGCalProducer -t cms-hgcal6 -r tcp://$HOSTIP:$RCPORT '&
xterm -r -sb -sl 100000 -geometry 160x30 -T "CMS-HGCAL7" -e 'bin/euCliProducer -n HGCalProducer -t cms-hgcal7 -r tcp://$HOSTIP:$RCPORT '&
xterm -r -sb -sl 100000 -geometry 160x30 -T "CMS-HGCAL8" -e 'bin/euCliProducer -n HGCalProducer -t cms-hgcal8 -r tcp://$HOSTIP:$RCPORT '&
xterm -r -sb -sl 100000 -geometry 160x30 -T "CMS-HGCAL9" -e 'bin/euCliProducer -n HGCalProducer -t cms-hgcal9 -r tcp://$HOSTIP:$RCPORT '&
xterm -r -sb -sl 100000 -geometry 160x30 -T "CMS-HGCAL10" -e 'bin/euCliProducer -n HGCalProducer -t cms-hgcal10 -r tcp://$HOSTIP:$RCPORT '&
xterm -r -sb -sl 100000 -geometry 160x30 -T "CMS-HGCAL11" -e 'bin/euCliProducer -n HGCalProducer -t cms-hgcal11 -r tcp://$HOSTIP:$RCPORT '&
xterm -r -sb -sl 100000 -geometry 160x30 -T "CMS-HGCAL12" -e 'bin/euCliProducer -n HGCalProducer -t cms-hgcal12 -r tcp://$HOSTIP:$RCPORT '&
xterm -r -sb -sl 100000 -geometry 160x30 -T "CMS-HGCAL13" -e 'bin/euCliProducer -n HGCalProducer -t cms-hgcal13 -r tcp://$HOSTIP:$RCPORT '&


xterm -r -sb -sl 100000 -geometry 200x40+300+750 -T "OnlineMon" -e 'bin/CMSHGCalMonitor  --monitor_name CMSHGCalMonitor --config_file user/cmshgcal/conf/onlinemon.conf --reduce 10 --reset -r tcp://$HOSTIP:$RCPORT |tee -a logs/mon.log ; read' &
# xterm -r -sb -sl 100000 -T "OnlineMon" -e 'bin/CMSHGCalMonitor  --monitor_name CMSHGCalMonitor --reset -r tcp://$HODTIP:$RCPORT |tee -a logs/mon.log ; read' &
#xterm -r -sb -sl 100000 -T "OnlineMon" -e './bin/euCliMonitor -n Ex0Monitor -t StdEventMonitor  -a tcp://45001 '&
