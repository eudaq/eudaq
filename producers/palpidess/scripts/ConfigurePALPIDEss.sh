#!/bin/bash
#
#
#
# setup environment


echo "EUDAQ_DIR=${EUDAQ_DIR}"
echo "PALPIDESS_LOGS=${PALPIDESS_LOGS}"
echo "PALPIDESS_SCRIPTS=${PALPIDESS_SCRIPTS}"

###############################################################################
### check command line arguments
#
# 1: Vreset
# 2: Vcasn
# 3: Vcasp
# 4: Ithr
# 5: Vlight
# 6: AcqTime
# 7: Vbb
# 8: TrigDelay
if [ "$#" -ne 8 ]
then
    echo "./ConfigurePALPIDEss.sh <Vreset> <Vcasn> <Vcasp> <Ithr> <Vlight> \\"
    echo "                        <AcqTime> <Vbb> <TrigDelay>"
    exit 1
fi
V_RST=$1
V_CASN=$2
V_CASP=$3
I_THR=$4
V_LIGHT=$5
T_ACQ=$6
V_BB=$7
TRIG_DELAY=$8
echo "V_RST=${V_RST}"
echo "V_CASN=${V_CASN}"
echo "V_CASP=${V_CASP}"
echo "I_THR=${I_THR}"
echo "V_LIGHT=${V_LIGHT}"
echo "T_ACQ=${T_ACQ}"
echo "V_BB=${V_BB}"
echo "TRIG_DELAY=${TRIG_DELAY}"

###############################################################################

# list of threshold currents (2.05uA = 0.98V, 1.02uA = 1.155V)
I_THR_LIST=( 0.98 1.155 1.265 1.35 )
declare -A I_THR_NAMES_LIST=( ["0.98"]=2.05 ["1.155"]=1.02 ["1.265"]=0.6 ["1.35"]=0.3 )
V_I_THR=${I_THR_NAMES_LIST["${I_THR}"]}
echo "V_I_THR=${V_I_THR}"

###############################################################################
### execution

# create output folder
mkdir -p ${PALPIDESS_LOGS}
if [ ! -d ${PALPIDESS_LOGS} ]
then
    echo "ERROR: could not create output folder"
    exit 2
fi

# TODO do we want to program the FPGA firmware here?

# check whether FEC 0 is available
ping -c 1 10.0.0.2
if [ "$?" -ne 0 ]
then
    echo "ERROR: could not reach FEC 0 (FEC_HLVDS)"
    exit 3
fi

echo ""
echo ""

# determine GIT version
echo "Determine firmware versions of the FECs"
${PALPIDESS_SCRIPTS}/slow_control/determine_firmware_version.py 0 > ${PALPIDESS_LOGS}/fec_0_firmware.txt
echo ""

# configure the SRS
echo "Configuring the SRS"
${PALPIDESS_SCRIPTS}/slow_control/config_pALPIDE_driver.py


# TODO handle the trigger delay

# determine configuration
echo "Dump the application registers of the FECs"
${PALPIDESS_SCRIPTS}/slow_control/read_pALPIDE_config.py > ${PALPIDESS_LOGS}/configuration.txt

# power on the proximity board
if ${PALPIDESS_SCRIPTS}/power_on.py
then
    exit 4
fi

# set back-bias voltage
if ${PALPIDESS_SCRIPTS}/change_back_bias.py ${V_BB}
then
    exit 5
fi

# set light voltage
if ${PATHSRS/change_light.py ${V_LIGHT}
then
    exit 6
fi

# Vreset
echo "V_RST=${V_RST}"
${PALPIDESS_SCRIPTS}/slow_control/biasDAC.py 12 ${V_RST}

# Vcasn
echo "V_CASN=${V_CASN}"
${PALPIDESS_SCRIPTS}/slow_control/biasDAC.py 8 ${V_CASN}

# Vcasp
echo "V_CASP=${V_CASP}"
${PALPIDESS_SCRIPTS}/slow_control/biasDAC.py 10 ${V_CASP}

# Ithr
echo "I_THR=${I_THR} / V_I_THR=${V_I_THR}"
${PALPIDESS_SCRIPTS}/slow_control/biasDAC.py 4 ${V_I_THR}

# TODO - can/should we reset?
${PALPIDESS_SCRIPTS}/slow_control/reset_FEC_HLVDS.py
sleep 10
echo "Configuration done"
