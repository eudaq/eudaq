# IP-environment variables are set by user/eudet/misc/environments/setup_eudaq2_aida-tlu.sh 
# Define port
RPCPORT=44000
RUNCONTROLIP="192.168.21.1"

printf '\033[1;32;48m \t STARTING DAQ \033[0m \n'
echo $(date)
printf '\033[22;33m\t Cleaning up first...  \033[0m \n'

sh KILLRUN

printf '\033[22;31m\t End of killall \033[0m \n'


# Start Run Control
xterm -T "Run Control" -e 'euRun' &
sleep 2 

# Start Logger
xterm -T "Log Collector" -e 'euLog -r tcp://${RUNCONTROLIP}' &
sleep 1

# Start TLU Producer
xterm -T "EUDET TLU Producer" -e 'euCliProducer -n EudetTluProducer -t eudet_tlu -r tcp://${RUNCONTROLIP}:${RPCPORT}' & 
#xterm -T "AIDA TLU Producer" -e 'euCliProducer -n AidaTluProducer -t aida_tlu -r tcp://${RUNCONTROLIP}:${RPCPORT}' & 

sleep 1

xterm -T "CMSPixel Producer" -e 'euCliProducer -n CMSPixelProducer -t CMSPixelREF -r tcp://${RUNCONTROLIP}:${RPCPORT}' & 

# Start NI Producer locally connect to LV via TCP/IP
xterm -T "NI/Mimosa Producer" -e 'euCliProducer -n NiProducer -t ni_mimosa -r tcp://${NIIP}:${RPCPORT}' &
sleep 1

# Start one DataCollector
# name (-t) in conf file
# or: -n TriggerIDSyncDataCollecor

#xterm -T "Data Collector TLU" -e 'euCliCollector -n TriggerIDSyncDataCollector -t the_dc -r tcp://${RUNCONTROLIP}:${RPCPORT}' &
xterm -T "Data Collector TLU" -e 'euCliCollector -n EventIDSyncDataCollector -t the_dc -r tcp://${RUNCONTROLIP}:${RPCPORT}' &

#xterm -T "Data Collector TLU" -e 'euCliCollector -n DirectSaveDataCollector -t m26_dc -r tcp://${RUNCONTROLIP}:${RPCPORT}' &
#xterm -T "Data Collector TLU" -e 'euCliCollector -n DirectSaveDataCollector -t tlu_dc -r tcp://${RUNCONTROLIP}:${RPCPORT}' &
#xterm -T "Data Collector TLU" -e 'euCliCollector -n DirectSaveDataCollector -t cmspixel_dc -r tcp://${RUNCONTROLIP}:${RPCPORT}' &

#xterm -T "Online Monitor" -e 'StdEventMonitor -r tcp://${RUNCONTROLIP}:${RPCPORT}' & 

