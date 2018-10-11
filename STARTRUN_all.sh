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



#################  Data collector #################
xterm -r -sb -sl 100000 -T "EventnumberSyncData collector" -e 'bin/euCliCollector -n EventnumberSyncDataCollector -t dc1 -r tcp://${HOSTIP}:${RCPORT}; read' &
#xterm -r -sb -sl 100000 -T "EventnumberSyncData collector" -e 'bin/euCliCollector -n EventnumberSyncDataCollector -t dc2 -r tcp://${HOSTIP}:${RCPORT} -a tcp://45001; read' &

#AHCAL test data collector
#xterm -r -sb -sl 100000 -T "directsave data collector" -e 'bin/euCliCollector -n DirectSaveDataCollector -t dc2 -r tcp://${HOSTIP}:${RCPORT}; read' &

#################  Producer #################
#gnome-terminal --geometry=80x600-280-900 -t "CMS HGCal Producer" -e "bash -c \"source ../setup_eudaq_cmshgcal.sh; ./HGCalProducer -r tcp://$HOSTIP:$RCPORT\" " &
#xterm -r -sb -sl 100000 -geometry 160x30 -T "Ex0-Producer" -e 'bin/euCliProducer -n Ex0Producer -t exo -r tcp://$HOSTIP:$RCPORT |tee -a logs/ex0.log ; read'&
xterm -r -sb -sl 100000 -geometry 160x30 -T "CMS-HGCAL0" -e 'bin/euCliProducer -n HGCalProducer -t cms-hgcal0 -r tcp://$HOSTIP:$RCPORT '&
nohup ./bin/euCliProducer -n HGCalProducer -t cms-hgcal1 -r tcp://$HOSTIP:$RCPORT&
nohup ./bin/euCliProducer -n HGCalProducer -t cms-hgcal2 -r tcp://$HOSTIP:$RCPORT&
nohup ./bin/euCliProducer -n HGCalProducer -t cms-hgcal3 -r tcp://$HOSTIP:$RCPORT&
nohup ./bin/euCliProducer -n HGCalProducer -t cms-hgcal4 -r tcp://$HOSTIP:$RCPORT&
nohup ./bin/euCliProducer -n HGCalProducer -t cms-hgcal5 -r tcp://$HOSTIP:$RCPORT&
nohup ./bin/euCliProducer -n HGCalProducer -t cms-hgcal6 -r tcp://$HOSTIP:$RCPORT&
nohup ./bin/euCliProducer -n HGCalProducer -t cms-hgcal7 -r tcp://$HOSTIP:$RCPORT&
nohup ./bin/euCliProducer -n HGCalProducer -t cms-hgcal8 -r tcp://$HOSTIP:$RCPORT&
nohup ./bin/euCliProducer -n HGCalProducer -t cms-hgcal9 -r tcp://$HOSTIP:$RCPORT&
nohup ./bin/euCliProducer -n HGCalProducer -t cms-hgcal10 -r tcp://$HOSTIP:$RCPORT&
nohup ./bin/euCliProducer -n HGCalProducer -t cms-hgcal11 -r tcp://$HOSTIP:$RCPORT&
nohup ./bin/euCliProducer -n HGCalProducer -t cms-hgcal12 -r tcp://$HOSTIP:$RCPORT&
nohup ./bin/euCliProducer -n HGCalProducer -t cms-hgcal13 -r tcp://$HOSTIP:$RCPORT&

################## AHCAL Producer #################
#xterm -r -sb -sl 100000 -geometry 160x30 -T "AHCAL1" -e 'bin/euCliProducer -n AHCALProducer -t AHCAL1 -r tcp://$HOSTIP:$RCPORT | tee -a logs/AHCALProducer.log'&

################## OnlineMonitor ##################
xterm -r -sb -sl 100000 -geometry 200x40+300+750 -T "OnlineMon" -e 'bin/CMSHGCalMonitor  --monitor_name CMSHGCalMonitor --config_file user/cmshgcal/conf/onlinemon.conf --reduce 1 --reset -r tcp://$HOSTIP:$RCPORT  --root |tee -a logs/mon.log ; read' & #adding --root 1 may be needed for saving dqm files
# xterm -r -sb -sl 100000 -T "OnlineMon" -e 'bin/CMSHGCalMonitor  --monitor_name CMSHGCalMonitor --reset -r tcp://$HODTIP:$RCPORT |tee -a logs/mon.log ; read' &
#xterm -r -sb -sl 100000 -T "OnlineMon" -e './bin/euCliMonitor -n Ex0Monitor -t StdEventMonitor  -a tcp://45001 '&


###############  VME Producers on pcminn03 #####################
#printf '\033[22;33m\t killing all EUDAQ  processes on pcminn03 \033[0m \n'
#ssh -Y -T wc "/home/cmsdaq/DAQ/eudaq_v2/KILLRUN.local"  &
#sleep 3
#
#printf '\033[22;33m\t Starting the DWC Producer on pcminn03 \033[0m \n'
#flog="logs/Run${NEWRUNNUM}_DWC_Producer_$dt.log"
#nohup ssh -Y -T wc "/home/cmsdaq/DAQ/eudaq_v2/bin/euCliProducer -n CMSHGCal_DWC_Producer -t cms-hgcal-dwc -r tcp://${HOSTIP}:${RCPORT};"  > $flog 2>&1 &
#sleep 2

#printf '\033[22;33m\t Starting the Digitizer Producer on pcminn03 \033[0m \n'
#flog="logs/Run${NEWRUNNUM}_Digitizer_Producer_$dt.log"
#nohup ssh -Y -T wc "/home/cmsdaq/DAQ/eudaq_v2/bin/euCliProducer -n CMSHGCal_MCP_Producer -t cms-hgcal-mcp -r tcp://${HOSTIP}:${RCPORT}"  > $flog 2>&1 &
