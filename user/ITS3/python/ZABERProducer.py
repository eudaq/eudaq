#!/usr/bin/env python3

import pyeudaq
from labequipment import ZABER
from time import sleep
from datetime import datetime
import threading
from utils import exception_handler

class ZABERProducer(pyeudaq.Producer):
    def __init__(self,name,runctrl):
        pyeudaq.Producer.__init__(self,name,runctrl)
        self.name=name # TODO: better make available and use GetName()?
        self.z=None
        self.is_running=False
        self.idev=0
        self.isev=0
        self.lock=threading.Lock()
        self.last_status_msg=""

    @exception_handler
    def DoInitialise(self):
        conf = self.GetInitConfiguration().as_dict()
        self.z=ZABER(serial_path=conf.get("serial_path"), config_path=conf.get("config_path"))
        self.idev=0
        self.isev=0

    @exception_handler
    def DoConfigure(self):
        conf=self.GetConfiguration().as_dict()
        for setting in conf.keys():
            if setting.startswith("set_pos_"):
                axis_query = setting.replace("set_pos_", "")
                if axis_query.startswith("axisnumber_"):
                    axis_query = int(axis_query.replace("axisnumber_", ""))
                with self.lock:
                    axis = self.z.get_axis(axis_query)
                    self.z.move_axis(axis, float(conf[setting]), absolute=True)
            if setting.startswith("move_"):
                axis_query = setting.replace("move_", "")
                if axis_query.startswith("axisnumber_"):
                    axis_query = int(axis_query.replace("axisnumber_", ""))
                with self.lock:
                    axis = self.z.get_axis(axis_query)
                    self.z.move_axis(axis, float(conf[setting]), absolute=False)
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
        self.SetStatusMsg(self.last_status_msg)

    @exception_handler
    def RunLoop(self):
        # send status event roughly every 60s
        tlast=datetime.now()
        ilast=0
        self.send_status_event(tlast,bore=True)
        self.isev+=1
        while self.is_running:
            checkstatus=False
            if (self.isev-ilast)%1000==0: checkstatus=True
            if checkstatus:
                if (datetime.now()-tlast).total_seconds()>=60:
                    tlast=datetime.now()
                    ilast=self.isev
                    self.send_status_event(tlast)
                    self.isev+=1

    def send_status_event(self,time,bore=False,eore=False):
        ev=pyeudaq.Event('RawEvent',self.name+'_status')
        ev.SetTag('Time',time.isoformat())
        with self.lock:
            status=[]
            for axis in self.z.axes:
                pos = str(round(self.z.get_pos(axis)["data_converted"],2)) + axis.unit
                status.append("Axis {}: {}".format(self.z.get_displayname(axis), pos))
                ev.SetTag("position_" + self.z.get_displayname(axis), pos)
            self.last_status_msg = " | ".join(status)
        if bore:
            ev.SetBORE()
        if eore:
            ev.SetEORE()
        self.SendEvent(ev)

if __name__=='__main__':
    import argparse
    parser=argparse.ArgumentParser(description='ZABER Producer',formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--run-control' ,'-r',default='tcp://localhost:44000')
    parser.add_argument('--name' ,'-n',default='ZABER')
    args=parser.parse_args()

    myproducer=ZABERProducer(args.name,args.run_control)
    myproducer.Connect()
    while(myproducer.IsConnected()):
        #TODO: reod...
        sleep(1)

