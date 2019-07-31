# IP-environment variables are set by user/eudet/misc/environments/setup_eudaq2_aida-tlu.sh 
# Define port
RPCPORT=44000

# Start Run Control
xterm -T "Run Control" -e 'euRun' &
sleep 2 

# Start Logger
#xterm -T "Log Collector" -e 'euLog -r tcp://${RUNCONTROLIP}' &
#sleep 1

# Start TLU Producer
xterm -T "AidaTluProducer" -e 'euCliProducer -n AidaTluProducer -t aida_tlu -r tcp://${RUNCONTROLIP}:${RPCPORT}' & 
#sleep 1 

# Start Wiener Producer
#xterm -T "WienerProducer" -e 'euCliProducer -n WienerProducer -t wiener -r tcp://${RUNCONTROLIP}'

# OnlineMonitor # if you will have kpix data to connect to
#xterm -T "Online Monitor" -e 'StdEventMonitor -t StdEventMonitor -r tcp://${RUNCONTROLIP}' & 

