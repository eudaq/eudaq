#!/bin/bash
#
# store the git version information of the pALPIDEfs driver libary and EuDAQ
#

LOG_FILE_PREFIX=$1
SEARCH_DIR=$2

for str in $(find ${SEARCH_DIR} -name flags.make -exec cat {} \; | grep pALPIDEfs-software)
do
    if [[ $str == *pALPIDEfs-software* ]]
    then
        path=${str:2}
    fi
done

cd ${path}
git status > ${LOG_FILE_PREFIX}_driver_status
git diff   > ${LOG_FILE_PREFIX}_driver_diff
git log -1 > ${LOG_FILE_PREFIX}_driver_log
cd ${SEARCH_DIR}
git status > ${LOG_FILE_PREFIX}_eudaq_status
git diff   > ${LOG_FILE_PREFIX}_eudaq_diff
git log -1 > ${LOG_FILE_PREFIX}_eudaq_log
