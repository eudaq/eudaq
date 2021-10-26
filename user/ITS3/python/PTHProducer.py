#!/usr/bin/env python3

import pyeudaq
import serial
from pth import *
from time import sleep
from datetime import datetime
import threading

class PTHProducer(pyeudaq.Producer):
    def __init__(self,name,runctrl):
        pyeudaq.Producer.__init__(self,name,runctrl)
        self.name=name # TODO: better make available and use GetName()?
        self.pth=None
        self.is_running=False
        self.idev=0
        self.isev=0
        self.lock=threading.Lock()

    def DoInitialise(self):
        self.pth=PTH()
        self.idev=0
        self.isev=0

    def DoConfigure(self):
        self.idev=0
        self.isev=0

    def DoStartRun(self):
        self.is_running=True
        self.idev=0
        self.isev=0
        
    def DoStopRun(self):
        self.is_running=False

    def DoReset(self):
        self.is_running=False

    def DoStatus(self):
        self.SetStatusTag('StatusEventN','%d'%self.isev);
        self.SetStatusTag('DataEventN'  ,'%d'%self.idev);
        with self.lock:
            if self.pth:
                self.SetStatusMsg('%.2f degC | %.2f mbar | %.2f rel%%'%(self.pth.getT(),self.pth.getP(),self.pth.getH()))

    def RunLoop(self):
        self.idev=0
        self.isev=0
        # TODO: status events
        try:
            self.foo()
        except Exception as e:
            print(e)
            raise
    def foo(self):
        self.send_status_event(time=datetime.now(),bore=True)
        self.isev+=1
        while self.is_running:
            self.send_status_event(time=datetime.now())
            self.isev+=1
            sleep(10)
        self.send_status_event(time=datetime.now(),eore=True)
        self.isev+=1

    def send_status_event(self,time,bore=False,eore=False):
        ev=pyeudaq.Event('RawEvent',self.name+'_status')
        ev.SetTag('Time'       ,time.isoformat())
        with self.lock:
            ev.SetTag('Pressure'   ,'%.2f mbar' %self.pth.getP())
            ev.SetTag('Temperature','%.2f degC' %self.pth.getT())
            ev.SetTag('Humidity'   ,'%.2f rel%%'%self.pth.getH())
        if bore:
            ev.SetBORE()
        if eore:
            ev.SetEORE()
        self.SendEvent(ev)

if __name__=='__main__':
    import argparse
    parser=argparse.ArgumentParser(description='PTH Producer',formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--run-control' ,'-r',default='tcp://localhost:44000')
    parser.add_argument('--name' ,'-n',default='PTH')
    args=parser.parse_args()

    myproducer=PTHProducer(args.name,args.run_control)
    myproducer.Connect()
    while(myproducer.IsConnected()):
        #TODO: reod...
        sleep(1)

