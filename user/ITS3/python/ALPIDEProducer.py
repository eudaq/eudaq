#!/usr/bin/env python3

import pyeudaq
import alpidedaqboard
from datetime import datetime
from time import sleep
import subprocess
from utils import exception_handler

ALPIDE_DUMP_REGS={
  'COMMAND'        :0x0000,
  'MODE_CTRL'      :0x0001,
  'REGION_DISABLE1':0x0002,
  'REGION_DISABLE2':0x0003,
  'FROMU_CONF1'    :0x0004,
  'FROMU_CONF2'    :0x0005,
  'FROMU_CONF3'    :0x0006,
  'FROMU_PULSING1' :0x0007,
  'FROMU_PULSING2' :0x0008,
  'FROMU_STATUS1'  :0x0009,
  'FROMU_STATUS2'  :0x000A,
  'FROMU_STATUS3'  :0x000B,
  'FROMU_STATUS4'  :0x000C,
  'FROMU_STATUS5'  :0x000D,
  'DAC_CLK'        :0x000E,
  'DAC_CMU'        :0x000F,
  'CMU_DUM_CONFIG' :0x0010,
  'CMU_ERROR'      :0x0011,
#  'DMU_FIFO_LSB'   :0x0012, # FIXME: reading this has a side effect...
#  'DMU_FIFO_MSB'   :0x0013,
  'BUSY_MIN_WIDTH' :0x001B,
  'ANALOG_OVERRIDE':0x0600,
  'VRESETP'        :0x0601,
  'VRESETD'        :0x0602,
  'VCASP'          :0x0603,
  'VCASN'          :0x0604,
  'VPULSEH'        :0x0605,
  'VPULSEL'        :0x0606,
  'VCASN2'         :0x0607,
  'VCLIP'          :0x0608,
  'VTEMP'          :0x0609,
  'IAUX2'          :0x060A,
  'IRESET'         :0x060B,
  'IDB'            :0x060C,
  'IBIAS'          :0x060D,
  'ITHR'           :0x060E,
  'BUFFER_BYPASS'  :0x060F,
  'SEU_COUNTER'    :0x0700,
  'TEST_CONTROL'   :0x0701,
}
for reg in range(32):
    ALPIDE_DUMP_REGS['DC_DISABLE_%d'%reg]=reg<<11|0x300
    ALPIDE_DUMP_REGS['REGION_STATUS_%d'%reg]=reg<<11|0x301

class ALPIDEProducer(pyeudaq.Producer):
    def __init__(self,name,runctrl):
        pyeudaq.Producer.__init__(self,name,runctrl)
        self.name=name # TODO: better make available and use GetName()?
        self.daq=None
        self.is_running=False
        self.triggermode=None
        self.idev=0
        self.isev=0

    @exception_handler
    def DoInitialise(self):
        self.iev=0
        conf=self.GetInitConfiguration().as_dict()
        self.daq=alpidedaqboard.ALPIDEDAQBoard(conf['serial'])
        self.plane=int(conf['plane'])
        self.triggermode=conf['triggermode']
        if self.triggermode not in ['primary','replica']:
            raise ValueError('need to specify trigger mode to be either "primary" or "replica"')
        self.stoptrigger()

    @exception_handler
    def DoConfigure(self):        
        self.idev=0
        self.isev=0
        self.stoptrigger()

        self.daq.power_on()

        # just in case we got up on the wrong side of the fw...
        self.daq.trg.opcode.write(0x55)
        self.daq.rdoctrl.rst.issue()
        self.daq.rdopar.rst.issue()
        self.daq.evtpkr.rst.issue()
        self.daq.evtbld.rst.issue()
        self.daq.trg.rstrdobsy.issue()
        self.daq.purge_events()
        self.daq.xonxoff.rst.issue()
        
        # now for monitoring, also start clean
        self.daq.rdopar.clr.issue()
        self.daq.trgmon.clr.issue()

        # ...or the ALPIDE...
        self.daq.alpide_cmd_issue(0xD2) # GRST

        # now for monitoring, also start clean
        self.daq.rdopar.clr.issue()
        self.daq.trgmon.clr.issue()

        conf=self.GetConfiguration().as_dict()
        self.chipid=int(conf['CHIPID']) if 'CHIPID' in conf else 0x10
        if 'VCASN'   in conf: self.daq.alpide_reg_write(0x604,int(conf['VCASN' ]),chipid=self.chipid)
        if 'VCASN2'  in conf: self.daq.alpide_reg_write(0x607,int(conf['VCASN2']),chipid=self.chipid)
        if 'ITHR'    in conf: self.daq.alpide_reg_write(0x60E,int(conf['ITHR'  ]),chipid=self.chipid)
        if 'VCLIP'   in conf: self.daq.alpide_reg_write(0x608,int(conf['VCLIP' ]),chipid=self.chipid)
        if 'IDB'     in conf: self.daq.alpide_reg_write(0x60C,int(conf['IDB'   ]),chipid=self.chipid)
        self.daq.alpide_reg_write(0x602,int(conf['VRESETD']) if 'VRESETD' in conf else 147,chipid=self.chipid)
        
        self.strblen=int(conf['STROBE_LENGTH'])
        self.rdomode=conf['RDOMODE'] if 'RDOMODE' in conf else 'DATAPORT'

        self.daq.alpide_cmd_issue(0xE4) # PRST
        self.daq.alpide_reg_write(0x0004,0x0060      ,chipid=self.chipid) # disable busy monitoring, analog test pulse, enable test strobe
        self.daq.alpide_reg_write(0x0005,self.strblen,chipid=self.chipid) # strobe length
        self.daq.alpide_reg_write(0x0010,0x0030      ,chipid=self.chipid) # initial token, SDR, disable manchester, previous token == self!!!
        self.daq.alpide_cmd_issue(0x63) # RORST (needed!!!)

        if self.rdomode=='DCTRL':
            self.daq.alpide_reg_write(0x0001,0x020D,chipid=self.chipid) # normal readout, TRG mode, CMU RDO
            self.daq.rdoctrl.delay_set.write(2*self.strblen+100) # when to start reading (@80MHz, i.e. at least strobe-time x2 + sth.) ... TODO
            self.daq.rdoctrl.chipid.write(self.chipid)
            self.daq.rdoctrl.ctrl.write(1) # enable DCTRL RDO
            self.daq.rdomux.ctrl.write(2) # select DCTRL RDO
        elif self.rdomode=='DATAPORT':
            self.daq.alpide_reg_write(0x0001,0x000D,chipid=self.chipid) # normal readout, TRG mode
            self.daq.rdopar.ctrl.write(1) # enable parallel port RDO
            self.daq.rdomux.ctrl.write(1) # select parallel port RDO
            self.daq.xonxoff.ctrl.write(1) # enable XON XOFF
        else:
            raise ValueError('RDOMODE "%s" not understood'%self.rdomode)

        self.daq.trg.minspacing.write(int(conf['minspacing']) if 'minspacing' in conf else 0)
        self.daq.trg.fixedbusy .write(int(conf['fixedbusy' ]) if 'fixedbusy'  in conf else 0)

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
        self.stoptrigger()
        self.is_running=False

    @exception_handler
    def DoReset(self):
        self.stoptrigger()
        self.is_running=False
        # TODO: cleanup!?

    @exception_handler
    def DoStatus(self):
        self.SetStatusTag('StatusEventN','%d'%self.isev);
        self.SetStatusTag('DataEventN'  ,'%d'%self.idev);

    @exception_handler
    def RunLoop(self):
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
        while self.read_and_send_event(self.idev): # try to get anything remaining
            self.idev+=1
        tlast=datetime.now()
        self.send_status_event(self.isev,self.idev,tlast,eore=True)
        self.isev+=1

    def armtrigger(self):
        if self.triggermode=='primary':
            self.daq.trg.ctrl.write(0b1000) # primary mode, no maskig of external trigger or busy, no forced busy (anymore)
            # TODO: needs to go out:
            self.daq.trgseq.dt_set.write(8000) # 10 kHz
            self.daq.trgseq.ntrg_set.write(300)
        elif self.triggermode=='replica':
            self.daq.trg.ctrl.write(0b0000) # replica mode, no masking, no forced busy (anymore)

    def stoptrigger(self):
        if self.triggermode=='primary':
            self.daq.trg.ctrl.write(0b1001) # primary mode, no maskig of external trigger or busy, force busy
            # TODO: needs to go out:
            self.daq.trgseq.stop.issue() 
        elif self.triggermode=='replica':
            self.daq.trg.ctrl.write(0b0001) # replica mode, no masking, force busy
       
    def send_status_event(self,isev,idev,time,bore=False,eore=False):
        ev=pyeudaq.Event('RawEvent',self.name+'_status')
        idda,iddd,status=self.daq.power_status()
        temp=self.daq.carrier_temp()
        ev.SetTag('IDDA','%.2f mA'%idda)
        ev.SetTag('IDDD','%.2f mA'%iddd)
        ev.SetTag('Temperature','%.2f C'%temp if temp else 'Invalid temperature read!')
        ev.SetTag('Event','%d'%idev)
        ev.SetTag('Time',time.isoformat())
        if bore:
            ev.SetBORE()
            ev.SetTag('FPGA_GIT'    ,self.daq.get_fpga_git())
            ev.SetTag('FPGA_COMPILE',self.daq.get_fpga_tcompile().isoformat())
            #ev.SetTag('FX3_GIT'     ,self.daq.get_fx3_git())
            #ev.SetTag('FX3_COMPILE' ,self.daq.get_fx3_tcompile().isoformat())
            git =subprocess.check_output(['git','rev-parse','HEAD']).strip()
            diff=subprocess.check_output(['git','diff'])
            ev.SetTag('EUDAQ_GIT'   ,git )
            ev.SetTag('EUDAQ_DIFF'  ,diff)
            ev.SetTag('ALPIDEDAQ_GIT' ,self.daq.get_software_git())
            ev.SetTag('ALPIDEDAQ_DIFF',self.daq.get_software_diff())
        if eore:
            ev.SetEORE()
        if bore or eore:
            for reg,addr in ALPIDE_DUMP_REGS.items():
                ev.SetTag('ALPIDE_%04x_%s'%(addr,reg),'%d'%self.daq.alpide_reg_read(addr,chipid=self.chipid))
            self.daq.trgmon.lat.issue()
            for reg in self.daq.trgmon.regs:
                ev.SetTag('TRGMON_'+reg.upper(),'%d'%self.daq.trgmon.regs[reg].read())
        self.SendEvent(ev)

    def read_and_send_event(self,iev):
        raw=self.daq.event_read()
        if raw:
            raw=bytes(raw)
            assert len(raw)>=20
            assert list(raw[:4])==[0xAA]*4
            assert list(raw[-4:])==[0xBB]*4
            itrg=sum(b<<(j*8) for j,b in enumerate(raw[4: 8]))
            tev =sum(b<<(j*8) for j,b in enumerate(raw[8:16]))
            ev=pyeudaq.Event('RawEvent',self.name)
            ev.SetEventN(iev) # clarification: iev is the number of received events (starting from zero) # FIXME: is overwritten in SendEvent
            ev.SetTriggerN(itrg) # clarification: itrg is the event number form the DAQ board
            ev.SetTimestamp(begin=tev,end=tev)
            ev.SetDeviceN(self.plane)
            ev.AddBlock(0,raw)
            self.SendEvent(ev)
            return True
        else:
            return False

if __name__=='__main__':
    import argparse
    parser=argparse.ArgumentParser(description='ALPIDE EUDAQ2 Producer',formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--run-control' ,'-r',default='tcp://localhost:44000')
    parser.add_argument('--name' ,'-n',default='ALPIDE_XXX')
    args=parser.parse_args()

    myproducer=ALPIDEProducer(args.name,args.run_control)
    myproducer.Connect()
    while(myproducer.IsConnected()):
        sleep(1)
