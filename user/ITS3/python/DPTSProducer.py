#!/usr/bin/env python3

import pyeudaq
import daq
#import dptsctrl TODO
import trigger
from datetime import datetime
from time import sleep
import subprocess

class DPTSProducer(pyeudaq.Producer):
    def __init__(self,name,runctrl):
        pyeudaq.Producer.__init__(self,name,runctrl)
        print(name)
        self.name=name # TODO: better make available and use GetName()?
        self.daq=None
        self.trigger=None
        self.is_running=False
        self.idev=0
        self.isev=0

    def DoInitialise(self):
        conf=self.GetInitConfiguration().as_dict()
        self.trigger=trigger.Trigger(conf['trigger_path'])
        self.plane=int(conf['plane'])
        #self.dpts_ctrl=dptsctrl.DPTSCtrl(int(conf['ctrl_port'])) TODO

    def DoConfigure(self):        
        self.idev=0
        self.isev=0

        conf=self.GetConfiguration().as_dict()
        self.trigger_on=conf['trigger_on']
        if self.trigger_on not in ['D','E','AUX']:
            raise ValueError('need to specify trigger mode to be either "primary" or "replica"')
        if self.trigger_on=='AUX':
            self.trigger.write(trigger.Trigger.MOD_SOFTBUSY,0,0x1,commit=True) # enable soft busy
        else:
            self.trigger.write(trigger.Trigger.MOD_SOFTBUSY,0,0x0,commit=True) # release sof busy
#        if self.daq is not None:
#            del self.daq
#            self.daq=None
        if self.daq is None:
            self.daq=daq.ScopeAcqPS6000a(trg_ch=self.trigger_on,trg_mV=50,npre=5000,npost=200000)

    def DoStartRun(self):
        self.idev=0
        self.isev=0
        self.send_status_event(self.isev,self.idev,datetime.now(),bore=True)
        self.isev+=1
        self.is_running=True
        
    def DoStopRun(self):
        self.is_running=False

    def DoReset(self):
        self.is_running=False
        # TODO: cleanup!?

    def DoStatus(self):
        self.SetStatusTag('StatusEventN','%d'%self.isev);
        self.SetStatusTag('DataEventN'  ,'%d'%self.idev);

    def RunLoop(self):
        try:
            self.foo()
        except Exception as e:
            print(e)
            raise
    def foo(self):
        # status events: roughly every 10s (time checked at least every 1000 events or at receive timeout)
        tlast=datetime.now()
        ilast=0
        while self.is_running:
            checkstatus=False
            if self.read_and_send_event(self.idev):
                self.idev+=1
            else:
                checkstatus=True
            if (self.idev-ilast)%1000==0: checkstatus=True
            if checkstatus:
                if (datetime.now()-tlast).total_seconds()>=10:
                    tlast=datetime.now()
                    ilast=self.idev
                    self.send_status_event(self.isev,ilast,tlast)
                    self.isev+=1
        tlast=datetime.now()
        self.send_status_event(self.isev,self.idev,tlast)
        self.isev+=1
        sleep(1)
        #while self.read_and_send_event(self.idev): # try to get anything remaining TODO: timeout?
        #    self.idev+=1
        tlast=datetime.now()
        self.send_status_event(self.isev,self.idev,tlast,eore=True)
        self.isev+=1

    def send_status_event(self,isev,idev,time,bore=False,eore=False):
        ev=pyeudaq.Event('RawEvent',self.name+'_status')
        ev.SetTag('Event','%d'%idev)
        ev.SetTag('Time',time.isoformat())
        if bore:
            ev.SetBORE()
            git =subprocess.check_output(['git','rev-parse','HEAD']).strip()
            diff=subprocess.check_output(['git','diff'])
            ev.SetTag('EUDAQ_GIT'   ,git )
            ev.SetTag('EUDAQ_DIFF'  ,diff)
        if eore:
            ev.SetEORE()
        self.SendEvent(ev)

    def read_and_send_event(self,iev):
        self.daq.arm()
        if self.trigger_on=='AUX':
            print('release bsy')
            self.trigger.write(trigger.Trigger.MOD_SOFTBUSY,0,0x2,commit=True) # releases software busy and arms setting it right after receivign of trigger
        print('wait data')
        self.daq.wait()
        print('done')
        data=self.daq.rdo()
        ev=pyeudaq.Event('RawEvent',self.name)
        tev=0 # no global time stamp (TODO: can the osciloscope provide one?)
        ev.SetEventN(iev) # clarification: iev is the number of received events (starting from zero) # FIXME: is overwritten in SendEvent
        ev.SetTriggerN(iev) # clarification: itrg is set to iev
        ev.SetTimestamp(begin=tev,end=tev)
        ev.SetDeviceN(self.plane)
        ev.AddBlock(0,bytes(data))
        self.SendEvent(ev)
        return True

if __name__=='__main__':
    import argparse
    parser=argparse.ArgumentParser(description='DPTS EUDAQ2 Producer',formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--run-control' ,'-r',default='tcp://localhost:44000')
    parser.add_argument('--name' ,'-n',default='DPTS_XXX')
    args=parser.parse_args()

    myproducer=DPTSProducer(args.name,args.run_control)
    myproducer.Connect()
    while(myproducer.IsConnected()):
        #TODO: reod...
        sleep(1)
#        if myproducer.is_running and myproducer.triggermode=='primary':
#            myproducer.daq.trgseq.start.issue()

