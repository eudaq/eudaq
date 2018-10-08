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

source setup_eudaq_cmshgcal.sh

dt=`date +"%y_%b_%d_%Hh%Mm"`
echo $dt
if [ -f './data/runnumber.dat' ]; then
    RUNNUM=$(cat './data/runnumber.dat')
else
    RUNNUM=0
fi

NEWRUNNUM=$((RUNNUM+1))
echo 'Last Run number: ' $RUNNUM 'New Run number:' $NEWRUNNUM


#################  Run control ###################
./bin/euRun -n RunControl -a tcp://${RCPORT} &
sleep 2
#################  Log collector #################
./bin/euCliLogger -r tcp://${HOSTIP}:${RCPORT} -a tcp://44002 &
sleep 1
#################  Data collector #################
xterm -r -sb -sl 100000 -T "EventnumberSyncData collector" -e 'bin/euCliCollector -n EventnumberSyncDataCollector -t dc1 -r tcp://${HOSTIP}:${RCPORT} -a tcp://45001; read' &

#################  Online DQM #####################
xterm -r -sb -sl 100000 -geometry 200x40+300+750 -T "OnlineMon" -e 'bin/CMSHGCalMonitor  --monitor_name CMSHGCalMonitor --config_file user/cmshgcal/conf/onlinemon.conf --reduce 10 --reset -r tcp://$HOSTIP:$RCPORT |tee -a logs/mon.log ; read' &

#################  VME Producers on pcminn03 #####################
printf '\033[22;33m\t killing all EUDAQ  processes on pcminn03 \033[0m \n'
ssh -Y -T wc "/home/cmsdaq/DAQ/eudaq_v2/KILLRUN.local"  &
sleep 3

printf '\033[22;33m\t Starting the DWC Producer on pcminn03 \033[0m \n'
flog="logs/Run${NEWRUNNUM}_DWC_Producer_$dt.log"
nohup ssh -Y -T wc "/home/cmsdaq/DAQ/eudaq_v2/bin/euCliProducer -n CMSHGCal_DWC_Producer -t cms-hgcal-dwc -r tcp://${HOSTIP}:${RCPORT};"  > $flog 2>&1 &
sleep 2

printf '\033[22;33m\t Starting the Digitizer Producer on pcminn03 \033[0m \n'
flog="logs/Run${NEWRUNNUM}_Digitizer_Producer_$dt.log"
nohup ssh -Y -T wc "/home/cmsdaq/DAQ/eudaq_v2/bin/euCliProducer -n CMSHGCal_MCP_Producer -t cms-hgcal-mcp -r tcp://${HOSTIP}:${RCPORT}"  > $flog 2>&1 &


