#!/bin/bash

if [ -f KILLRUN.local ]
then
    sh KILLRUN.local
else
    sh KILLRUN.sh
fi

export RCPORT=44000
export HOSTIP=127.0.0.1
export HOSTNAME=127.0.0.1

#################  Run control ###################
./bin/euRun -n RunControl -a tcp://${RCPORT} &
sleep 2
#################  Log collector #################
./bin/euLog -r tcp://${HOSTIP}:${RCPORT} &
sleep 1
#################  Data collector #################
xterm -r -sb -sl 100000 -T "DirectSaveData collector" -e 'bin/euCliCollector -n DirectSaveDataCollector -t dc1 -r tcp://${HOSTIP}:${RCPORT}; read' &

#################  Producer #################
xterm -r -sb -sl 100000 -geometry 160x30 -T "CMS-HGCAL-MCP" -e 'bin/euCliProducer -n CMSHGCal_MCP_Producer -t cms-hgcal-mcp -r tcp://$HOSTIP:$RCPORT '&


#xterm -r -sb -sl 100000 -geometry 200x40+300+750 -T "OnlineMon" -e 'bin/CMSHGCalMonitor  --monitor_name CMSHGCalMonitor --config_file user/cmshgcal/conf/onlinemon.conf --reduce 10 --reset -r tcp://$HOSTIP:$RCPORT |tee -a logs/mon.log ; read' &