#! /usr/bin/env python
##
## code testing of class SlowControl
##
import SlowControl # slow control code

m = SlowControl.SlowControl(0) # HLVDS FEC (master)
s = SlowControl.SlowControl(1) # HLVDS ADC (slave)

clk_high = 0x1
clk_low  = 0x3

readout_start = 0x674
readout_end   = readout_start + (60*60+90*90)*2*(clk_high+clk_low)-1  # two memories per pixel, every fourth clock cycle is readout

# 0x16 (22) readout control
#
#  3: driver loop mode (activates the complete sequence although no data is sent)
#  2: single-event readout enable
#  1: single-event readout request (rising edge sensitive, need to toggle!)
#  0: continuous readout enable
rdo_settings = 0x0 # 0x8

# 0x18 (24) trigger control
#
# 18-12: tlu wait cycles (default = 0x0c <<  4)
# 10- 4: tlu clock div   (default = 0x00 << 12)
#     3: driver busy enable ( 0x8 )
#     2: tlu reset enable   ( 0x4 )
#  1- 0: trig mode  (0b00 = auto/continuously, 0b01 = NIM in, 0b10 = TLU triggering
trg_settings = 0x2 | (0x0c << 4) | (0x00 << 12)

### FEC HLVDS configuration registers
values = [
    # explorer driver configuration
    0x46,          # 0x00 ( 0) reset status
    0x0,           # 0x01 ( 1) pixreset_n start
    0x66,          # 0x02 ( 2) pixreset_n end
    0x80,          # 0x03 ( 3) store1_n   start
    0x100,         # 0x04 ( 4) store1_n   end
    0x580,         # 0x05 ( 5) store2_n   start
    0x600,         # 0x06 ( 6) store2_n   end
    0x666,         # 0x07 ( 7) start      start
    readout_end,   # 0x08 ( 8) start      end
    0x0,           # 0x09 ( 9) pulse      start
    0x0,           # 0x0a (10) pulse      end
    0x646,         # 0x0b (11) trig decision
    readout_start, # 0x0c (12) readout    start
    readout_end,   # 0x0d (13) readout    end
    0x1,           # 0x0e (14) readout    steps
    0x1,           # 0x0f (15) dut_clk_en start
    0x1,           # 0x10 (16) dut_clk_en end
    clk_high,      # 0x11 (17) dut_clk_high
    clk_low,       # 0x12 (18) dut_clk_low
    0x667,         # 0x13 (19) dut_clk_ref
    0x0,           # 0x14 (20) cnt_reload_val
    readout_end,   # 0x15 (21) cnt_return_val
    # general configuration
    rdo_settings,  # 0x16 (22) readout control
    8832,          # 0x17 (23) maximum frame size
    trg_settings,  # 0x18 (24) trigger control
    0x1            # 0x19 (25) operation mode (Master = 0x1 / Standalone 0x0)
]

# all higher addresses are read-only
SlowControl.write_burst(m, 6039, 0x0, values, False)

### FEC ADC
SlowControl.write_burst(s, 6039, 0x0, [clk_high+clk_low], False)

quit()
