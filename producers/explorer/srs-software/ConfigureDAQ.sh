#!/bin/bash
#
#
#

echo "Configuring DAQ for Single Event readout and TLU triggering"

#${PATHSRS}/slow_control/reset_FEC_HLVDS.py
#${PATHSRS}/slow_control/reset_FEC_ADC.py
#sleep 10
${PATHSRS}/slow_control/tlu_triggering.py
${PATHSRS}/slow_control/prepare_single_acq_explorer1.py
echo ""
