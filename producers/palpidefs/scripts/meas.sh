#!/bin/bash

SCRIPT_PATH=$1
MEAS_FILE=$2
LOG_FILE=$3
PID_FILE=$4

${SCRIPT_PATH}/meas.py ${MEAS_FILE} > ${LOG_FILE} 2>&1 &
echo $! > ${PID_FILE}
