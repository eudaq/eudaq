#!/usr/bin/env python3

import pyeudaq
from labequipment import RTD23
from time import sleep
from datetime import datetime
import threading
from utils import exception_handler

class RTD23Producer(pyeudaq.Producer):
    def __init__(self,name,runctrl):
        pyeudaq.Producer.__init__(self,name,runctrl)
        self.name=name # TODO: better make available and use GetName()?
        self.rtd23=None
        self.is_running=False
        self.idev=0
        self.isev=0
        self.lock=threading.Lock()
        self.lastT=0

    @exception_handler
    def DoInitialise(self):
        self.rtd23=RTD23()
        self.idev=0
        self.isev=0

    @exception_handler
    def DoConfigure(self):
        self.idev=0
        self.isev=0

    @exception_handler
    def DoStartRun(self):
        self.is_running=True
        self.idev=0
        self.isev=0

    @exception_handler
    def DoStopRun(self):
        self.is_running=False

    @exception_handler
    def DoReset(self):
        self.is_running=False

    @exception_handler
    def DoStatus(self):
        self.SetStatusTag('StatusEventN','%d'%self.isev)
        self.SetStatusTag('DataEventN'  ,'%d'%self.idev)
        self.SetStatusMsg('%.2f degC'%(self.lastT))

    @exception_handler
    def RunLoop(self):
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
            self.lastT = self.rtd23.get_temperature()
            ev.SetTag('Temperature','%.2f degC' %self.lastT)
        if bore:
            ev.SetBORE()
        if eore:
            ev.SetEORE()
        self.SendEvent(ev)

if __name__=='__main__':
    import argparse
    parser=argparse.ArgumentParser(description='RTD23 Producer',formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--run-control' ,'-r',default='tcp://localhost:44000')
    parser.add_argument('--name' ,'-n',default='RTD23')
    args=parser.parse_args()

    myproducer=RTD23Producer(args.name,args.run_control)
    myproducer.Connect()
    while(myproducer.IsConnected()):
        #TODO: reod...
        sleep(1)

