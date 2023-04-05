#!/usr/bin/env python3

import pyeudaq
from pyeudaq import EUDAQ_INFO, EUDAQ_DEBUG
import trigger
import mlr1daqboard
from mlr1daqboard import pico_daq as daq
from datetime import datetime,timedelta
from time import sleep
import subprocess
import numpy as np
import os
from utils import exception_handler, logging_to_eudaq

def get_software_git(module):
    try:
        return subprocess.check_output(['git','rev-parse','HEAD'],
                                        cwd=os.path.dirname(module.__file__),
                                        stderr=subprocess.STDOUT).strip()
    except subprocess.CalledProcessError:
        return f"{module.__file__} is not a located in a git repository!"

def get_software_diff(module):
    try:
        return subprocess.check_output(['git','diff'],
                                       cwd=os.path.dirname(module.__file__),
                                       stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError:
            st = os.stat(module.__file__)
            return \
                f"File {module.__file__}:\n" +\
                f"Created:  {datetime.utcfromtimestamp(st.st_ctime)} UTC\n" +\
                f"Accessed: {datetime.utcfromtimestamp(st.st_atime)} UTC\n" +\
                f"Modified: {datetime.utcfromtimestamp(st.st_mtime)} UTC\n" +\
                f"Size:     {st.st_size}"

class DPTSProducer(pyeudaq.Producer):
    def __init__(self,name,runctrl):
        pyeudaq.Producer.__init__(self,name,runctrl)
        self.name=name # TODO: better make available and use GetName()?
        self.dpts=None
        self.daq=None
        self.trigger=None
        self.is_running=False
        self.idev=0
        self.isev=0
        self.iwav=0
        self.via_daq_board=True
        self.smart_rdo=None

    @exception_handler
    def DoInitialise(self):
        conf=self.GetInitConfiguration().as_dict()
        if 'smart_scope_readout' in conf:
            self.smart_rdo=timedelta(seconds=float(conf['smart_scope_readout']))
        self.trigger=trigger.Trigger(conf['trigger_path'] if "trigger_path" in conf else trigger.find_trigger() )
        self.plane=int(conf['plane'])
        self.daq=daq.ScopeAcqPS6000a(
            trg_ch="AUX",trg_mV=50,
            npre=10000,npost=250000,
            nsegments=int(conf['nsegments']) if 'nsegments' in conf else 1)
        assert self.daq.dt*1e9 <= 0.2, f"Expected picosopce time bins of 0.2 ns, but got {self.daq.dt*1e9} ns"
        if 'ctrl_ports' in conf:
            import dpts_ctrl # dpts-utils path must be set to use this (experts only)
            ctrl_ports = [p.strip() for p in conf['ctrl_ports'].split(',')]
            self.dpts = {p:dpts_ctrl.PmodCtrlDPTS(p) for p in ctrl_ports}
            self.via_daq_board=False
        elif 'serials' in conf and 'proximities' in conf:
            serials = [s.strip() for s in conf['serials'].split(',')]
            proxies = [p.strip() for p in conf['proximities'].split(',')]
            self.dpts = {p:mlr1daqboard.DPTSDAQBoard(calibration=p,serial=s) for p,s in zip(proxies,serials)}
        else:
            raise ValueError("Neither 'ctrl_ports' nor 'serials' and 'proximities' defined in conf file.")

    @exception_handler
    def DoConfigure(self):
        self.idev=0
        self.isev=0
        self.iwav=0
        conf=self.GetConfiguration().as_dict()
        self.trigger_on=conf['trigger_on']
        if self.daq.nsegments>1:
            assert not self.trigger_on=='AUX', \
                'Rapid block mode (nsegments>1) currently not supported with AUX trigger'
        if self.trigger_on not in ['B', 'C', 'D','E','AUX']:
            raise ValueError('need to specify trigger mode to be B, C, D, E or AUX')
        self.daq.set_trigger(trg_ch=self.trigger_on,trg_mV=50,aux_busy=(self.trigger_on!='AUX'))
        if self.trigger_on=='AUX':
            self.trigger.write(trigger.Trigger.MOD_SOFTBUSY,0,0x1,commit=True) # enable soft busy
        else:
            self.trigger.write(trigger.Trigger.MOD_SOFTBUSY,0,0x0,commit=True) # release sof busy
        if self.via_daq_board:
            for p,d in self.dpts.items():
                d.power_on()
                for bias in ["ireset","idb","ibias","ibiasn","vcasb","vcasn"]:
                    key = f"{p}_{bias}"
                    if key in conf:
                        getattr(d,f"set_{bias}")(float(conf[key]))
        for d in self.dpts: # expects arguments like JCI_mc or DPTS-002_mc = 0x00004000
            regs = {"rs":0x0,"mc":0x0,"md":0x0,"cs":0x0,"mr":0x0}
            for reg in regs.keys():
                key = f"{d}_{reg}"
                if key in conf: regs[reg] = int(conf[key],16)
            self.dpts[d].write_shreg(**regs)

    @exception_handler
    def DoStartRun(self):
        self.idev=0
        self.isev=0
        self.send_status_event(self.isev,self.idev,datetime.now(),bore=True)
        self.isev+=1
        self.armtrigger()
        self.is_running=True

    @exception_handler
    def DoStopRun(self):
        self.is_running=False

    @exception_handler
    def DoReset(self):
        self.is_running=False
        # TODO: cleanup!?

    @exception_handler
    def DoStatus(self):
        self.SetStatusTag('StatusEventN','%d'%self.isev)
        self.SetStatusTag('DataEventN'  ,'%d'%self.idev)
        self.SetStatusMsg(f'Waveforms with signal: {self.iwav}')

    @exception_handler
    def RunLoop(self):
        # status events: roughly every 10s (time checked at least every 1000 events or at receive timeout)
        tlast=datetime.now()
        ilast=0
        tlast_rdo=datetime.now()
        ilast_cap=0
        while self.is_running:
            checkstatus=False
            if self.daq.ready():
                self.read_and_send_events()
            elif not self.is_running:
                EUDAQ_INFO('stopping picoscope')
                self.daq.stop()
                ncap=self.daq.get_ncaptures()
                if ncap>0:
                    EUDAQ_INFO(f'{ncap} segments left to be read')
                    self.read_and_send_events(ncap)
                EUDAQ_INFO('all picoscope data read')
            elif self.smart_rdo is not None:
                ncap=self.daq.get_ncaptures()
                if ncap>0 and ilast_cap==ncap:
                    if datetime.now()-tlast_rdo>self.smart_rdo:
                        self.daq.stop()
                        self.read_and_send_events(self.daq.get_ncaptures())
                    else:
                        checkstatus=True
                else:
                    tlast_rdo=datetime.now()
                    ilast_cap=ncap
                    checkstatus=True
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
        self.send_status_event(self.isev,self.idev,tlast,eore=True)
        self.isev+=1
        self.daq.stop()

    def send_status_event(self,isev,idev,time,bore=False,eore=False):
        ev=pyeudaq.Event('RawEvent',self.name+'_status')
        ev.SetTag('Event','%d'%idev)
        ev.SetTag('Time',time.isoformat())
        if self.via_daq_board:
            for p,d in self.dpts.items():
                ev.SetTag(f"{p}_Ia",f"{d.read_isenseA():.2f} mA")
                ev.SetTag(f"{p}_Id",f"{d.read_isenseD():.2f} mA")
                ev.SetTag(f"{p}_Ib",f"{d.read_isenseB():.2f} mA")
        if bore:
            ev.SetBORE()
            git =subprocess.check_output(['git','rev-parse','HEAD']).strip()
            diff=subprocess.check_output(['git','diff'])
            ev.SetTag('EUDAQ_GIT'     ,git )
            ev.SetTag('EUDAQ_DIFF'    ,diff)
            ev.SetTag('DPTSUTILS_GIT' ,get_software_git(daq))
            ev.SetTag('DPTSUTILS_DIFF',get_software_diff(daq))
            ev.SetTag('TRIGGER_GIT'   ,get_software_git(trigger))
            ev.SetTag('TRIGGER_DIFF'  ,get_software_diff(trigger))
            ev.SetTag('Picoscope_model'       ,f"{self.daq.model}")
            ev.SetTag('Picoscope_time_bins'   ,f"{self.daq.dt*1e9} ns")
            ev.SetTag('Picoscope_pre_samples' ,f"{self.daq.npre}")
            ev.SetTag('Picoscope_post_samples',f"{self.daq.npost}")
        if bore and self.via_daq_board:
            ev.SetTag('MLR1SW_GIT' ,mlr1daqboard.get_software_git())
            ev.SetTag('MLR1SW_DIFF',mlr1daqboard.get_software_diff())
            for p,d in self.dpts.items():
                ev.SetTag(f'{p}_FW_VERSION',d.read_fw_version())
        if eore:
            ev.SetEORE()
        self.SendEvent(ev)

    def armtrigger(self):
        self.daq.arm()
        if self.trigger_on=='AUX':
            # print('release bsy')
            self.trigger.write(trigger.Trigger.MOD_SOFTBUSY,0,0x2,commit=True) # releases software busy and arms setting it right after receiving of trigger        

    def read_and_send_events(self,nevents=None):
        if nevents is None: nevents=self.daq.nsegments
        EUDAQ_DEBUG(f"rdo {nevents} event(s)")
        if nevents==1:
            data_array=[self.daq.rdo(nevents)]
        else:
            data_array=self.daq.rdo(nevents)
        for data in data_array:
            ev=pyeudaq.Event('RawEvent',self.name)
            tev=0 # no global time stamp (TODO: can the osciloscope provide one?)
            ev.SetEventN(self.idev)
            ev.SetTriggerN(self.idev)
            ev.SetTimestamp(begin=tev,end=tev)
            ev.SetDeviceN(self.plane)
            above=np.argwhere(data>30)[:,1]
            if above.size: # quick check if any waveform of any channel has values above a threshold
                self.iwav+=1
                trunc=np.max(above)+10 # truncate waveform after last detected peak
                ev.AddBlock(0,bytes(data[:,0:np.min([trunc,len(data[0])]) ] ))
            self.SendEvent(ev)
            self.idev+=1

        if self.is_running: self.armtrigger()

if __name__=='__main__':
    import argparse
    parser=argparse.ArgumentParser(description='DPTS EUDAQ2 Producer',formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--run-control' ,'-r',default='tcp://localhost:44000')
    parser.add_argument('--name' ,'-n',default='DPTS_XXX')
    args=parser.parse_args()

    logging_to_eudaq()

    myproducer=DPTSProducer(args.name,args.run_control)
    myproducer.Connect()
    while(myproducer.IsConnected()):
        sleep(1)
