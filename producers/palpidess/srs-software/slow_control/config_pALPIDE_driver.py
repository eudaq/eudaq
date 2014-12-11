#! /usr/bin/env python
##
## standard configuration of the FEC HLVDS for the use
## with the pALPIDE with the Padua Proximity board V1
##
import sys
import os
import SlowControl # slow control code
import biasDAC     # special code to setup up the voltage biases

m = SlowControl.SlowControl(0) # HLVDS FEC (master)

# was the integration time given as a commandline argument?
if len(sys.argv) == 2:
    integration_time = int(float(sys.argv[1])/0.00625)
    # Acquisition time is expected to specified in micro seconds (us)
else:
    integration_time = 0xf6               # default value

# 0x16 (22) readout control
#
# The bits (2:0) are not set during configuration, as they are set during the
# (start-up of a) measurement.
#
#  3: driver loop mode (activates the complete sequence although no data is sent)
#  2: single-event readout enable
#  1: single-event readout request (rising edge sensitive, need to toggle!)
#  0: continuous readout enable
rdo_settings = 0x0

# 0x18 (24) trigger control
#
# 18-12: tlu wait cycles (default = 0x0c <<  4)
# 10- 4: tlu clock div   (default = 0x00 << 12)
#     3: driver busy enable ( 0x8 )
#     2: tlu reset enable   ( 0x4 )
#  1- 0: trig mode  (0b00 = auto/continuously, 0b01 = NIM in,
#                    0b10 = TLU triggering, 0b11 valid based)
trg_settings = 0x1 | (0x0c << 4) | (0x00 << 12)

### FEC HLVDS configuration registers
# all times/delays are specified in multiples of 6.25 ns
values_HLVDS = [
    # explorer driver configuration
    0b1100,           # 0x00 ( 0) enable digital pulsing (0), clock init state (1),
                      #           repeat global reset (2), activate mem_wr_en (3)
    0x10,             # 0x01 ( 1) length of GRSTb signal [init_dly_t]
    0xa0,             # 0x02 ( 2) length of the analog pulser reference pulse [dly_t]
    0x0,              # 0x03 ( 3) pre-acqusition delay (in between trigger and acqs start)
                      #           [pre_acq_dly_t]
    integration_time, # 0x04 ( 4) integration time [acq_time_t]
    0x0,              # 0x05 ( 5) delay in between acquisition and readout [rdo_dly_t]
    0xb,              # 0x06 ( 6) transport delay of the signals from SRS -> pALPIDE -> SRS
                      #           [transport_dly_t]
    0x1,              # 0x07 ( 7) clock divider for the output clock [clk_div_t]
    0x3,              # 0x08 ( 8) readout frequency divider [rdo_div_t]
    0x0,              # 0x09 ( 9) post-event delay [post_evt_dly_t]
    0b0111111100000,  # 0x0a (10) global reset settings by state (0 = active, 1 = inactive)
    # .*=-.*=-.*=-.   #           stReset (0), stInit (1), stDly (2), stTrigWait (3), stPreAcq (4),
                      #           stAcq (5), stAcqWait (6), stRdoWait (7), stRdoStart (8),
                      #           stRdoFirst (9), stRdoPause (10), stRdo (11), stPostEvtDly (12)
    0x2,              # 0x0b (11) nim_out signal assignment (0 = off, 1 = on)
                      #           a_pulse_ref (0), combined_busy (1)
    0x0,              # 0x0c (12)
    0x0,              # 0x0d (13)
    0x0,              # 0x0e (14)
    0x0,              # 0x0f (15)
    0x0,              # 0x10 (16)
    0x0,              # 0x11 (17)
    0x0,              # 0x12 (18)
    0x0,              # 0x13 (19)
    0x0,              # 0x14 (20)
    0x0,              # 0x15 (21)
    # general configuration
    rdo_settings,     # 0x16 (22) readout control
    8832,             # 0x17 (23) maximum frame size
    trg_settings      # 0x18 (24) trigger control
]

# all higher addresses are read-only
SlowControl.write_burst(m, 6039, 0x0, values_HLVDS, False)

biasDAC.set_bias_voltage(12, 1.6, m) # Vreset
biasDAC.set_bias_voltage(8,  0.4, m) # VCASN
biasDAC.set_bias_voltage(10,  0.6, m) # VCASP

quit()
