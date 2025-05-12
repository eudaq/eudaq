# Define port
RPCPORT=44000
RUNCONTROLIP=127.0.0.1 
#192.168.24.1

# EUDAQDIR needs to be set independently. Then, add it to pythonpath if it's not in there already
test "${PYTHONPATH#*"${EUDAQDIR}/lib"}" != "$PYTHONPATH" || export PYTHONPATH="${EUDAQDIR}/lib:$PYTHONPATH"

# start the python procuder
python ${EUDAQDIR}/user/sourcemeter/python/SourcemeterPyProducer.py -r tcp://${RUNCONTROLIP}:${RPCPORT}

