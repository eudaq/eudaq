# IP-environment variables are set by user/eudet/misc/environments/setup_eudaq2_aida-tlu.sh 
# Define port
RPCPORT=44000

# Start Run Control
xterm -T "Run Control" -e 'euRun -n kpixRunControl' &
sleep 2 

# Start Logger
xterm -T "Log Collector" -e 'euLog -r tcp://${RUNCONTROLIP}' &
sleep 1

# Start one DataCollector
# name (-t) in conf file
xterm -T "Data Collector TLU" -e 'euCliCollector -n TriggerIDSyncDataCollector -t one_dc -r tcp://${RUNCONTROLIP}:${RPCPORT}' &
sleep 1

# Start TLU Producer
xterm -T "AidaTluProducer" -e 'euCliProducer -n AidaTluProducer -t aida_tlu -r tcp://${RUNCONTROLIP}:${RPCPORT}' & 

# Start Kpix DataCollector
xterm -T "Data Collector KPiX" -e 'euCliCollector -n kpixDataCollector -t kpixDC -r tcp://${RUNCONTROLIP}:${RPCPORT}'&
sleep 1

# Start Kpix Producer
python3 ${EUDAQ}/user/aidastrip/python/kpixPyProducer.py &
sleep 1

# Start PI Stage
xterm -T "PI Stage" -e 'euCliProducer -n PIStageProducer -t Stage ' &
sleep 1

# OnlineMonitor
xterm -T "Online Monitor" -e 'StdEventMonitor -t StdEventMonitor -r tcp://${RUNCONTROLIP}' & 
