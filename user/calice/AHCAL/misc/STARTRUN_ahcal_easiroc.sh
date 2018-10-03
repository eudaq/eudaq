#!/bin/bash

if [ -f KILLRUN.local ]
then
    sh KILLRUN.local
else
    sh KILLRUN
fi

export RCPORT=44000
export HOSTIP=127.0.0.1
#################  Run control ###################
#xterm -sb -sl 1000000 -T "Runcontrol" -e 'bin/euRun -n RunControl -a tcp://$RCPORT ; read '&
xterm -sb -sl 1000000 -T "Runcontrol" -e 'bin/euRun -n AhcalRunControl -a tcp://$RCPORT ; read '&
sleep 1  
#################  Log collector #################
xterm -sb -sl 1000 -geometry 160x30 -T "Logger" -e 'bin/euLog ; read' &
#sleep 1
#################  Data collector #################
xterm -sb -sl 100000 -T "directsave Data collector" -e 'bin/euCliCollector -n DirectSaveDataCollector -t dc1 ; read' &
xterm -sb -sl 100000 -geometry 160x30 -T "Hodoscope Data collector" -e 'bin/euCliCollector -n AhcalHodoscopeDataCollector -t dc2 | tee -a logs/dc2.log; read' &
#xterm -sb -sl 100000 -geometry 160x30 -T "slcio Hodoscope Data collector" -e 'bin/euCliCollector -n AhcalHodoscopeDataCollector -t dc3 | tee -a logs/dc3.log; read' &
#xterm -sb -sl 100000  -T "Data collector2" -e 'bin/euCliCollector -n DirectSaveDataCollector -t dc2 ; read' &
#sleep 1

#################  Producer #################
xterm -sb -sl 1000000 -geometry 160x30 -T "Hodoscope" -e 'bin/euCliProducer -n CaliceEasirocProducer -t hodoscope1 -r tcp://$HOSTIP:$RCPOR|tee -a logs/hodoscope1.log && read || read'&
xterm -sb -sl 1000000 -geometry 160x30 -T "Hodoscope 2" -e 'bin/euCliProducer -n CaliceEasirocProducer -t hodoscope2 -r tcp://$HOSTIP:$RCPOR|tee -a logs/hodoscope2.log && read || read'&
# sleep 1
xterm -sb -sl 1000000 -geometry 160x30 -T "AHCAL" -e 'bin/euCliProducer -n AHCALProducer -t Calice1 -r tcp://$HOSTIP:$RCPOR|tee -a logs/ahcal.log && read || read'&

#################  Online Monitor #################
#echo "starting online monitor"
xterm -sb -sl 100000  -T "Online monitor" -e 'bin/StdEventMonitor -c conf/onlinemonitor.conf --monitor_name StdEventMonitor --reset -r tcp://$HOSTIP:$RCPORT ; read' &
# echo "online monitor started"
exit

[ "$1" != "" ] && RCPORT=$1

export TLUIP=127.0.0.1
export HOSTNAME=127.0.0.1

cd `dirname $0`
if [ -z "$LD_LIBRARY_PATH" ]; then
  export LD_LIBRARY_PATH="`pwd`/lib"
else
  export LD_LIBRARY_PATH="`pwd`/lib:$LD_LIBRARY_PATH"
fi

printf '\033[1;32;48m \t STARTING DAQ LOCALLY\033[0m \n'
echo $(date)
printf '\033[22;33m\t Cleaning up first...  \033[0m \n'

if [ -f KILLRUN.local ]
then
    sh KILLRUN.local
else
    sh KILLRUN
fi

printf '\033[22;31m\t End of killall \033[0m \n'

sleep 1

######################################################################
if [ -n "`ls data/run*.raw`" ]
then
    printf '\033[22;33m\t Making sure all data files are properly writeprotected \033[0m \n'
    chmod a=r data/run*.raw
    printf '\033[22;32m\t ...Done!\033[0m \n'
fi

cd bin
#=====================================================================
printf '\033[22;33m\t Starting Subprocesses \033[0m \n'
#=====================================================================

######################################################################
# euRun
###############
printf '\033[22;33m\t RunControl \033[0m \n'
xterm -sb -sl 1000000 -T "Runcontrol" -e './euRun -a tcp://$RCPORT || read '&
sleep 2

######################################################################
# euLog
###############
#printf '\033[22;33m\t Logger  \033[0m \n'
xterm -sb -sl 1000 -T "DataLogger" -e './euLog -r tcp://$HOSTIP:$RCPORT || read' &
sleep 2
#sleep 10

######################################################################
# DataCollector
###############
#printf '\033[22;33m\t TestDataCollector \033[0m \n'
#xterm -sb -sl 100000  -T "direct save Data Collector" -e './Launcher -n DirectSaveDataCollector -t d1 -r tcp://$HOSTIP:$RCPORT && read || read' &
#sleep 2

#xterm -sb -sl 100000 -rv -T "event number Data Collector" -e './Launcher -n EventnumberSyncDataCollector -t d2 -r tcp://$HOSTIP:$RCPORT && read || read' &
#sleep 2

#xterm -sb -sl 100000 -rv -T "bif Data Collector" -e './Launcher -n DirectSaveDataCollector -t d3 -r tcp://$HOSTIP:$RCPORT && read || read' &
#sleep 2

xterm -sb -sl 100000  -T "Event Building Data Collector" -e './Launcher -n CaliceAhcalBifEventBuildingDataCollector -t d4 -r tcp://$HOSTIP:$RCPORT && read || read' &
sleep 2

#xterm -sb -sl 1000 -geom 80x10-480-900 -fn fixed -T "Data Collector" -e './TestDataCollector -r tcp://$HOSTIP:$RCPORT' &
#sleep 2


######################################################################
# AHCAL producer
################
#xterm -sb -sl 1000 -T "AHCAL" -e './AHCALProducer -r tcp://$HOSTIP:$RCPOR || read'&
xterm -sb -sl 1000000 -geometry 160x30 -T "AHCAL" -e './AHCALProducer -r tcp://$HOSTIP:$RCPOR|tee -a ../logs/ahcal.log && read || read'&
sleep 2

####################################################################
# AHCAL BIF
####################################
xterm -sb -sl 1000000 -geometry 160x30 -T "BIF" -e './BIFAHCALProducer -r tcp://$HOSTIP:$RCPOR|tee -a ../logs/bif.log && read || read'&
sleep 2

printf ' \n'
printf ' \n'
printf ' \n'
printf '\033[1;32;48m\t ...Done!\033[0m \n'
printf '\033[1;32;48mSTART OF DAQ COMPLETE\033[0m \n'
