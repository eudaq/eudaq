#!/usr/bin/env python3

import pyeudaq
import moss_test
from moss_test.test_system.version import get_software_version
from moss_test.test_system.exceptions import IncompleteMossPacket
from moss_test.moss_unit_if.moss_unit_if import MossUnitIF
from moss_test.moss_unit_if.power import log_moss_currents
from moss_test.moss_unit_if.moss_registers import MossRegion, MossDac, MossPeripheryRegister, MossRegionRegister
from datetime import datetime
from time import sleep
import subprocess
from utils import exception_handler, logging_to_eudaq, str2int
import logging

class MOSSProducer(pyeudaq.Producer):
    def __init__(self,name,runctrl):
        pyeudaq.Producer.__init__(self,name,runctrl)
        self.name=name
        self.ts=None
        self.moss=None
        self.fpga=None
        self.is_running=False
        self.idev=0
        self.isev=0
        self.last_event_size=None
        self.total_event_size=0
        self.last_currents=None
        self.debug=False
        self.custom_msg = ""

    @exception_handler
    def DoInitialise(self):
        self.idev=0
        self.isev=0
        self.last_event_size=None
        self.total_event_size=0
        conf=self.GetInitConfiguration().as_dict()
        self.plane=int(conf['plane'])
        self.ts=moss_test.ts(conf['ts_config_path'])
        self.ts.initialize()
        self.moss: MossUnitIF=self.ts.moss_if(conf["loc_id"])
        self.fpga=self.ts.fpga(self.moss.sn())
        self.debug="DEBUG" in conf and int(conf["DEBUG"])
        self.custom_msg = ""

    @exception_handler
    def DoConfigure(self):        
        self.idev=0
        self.isev=0
        self.last_event_size=None
        self.total_event_size=0
        self.custom_msg = ""
        conf: dict = self.GetConfiguration().as_dict()

        unit = self.moss.proximity_id()

        self.fpga.moss_top().disable_input_all_units()
        self.fpga.moss_top().enable_static_mode()
        self.fpga.trigger_handler().set_base_configuration()
        self.fpga.trigger_handler().reset_counters()
        self.fpga.moss_top().select_data_fifo(unit)

        while self.fpga.read_data():
            self.fpga.logger.debug("Purging FX3 and FIFO...")

        assert self.moss.power_on(), "MOSS Powering failed!"

        self.moss.logger.info("Currents after power ON:")
        log_moss_currents(self.moss.loc(), self.moss.monitor_currents())

        bandgap_trim: str = conf.get("BANDGAP_TRIM", "auto")
        if bandgap_trim == "auto":
            self.moss.trim_all_bandgaps()
        else:
            for i,trim in enumerate(bandgap_trim.strip().strip("[]").split(",")):
                val = str2int(trim.strip())
                self.moss.logger.info(f"Setting region {i} trimming to 0x{val:02x}.")
                self.moss.set_dac_trimming_raw(i, val)
        self.moss.set_default_dacs(MossRegion.ALL_REGIONS)

        region_enable = str2int(conf["REGION_ENABLE"])
        self.moss.set_enable_region_readout_mask(region_enable)

        for region in [r for r in range(4) if region_enable>>r&1]:
            self.moss.logger.info(f"Configuring region {region}")
            for dac in MossDac:
                dac_name_conf = f"Region{region}_"+dac.name
                if dac_name_conf in conf:
                    self.moss.logger.info(f"Setting {dac.name} to {conf[dac_name_conf]}")
                    self.moss.set_dac(region, dac, int(conf[dac_name_conf]))

        self.moss.readout_reset() # prepares readout, unmasks all pixels...
        self.moss.set_strobe_length(MossRegion.ALL_REGIONS, int(conf["STROBE_LENGTH"]))

        self.moss.logger.info("Currents after configure:")
        log_moss_currents(self.moss.loc(), self.moss.monitor_currents())

        self.fpga.trigger_handler().set_configuration(
            enable=False,
            listen=True,
            send_strobe=True,
            send_readout=True,
            triggers_to_execute=1,
            trigger2strobe=int(conf.get("STROBE_DELAY", 1)),
            trigger2readout=int(conf.get("STROBE_DELAY", 1)) + int(conf["STROBE_LENGTH"]) + 100,  # 100 for margin
            trigger2trigger=int(conf.get("STROBE_DELAY", 1)) + int(conf["STROBE_LENGTH"]) + 1000,  # irrelevant for 1 trigger
            hu_enabled=1 << (unit - 1),
        )
        self.fpga.moss_top().enable_input_unit(unit)
        

    @exception_handler
    def DoStartRun(self):
        self.idev=0
        self.isev=0
        self.last_event_size=None
        self.total_event_size=0
        self.custom_msg = ""
        self.moss.logger.info("Flushing MOSS FIFO")  # this should not be needed any more, but can do no harm
        self.fpga.moss_top().readout_transaction(self.moss.proximity_id())
        sleep(0.01)
        while self.fpga.read_data():
            self.fpga.logger.debug("Purging unit FIFO!")
        self.send_status_event(self.isev,self.idev,datetime.now(),bore=True)
        assert self.moss.is_power_ok(), "MOSS power NOK!"
        self.fpga.trigger_handler().reset_counters()
        self.fpga.trigger_handler().set_base_configuration(
            enable=True,
            listen=True,
            send_strobe=True,
            send_readout=True,
        )
        self.isev+=1
        self.is_running=True
        
    @exception_handler
    def DoStopRun(self):
        self.is_running=False
        self.fpga.trigger_handler().set_base_configuration(enable=False)

    @exception_handler
    def DoReset(self):
        self.is_running=False
        self.fpga.trigger_handler().set_base_configuration(enable=False)

    @exception_handler
    def DoTerminate(self):
        self.fpga.trigger_handler().set_base_configuration(enable=False)
        self.fpga.moss_top().disable_input_all_units()

    @exception_handler
    def DoStatus(self):
        self.SetStatusTag('StatusEventN','%d'%self.isev)
        self.SetStatusTag('DataEventN'  ,'%d'%self.idev)
        self.SetStatusMsg(self.custom_msg + f"{self.last_currents} | Last event size: {self.last_event_size}" +\
                          f" | Avg. event size: {int(self.total_event_size/self.idev) if self.idev else 0}")

    @exception_handler
    def RunLoop(self):
        # status events: roughly every 10s (time checked at least every 1000 events or at receive timeout)
        tlast=datetime.now()
        ilast=0
        while self.is_running:
            checkstatus=False
            if not self.read_and_send_event():
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
        while self.read_and_send_event(): # try to get anything remaining
            pass
        tlast=datetime.now()
        self.send_status_event(self.isev,self.idev,tlast,eore=True)
        self.isev+=1

       
    def send_status_event(self,isev:int,idev:int,time: datetime,bore=False,eore=False):
        ev=pyeudaq.Event('RawEvent',self.name+'_status')
        currents = self.moss.measure_currents()
        self.last_currents = currents
        ev.SetTag('SatusEventN', str(isev))
        ev.SetTag('AVDD', str(currents.avdd))
        ev.SetTag('DVDD', str(currents.dvdd))
        ev.SetTag('IOVDD',str(currents.iovdd))
        if self.ts.is_ntc_connected():
            for channel, (temperature, _) in self.ts.ntc().sample_all_temperatures().items():
                ev.SetTag(channel, f"{temperature:.1f} C")
        ev.SetTag('Events','%d'%idev)
        ev.SetTag('Time',time.isoformat())
        if bore:
            ev.SetBORE()
            ev.SetTag('FPGA_GIT'    , hex(self.fpga.id().hash_long()))
            ev.SetTag('FPGA_COMPILE', self.fpga.id().compile_date())
            git =subprocess.check_output(['git','rev-parse','HEAD']).strip()
            diff=subprocess.check_output(['git','diff'])
            ev.SetTag('EUDAQ_GIT'   , git )
            ev.SetTag('EUDAQ_DIFF'  , diff)
            ev.SetTag('MOSSSW_VERSION'  , get_software_version())
        if eore:
            ev.SetEORE()
            for k,v in self.fpga.trigger_handler().get_statistics().items():
                ev.SetTag(k, str(v))
        if bore or eore:
            for region in range(4):
                for addr in MossRegionRegister:
                    if "BASE" in addr.name: continue
                    ev.SetTag(f"MOSS_0x{addr+region*0x200:04X}_region{region}_{addr.name}",
                              f"0x{self.moss.read_region_register(region, addr):04X}")
            for addr in MossPeripheryRegister:
                ev.SetTag(f"MOSS_0x{addr:04X}_{addr.name}",
                          f"0x{self.moss.read(addr):04X}")
            for module in [self.fpga.id(), self.fpga.trigger_handler(), self.fpga.moss_top(), self.fpga.power_controller()]:
                for key, val in module.dump_all_registers().items():
                    ev.SetTag(f"FPGA_{module.module_id().name}_{key}", hex(val))
        self.SendEvent(ev)

    def read_and_send_event(self):
        if self.debug:
            if self.is_running:
                self.fpga.moss_top().strobe_transaction(self.moss.proximity_id())
                sleep(0.001)
                self.fpga.moss_top().readout_transaction(self.moss.proximity_id())
                sleep(0.1)
        try:
            data = self.fpga.get_data_packet() # >10 kb of data => noise
        except IncompleteMossPacket as exc:
            # avoid runs crashing DAQ for unsupervised running
            pyeudaq.EUDAQ_WARN(f"IncompleteMossPacket: {exc}")
            self.custom_msg = "WARNING: IncompleteMossPacket | "
            if self.fpga.moss_top().get_fifo_full_counter(self.moss.proximity_id()):
                self.custom_msg += "FIFO full | "
            data = exc.packet
        if data is None:
            return False
        ev=pyeudaq.Event('RawEvent',self.name)
        ev.SetTriggerN(self.idev)
        now = int(datetime.now().timestamp() * 1e6)
        ev.SetTimestamp(begin=now,end=now)
        ev.SetDeviceN(self.plane)
        ev.AddBlock(0,data)
        ev.SetTag("VerticalLocation", "TOP" if self.moss.location().is_top() else "BOTTOM")
        self.SendEvent(ev)
        self.idev+=1
        self.last_event_size = len(data)
        self.total_event_size += self.last_event_size
        return True


if __name__=='__main__':
    import argparse
    parser=argparse.ArgumentParser(description='MOSS EUDAQ2 Producer',formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--run-control' ,'-r',default='tcp://localhost:44000')
    parser.add_argument('--name' ,'-n',default='MOSS_XXX')
    args=parser.parse_args()

    logging_to_eudaq(logging.INFO)

    myproducer=MOSSProducer(args.name,args.run_control)
    myproducer.Connect()
    while(myproducer.IsConnected()):
        sleep(1)
