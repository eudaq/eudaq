# Define port
RPCPORT=44000
RUNCONTROLIP=192.168.24.1

# start the python procuder 
xterm -T "sourcemeter Producer" -e 'python ${EUDAQDIR}/user/sourcemeter/python SourcemeterPyProducer.py-r tcp://${RUNCONTROLIP}:${RPCPORT}' &

