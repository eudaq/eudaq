#!/bin/sh

# determine dir where the env script is located
DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

export PYTHONPATH=${DIR}/slow_control:${DIR}/common

export PALPIDESS_SCRIPTS=${DIR}
export EUDAQ_DIR=$(readlink -f ${PALPIDESS_SCRIPTS}/../../../)
export PALPIDESS_LOGS=${EUDAQ_DIR}/logs/palpidess
mkdir -p ${PALPIDESS_LOGS}
echo "PALPIDESS_SCRIPTS=${PALPIDESS_SCRIPTS}"
echo "EUDAQ_DIR=${EUDAQ_DIR}"
echo "PALPIDESS_LOGS=${PALPIDESS_LOGS}"

export LOG_PREFIX=$(date +%s)

# EUDAQ specific variables
#export RCPORT=44000
#export HOSTIP=pcaliceitsup3

xterm -sb -sl 1000 -geom 80x20 -fn fixed -T 'pALPIDEss Producer' -e 'unbuffer ${EUDAQ_DIR}/bin/PalpidessProducer.exe -r tcp://${HOSTIP}:${RCPORT} 2>&1 | tee -a ${PALPIDESS_LOGS}/${LOG_PREFIX}' &
