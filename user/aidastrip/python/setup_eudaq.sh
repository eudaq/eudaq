#!/bin/bash     

export EUDAQ_DIR=/opt/eudaq2/
export PYTHONPATH=${EUDAQ_DIR}/lib:${PYTHONPATH}

export LD_LIBRARY_PATH=${EUDAQ_DIR}/lib:${LD_LIBRARY_PATH}

#export ROGUE_DIR=${PWD}/rogue/
#export PYTHONPATH=${EUDAQ_DIR}/python:${ROGUE_DIR}/python:${PYTHONPATH}
