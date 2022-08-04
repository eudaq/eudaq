#!/usr/bin/env python3

import pyeudaq
import mlr1daqboard
import logging
import subprocess
from time import sleep
from datetime import datetime

class APTSProducer(pyeudaq.Producer):
    def __init__(self, name, runctrl):
        pyeudaq.Producer.__init__(self,name,runctrl)
        self.name=name # TODO: better make available and use GetName()?
        self.daq=None
        self.is_running=False
        self.idev=0
        self.isev=0

    def DoInitialise(self):
        self.iev=0
        conf=self.GetInitConfiguration().as_dict()
        self.daq=mlr1daqboard.APTSDAQBoard(calibration = conf['proximity'], serial = conf['serial'])
        self.plane=int(conf['plane'])

    def DoConfigure(self):        
        self.idev=0
        self.isev=0
        conf=self.GetConfiguration().as_dict()

        self.daq.power_on()
        if 'IBIASN' in conf: self.daq.set_idac('CE_COL_AP_IBIASN',             float(conf['IBIASN']))
        if 'IBIASP' in conf: self.daq.set_idac('AP_IBIASP_DP_IDB',             float(conf['IBIASP']))
        if 'IBIAS3' in conf: self.daq.set_idac('AP_IBIAS3_DP_IBIAS',           float(conf['IBIAS3']))
        if 'IBIAS4' in conf: self.daq.set_idac('CE_MAT_AP_IBIAS4SF_DP_IBIASF', float(conf['IBIAS4']))
        if 'IRESET' in conf: self.daq.set_idac('CE_PMOS_AP_DP_IRESET',         float(conf['IRESET']))
        if 'VRESET' in conf: self.daq.set_vdac('AP_VRESET',                    float(conf['VRESET']))
        if 'VCASP'  in conf: self.daq.set_vdac('AP_VCASP_MUX0_DP_VCASB',       float(conf['VCASP']))
        if 'VCASN'  in conf: self.daq.set_vdac('AP_VCASN_MUX1_DP_VCASN',       float(conf['VCASN']))

        sampling_period  =int(conf['sampling_period']  ) if 'sampling_period'   in conf else 40
        n_frames_before  =int(conf['n_frames_before']  ) if 'n_frames_before'   in conf else 20
        n_frames_after   =int(conf['n_frames_after']   ) if 'n_frames_after'    in conf else 50
        trg_thr          =int(conf['trg_thr']          ) if 'trg_thr'           in conf else 20
        n_frames_auto_trg=int(conf['n_frames_auto_trg']) if 'n_frames_auto_trg' in conf else 2
        trg_type         =int(conf['trg_type']         ) if 'trg_type'          in conf else 0
        assert sampling_period>=40 # 6.25 ns units, Max sampling rate 4 MSPs = 40 units
        assert 0<n_frames_before<=100 # max based on Valerio's tests
        assert 0<n_frames_after <=700 # max based on Valerio's tests
        assert 0<n_frames_auto_trg<32
        assert trg_type in [0,1]
        self.daq.write_register(0x0,0xEF,0)  # PacketEndWithEachFrame
        self.daq.write_register(0x3,0x00,0)  # Switch ADC off (needs to be off when setting sampling period)
        self.daq.write_register(0x3,0x87,0)  # setDebugMode: 0: actual data, 1: test pattern 
        self.daq.write_register(0x3,0x01,sampling_period) 
        self.daq.write_register(0x3,0x02,n_frames_after)
        self.daq.write_register(0x3,0x03,n_frames_before)
        self.daq.write_register(0x3,0x04,1)       # Time between pulse and next sample
        self.daq.write_register(0x3,0x05,10000)   # SetPulseDuration, irrelevant
        self.daq.write_register(0x3,0x06,trg_thr) # Auto trigger threshold (1 bit = 38.1 uV)
        self.daq.write_register(0x3,0x07,n_frames_auto_trg) # num. of frames between samples compared in auto trigger
        self.daq.write_register(0x3,0x08,0xFFFF)    # Mask pixels for auto triggering TODO what is the mapping here?
        self.daq.write_register(0x3,0x09,0)     # Enable pulsing (disables triggering)
        self.daq.write_register(0x3,0x0A,1)     # WaitForTrigger 0 = acquire immediately, 1 = wait for trigger (int or ext)
        self.daq.write_register(0x3,0x0B,trg_type)  # TriggerType 0:external 1: auto
        self.daq.write_register(0x3,0x0C,1)  # SaveTriggerTimestamps 1:save 0:not save
        self.daq.write_register(0x3,0x0D,10) # TimeBeforeBusy 1 time unit = 10ns
        self.daq.write_register(0x3,0x0E,10) # TriggerOutDuration - 100 ns
        self.daq.write_register(0x3,0x00,1)  # Switch ADC on

    def DoStartRun(self):
        self.idev=0
        self.isev=0
        self.send_status_event(self.isev,self.idev,datetime.now(),bore=True)
        self.isev+=1
        while self.daq.read_data(1<<16,timeout=100) is not None:
            logging.info('Dirty buffer, purging...') # this should not happen
        self.daq.write_register(3,0xAC,1) # request next data / arm trigger
        self.is_running=True

    def DoStopRun(self):        
        self.is_running = False
        
    def DoReset(self):
        self.is_running = False
        
    def DoStatus(self):
        self.SetStatusTag('StatusEventN','%d'%self.isev)
        self.SetStatusTag('DataEventN'  ,'%d'%self.idev)

    def RunLoop(self):
        try:
            self._RunLoop()
        except Exception as e:
            logging.exception(e)
            raise
        
    def _RunLoop(self):
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
        while self.read_and_send_event(self.idev): # try to get anything remaining TODO: timeout?
            self.idev+=1
        tlast=datetime.now()
        self.send_status_event(self.isev,self.idev,tlast,eore=True)
        self.isev+=1
    
    def read_and_send_event(self,iev):
        logging.debug(" --- Read and send event ---")
        data = self.daq.read_data(40960,timeout=100) # sufficent for up to 1k frames
        if data is None: return False
        data = bytes(data)
        tsdata = bytes(self.daq.read_data(12,timeout=100))
        trg_ts = mlr1daqboard.decode_trigger_timestamp(tsdata)

        logging.debug(" send event APTSProducer.py")
        ev = pyeudaq.Event("RawEvent", self.name)
        ev.SetEventN(iev)
        ev.SetTriggerN(iev)
        ev.SetTimestamp(begin=trg_ts, end=trg_ts)
        ev.SetDeviceN(self.plane)
        ev.AddBlock(0,data)
        ev.AddBlock(1,tsdata)
        self.SendEvent(ev)

        if self.is_running: 
            self.daq.write_register(3,0xAC,1,log_level=9) # request next data / arm trigger
        return True
                
    def send_status_event(self,isev,idev,time,bore=False,eore=False):
        ev=pyeudaq.Event('RawEvent',self.name+'_status')
        ev.SetTag('Event','%d'%idev)
        ev.SetTag('Time',time.isoformat())
        ev.SetTag('Ia','%.2f mA'%self.daq.read_isenseA())
        ev.SetTag('Temperature','%.2f'%self.daq.read_temperature())
        if bore:
            ev.SetBORE()
            git =subprocess.check_output(['git','rev-parse','HEAD']).strip()
            diff=subprocess.check_output(['git','diff'])
            ev.SetTag('EUDAQ_GIT'  ,git )
            ev.SetTag('EUDAQ_DIFF' ,diff)
            ev.SetTag('MLR1SW_GIT' ,mlr1daqboard.get_software_git())
            ev.SetTag('MLR1SW_DIFF',mlr1daqboard.get_software_diff())
            ev.SetTag('FW_VERSION' ,self.daq.read_fw_version())
        if eore:
            ev.SetEORE()
        self.SendEvent(ev)
    
    
if __name__ == "__main__":
    import argparse
    parser=argparse.ArgumentParser(description='APTS EUDAQ2 Producer',formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--run-control' ,'-r',default='tcp://localhost:44000')
    parser.add_argument('--name' ,'-n',default='APTS_XXX')
    parser.add_argument('--log-level' ,'-l',default='INFO')
    args=parser.parse_args()

    logging.basicConfig(level=logging.getLevelName(args.log_level.upper()),format='%(asctime)s - %(levelname)s - %(message)s')

    myproducer=APTSProducer(args.name,args.run_control)
    myproducer.Connect()
    while(myproducer.IsConnected()):
        sleep(1)
