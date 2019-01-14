#!/usr/bin/env sh                                                                                         

#export EUDAQ_DIR=/opt/eudaq2/
export EUDAQ_DIR=${PWD}/../../..
export PYTHONPATH=${EUDAQ_DIR}/lib:${PYTHONPATH}

export LD_LIBRARY_PATH=${EUDAQ_DIR}/lib:${LD_LIBRARY_PATH}
