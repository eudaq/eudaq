#!/bin/bash
#
#
#
# setup environment


#DATE=`date +%Y-%m-%d_%H-%M`
RUN=`expr $(<../data/runnumber.dat) + 1`
echo "$RUN"
DATA=../data/SRS/run$RUN
echo "PATHSRS=$PATHSRS"
###############################################################################
### execution

# check whether the ADC DCM is locked
$PATHSRS/slow_control/check_ADC.py

# create output folder
mkdir -p $DATA
if [ ! -d $DATA ]
then
    echo "ERROR: could not create output folder"
fi

# check whether FEC 0 is available
ping -c 1 10.0.0.2
if [ "$?" -ne 0 ]
then
    echo "ERROR: could not reach FEC 0 (FEC_HLVDS)"
    exit 2
fi

# check whether FEC 1 is available
ping -c 1 10.0.1.2
if [ "$?" -ne 0 ]
then
    echo "ERROR: could not reach FEC 1 (FEC_ADC)"
    exit 2
fi

echo ""
echo ""

# determine GIT version
echo "Determine firmware versions of the FECs"
$PATHSRS/slow_control/determine_firmware_version.py 0 > $DATA/fec_0_firmware.txt
$PATHSRS/slow_control/determine_firmware_version.py 1 > $DATA/fec_1_firmware.txt
echo ""

# configure the SRS
echo "Configuring the SRS"
$PATHSRS/slow_control/config_explorer_driver.py
$PATHSRS/slow_control/setup_ADC.py
#$PATHSRS/slow_control/shutdown_ADC.py

# determine configuration
echo "Dump the application registers of the FECs"
$PATHSRS/slow_control/read_explorer1_config.py > $DATA/configuration.txt

# setup the reset voltage
#slow_control/setup_DAC.sh # TODO replace this by a improved script
RESET_CHANS=( 3 6 7 11 )
RESET_VOLTAGE=0.7

for i in ${RESET_CHANS[@]}
do
    $PATHSRS/slow_control/biasDAC.py $i $RESET_VOLTAGE
done


# setup the bias voltage for the op amps
BIAS_CHANS=( 0 1 )
BIAS_VOLTAGE=1.8

for i in ${BIAS_CHANS[@]}
do
    $PATHSRS/slow_control/biasDAC.py $i $BIAS_VOLTAGE
done

${PATHSRS}/slow_control/reset_FEC_HLVDS.py
${PATHSRS}/slow_control/reset_FEC_ADC.py
sleep 10
echo "Configuration done"
