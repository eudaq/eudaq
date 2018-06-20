#! /usr/bin/env python
# load binary lib/pyeudaq.so
import pyeudaq
import time
from collections import deque

class ExamplePyDataCollector(pyeudaq.DataCollector):
    def __init__(self, name, runctrl):
        pyeudaq.Producer.__init__(self, 'PyDataCollector', name, runctrl)
        print 'New instance of ExamplePyDataCollector'
        self.dict_con_ev = {}

    def DoInitialise(self):
        print 'DoInitialise'
        print 'key_a(init) = ', self.GetInitItem("key_a")

    def DoConfigure(self):
        print 'DoConfigure'
        print 'key_b(conf) = ', self.GetConfigItem("key_b")

    def DoStartRun(self):
        print 'DoStartRun'
        self.dict_con_ev = {}
        
    def DoReset(self):
        print 'DoReset'

    def DoConnect(self, con):
        t = con.GetType()
        n = con.GetName()
        r = con.GetRemote()
        print "new producer connection: ", t, n, "from ", r
        self.dict_con_ev[con] = deque()
        
    def DoDisconnect(self, con):
        t = con.GetType()
        n = con.GetName()
        r = con.GetRemote()
        print "delete producer connection: ", t, n, "from ", r
        del self.dict_con_ev[con]
        
    def DoReceive(self, con, ev):
        self.dict_con_ev[con].append(ev)
        for con, ev_queue in self.dict_con_ev.items():
            if(len(ev_queue)==0):
                return
            
        sycn_ev = Event('RawEvent', 'mysyncdata')
        for con, ev_queue in self.dict_con_ev.items():
            sub_ev = ev_queue.popleft()
            sync_ev.AddSubEvent(sub_ev)
            
        self.WriteEvent(ev)

        
if __name__ == "__main__":
    mydc = ExamplePyDataCollector("mydc", "tcp://localhost:44000")
    print "connecting to runcontrol in localhost:44000", 
    mydc.Connect()
    time.sleep(2)
    while(mydc.IsConnected()):
        time.sleep(1)
