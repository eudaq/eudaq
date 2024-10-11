#!/usr/bin/env python3

from time import sleep
from datetime import datetime
from numpy.typing import NDArray
import pyeudaq
from pyeudaq import EUDAQ_INFO, EUDAQ_WARN
from triggerboard import TriggerBoard, find_trigger, DacAddrCh, DacVoltage
from triggerboard import TriggerConditionParser, CounterCondition
from utils import exception_handler
from spillcounter import SpillCounter

class TriggerProducer(pyeudaq.Producer):
    def __init__(self,name,runctrl):
        pyeudaq.Producer.__init__(self,name,runctrl)
        self.name = name
        self.tb: TriggerBoard = None
        self.is_running = False
        self.isev = 0
        self.last_status: str = None
        self.counts_last: NDArray = None
        self.read_interval = 2.  # seconds
        self.save_interval = 10. # seconds
        self.status_msg = ""
        self.sc: SpillCounter = None

    @exception_handler
    def DoInitialise(self):
        self.isev = 0
        conf: dict[str, str] = self.GetInitConfiguration().as_dict()
        port = conf.pop("port", "auto").lower()
        self.tb = TriggerBoard(find_trigger() if port == "auto" else port)
        for dac_name in DacAddrCh.keys():
            if dac_name in conf:
                dac = DacAddrCh(dac_name)
                val = DacVoltage(conf.pop(dac_name)).value
                EUDAQ_INFO(f"Setting {dac} to {val} V")
                self.tb.set_dac(val=val, channel=dac.channel, address=dac.address)
        sc_dict = {}
        for key in list(conf.keys()):
            if key.startswith("spill_counter_"):
                sc_dict[key.removeprefix("spill_counter_")] = int(conf.pop(key))
        self.sc = SpillCounter(**sc_dict)
        for key in conf.keys():
            EUDAQ_WARN(f"Unused key in .ini file: {key}")

    @exception_handler
    def DoConfigure(self):
        self.isev = 0
        conf: dict[str, str] = self.GetConfiguration().as_dict()
        trg = conf.pop("trg", None)
        veto = conf.pop("veto", None)
        cmd_sequence = TriggerConditionParser().get_cmd_sequence(
            trg if trg else None,
            veto if veto else None,
        )
        self.tb.write_trg_logic(cmd_sequence)
        count = conf.pop("count").split()
        self.count_conditions = [CounterCondition(c) for c in count]
        self.read_interval = float(conf.pop("read_interval", 2.)) # seconds
        for key in conf.keys():
            EUDAQ_WARN(f"Unused key in .conf file: {key}")

    @exception_handler
    def DoStartRun(self):
        self.is_running = True
        self.isev = 0
        self.counts_last = None
        self.status_msg = ""

    @exception_handler
    def DoStopRun(self):
        self.is_running = False
        self.status_msg = ""

    @exception_handler
    def DoReset(self):
        self.is_running = False

    @exception_handler
    def DoStatus(self):
        self.SetStatusTag("StatusEventN",str(self.isev))
        self.SetStatusTag("DataEventN"  ,"0")
        if self.status_msg:
            self.SetStatusMsg(self.status_msg)

    @exception_handler
    def RunLoop(self):
        self.sc.start()
        self.read_counts(init=True)
        tlast_read=datetime.now()
        tlast_save=datetime.now()
        self.send_status_event(tlast_read, bore=True)
        while self.is_running:
            if (datetime.now()-tlast_read).total_seconds() >= self.read_interval:
                self.read_counts()
                tlast_read=datetime.now()
            if (datetime.now()-tlast_save).total_seconds() >= self.save_interval:
                self.send_status_event(tlast_read)
                tlast_save=datetime.now()
            sleep(self.read_interval * 0.05)
        self.read_counts()
        self.send_status_event(datetime.now(), eore=True)

    def read_counts(self, init: bool = False):
        if init:
            self.counts_last = self.tb.latch_and_read_cnts()
            EUDAQ_INFO("  ".join(f"{c.name:>10s}" for c in self.count_conditions))
            return
        counts = self.tb.latch_and_read_cnts()
        dc = counts - self.counts_last
        self.counts_last = counts
        if not self.count_conditions:
            return
        EUDAQ_INFO(" ".join(f"{c.count(dc):10d}" for c in self.count_conditions))
        self.sc.update(self.count_conditions[0].count(dc))
        new_status = self.sc.get_status()
        if new_status != self.status_msg:
            self.status_msg = new_status
            if self.sc.cycle > 0:
                EUDAQ_INFO(new_status)

    def send_status_event(self, status_time: datetime, bore: bool = False, eore: bool = False):
        ev = pyeudaq.Event("RawEvent", self.name + "_status")
        ev.SetTag("Time", status_time.isoformat())
        ev.AddBlock(0, self.counts_last.tobytes())
        if bore:
            ev.SetTag("Port",self.tb.con.port)
            ev.SetBORE()
        if eore:
            ev.SetEORE()
        self.SendEvent(ev)
        self.isev+=1

if __name__=='__main__':
    import argparse
    parser = argparse.ArgumentParser(
        description="Trigger Producer",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    parser.add_argument("--run-control", "-r" , default="tcp://localhost:44000")
    parser.add_argument("--name", "-n", default="TRIGGER")
    args=parser.parse_args()

    myproducer=TriggerProducer(args.name, args.run_control)
    myproducer.Connect()
    while(myproducer.IsConnected()):
        sleep(1)

