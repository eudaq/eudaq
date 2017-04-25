#! /usr/bin/env python
##
## code testing of class SlowControl
##
import SlowControl # slow control code
import time

sc = SlowControl.SlowControl(1)

SlowControl.write_list(sc, 6007, [ 0xFFFFFFFF ], [ 0xFFFF8000 ], False, False)

quit()
