#!/usr/bin/env python3

import pyeudaq
from labequipment import HAMEG
from time import sleep
from datetime import datetime
import threading
from utils import exception_handler

class PowerProducer(pyeudaq.Producer):
    def __init__(self,name,runctrl):
        pyeudaq.Producer.__init__(self,name,runctrl)
        self.name=name # TODO: better make available and use GetName()?
        self.ps=None
        self.is_running=False
        self.idev=0
        self.isev=0
        self.lock=threading.Lock()
        self.last_status=None

    @exception_handler
    def DoInitialise(self):
        self.ps=HAMEG(self.GetInitConfiguration().as_dict()['path'])
        self.idev=0
        self.isev=0

    @exception_handler
    def DoConfigure(self):
        conf=self.GetConfiguration().as_dict()
        for ch in range(1,self.ps.n_ch+1):
            if 'current_%d'%ch in conf: self.ps.set_curr(ch,float(conf['current_%d'%ch]))
            if 'voltage_%d'%ch in conf:
                if 'steps_%d'%ch in conf:
                    nsteps=int(conf['steps_%d'%ch])
                    for step in range(1,nsteps+1):
                            self.ps.set_volt(ch,float(conf['voltage_%d'%ch])*step/nsteps)
                            sleep(0.5/nsteps)
                else:
                    self.ps.set_volt(ch,float(conf['voltage_%d'%ch]))
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
        self.SetStatusTag('StatusEventN','%d'%self.isev);
        self.SetStatusTag('DataEventN'  ,'%d'%self.idev);
        if self.last_status:
            sel,volt_set,curr_set,volt_meas,curr_meas,fused=self.last_status
            self.SetStatusMsg(
                ' | '.join([f"{'ON' if sel[ch] else 'OFF'}{' FUSE' if fused[ch] else ''} {volt_meas[ch]:.2f}V {curr_meas[ch]*1.e3:.1f}mA" for ch in range(self.ps.n_ch)])
            )

    @exception_handler
    def RunLoop(self):
        self.send_status_event(time=datetime.now(),bore=True)
        self.isev+=1
        while self.is_running:
            self.send_status_event(time=datetime.now())
            self.isev+=1
            sleep(5)
        self.send_status_event(time=datetime.now(),eore=True)
        self.isev+=1

    def send_status_event(self,time,bore=False,eore=False):
        ev=pyeudaq.Event('RawEvent',self.name+'_status')
        ev.SetTag('Time',time.isoformat())
        with self.lock:
            self.last_status = self.ps.status()
            sel,volt_set,curr_set,volt_meas,curr_meas,fused=self.last_status
        for ch in range(1,self.ps.n_ch+1):
            ev.SetTag('enabled_%d'     %ch,'%d'     %(sel      [ch-1]     ))
            ev.SetTag('fused_%d'       %ch,'%d'     %(fused    [ch-1]     ))
            ev.SetTag('voltage_set_%d' %ch,'%.2f V' %(volt_set [ch-1]     ))
            ev.SetTag('current_set_%d' %ch,'%.2f mA'%(curr_set [ch-1]/1e-3))
            ev.SetTag('voltage_meas_%d'%ch,'%.2f V' %(volt_meas[ch-1]     ))
            ev.SetTag('current_meas_%d'%ch,'%.2f mA'%(curr_meas[ch-1]/1e-3))
        if bore:
            ev.SetTag('IDN',self.ps.idn)
            ev.SetBORE()
        if eore:
            ev.SetEORE()
        self.SendEvent(ev)

if __name__=='__main__':
    import argparse
    parser=argparse.ArgumentParser(description='Power Producer',formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--run-control' ,'-r',default='tcp://localhost:44000')
    parser.add_argument('--name' ,'-n',default='POWER')
    args=parser.parse_args()

    myproducer=PowerProducer(args.name,args.run_control)
    myproducer.Connect()
    while(myproducer.IsConnected()):
        #TODO: reod...
        sleep(1)

