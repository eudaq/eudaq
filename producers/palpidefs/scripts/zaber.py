#!/usr/bin/env python

import sys
import serial
import array
import time

class ZaberConnection:
    def __init__(self,port):
        self.link=serial.Serial(port,9600)
    def init(self):
        self.send(0,2,0) # renumber
        time.sleep(2)
    def send(self,id,cmd,data):
        self.link.flushInput()
        self.link.write(array.array('B',[id,cmd,data&0xFF,data>>8&0xFF,data>>16&0xFF,data>>24&0xFF]).tostring())
        res=array.array('B',self.link.read(6))
        return res[5]<<24|res[4]<<16|res[3]<<8|res[2]

class Zaber:
    def __init__(self,connection,id):
        self.connection = connection
        self.id = id

    def init(self):
        #self.connection.send(self.id, 0, 0)     # reset the device
        time.sleep(1) # wait
        mode = (0x1 << 15) | (0x1 << 14)        # deactivate the LEDs
        self.connection.send(self.id, 40, mode) # set device mode
        #self.home()                             # move to home position

    def home(self):
        self.connection.send(self.id, 1, 0)     # move to home position

    def move_abs(self,pos):
        return self.connection.send(self.id,20,pos)

    def move_rel(self,pos):
        return self.connection.send(self.id,21,pos)

    def getpos(self):
        return self.connection.send(self.id,60,0)

    def set_speed(self):
        return self.connection.send(self.id,42, 1075) # set the zaber moving speed of 1mm/s 

def main():
    # set up the device
    # zaber.py <device> <id> <mode> <value>
    dev=sys.argv[1]
    id=int(sys.argv[2])
    mode=int(sys.argv[3]) if len(sys.argv)>=4 else -1;

    con=ZaberConnection(dev)
    stage=Zaber(con, id)

    # step size for stages
    # linear stage: 1 step = 0.09921875 um
    # rotary stage: 1 step = 4.091 urad

    print 'arguments:'
    print sys.argv

    # switch mode
    if mode==0: # initialise
        con.init()
        stage.init()
    elif mode==1: # move to home position
        stage.home()
    elif mode==2: # get current position in steps
        #stage.getpos()
        print stage.getpos()
    elif mode==3: # move to absolute positon in steps
        stage.move_abs(int(sys.argv[4]))
        # moved successfully?
        if (int(stage.getpos())-int(sys.argv[4]))!=0:
            return 1
    elif mode==4: # move relatively in steps
        stage.move_rel(int(sys.argv[4]))

    elif mode==5: # move in milimeters
        stage.set_speed()
        stage.move_abs(int(float(sys.argv[4])/1e-3/0.09921875))
        # moved successfully?
        if abs(float(int(stage.getpos())*1e-3*0.09921875)-float(sys.argv[4]))>1.e-4:
            return 1
    elif mode==6: # receive current position in mm
        print float(int(stage.getpos())*1e-3*0.09921875)
    return 0

## execute the main
if __name__ == "__main__":
    sys.exit(main())
