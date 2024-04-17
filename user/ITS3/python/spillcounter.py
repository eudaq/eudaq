#!/usr/bin/env python3

import time
import numpy as np
from numpy.typing import NDArray

class SpillCounter:
    def __init__(self,
                 sampling_period_s: int = 120,
                 bucket_size_ms: int = 2000,
                 spill_separation: int = 3,
                 threshold: int = 0
        ):
        self.spill_sep = spill_separation # number of buckets that separate spills
        self.threshold = threshold # min. number of counts in bucket to consider it a spill (noise suppression)
        self.spills: list[NDArray] = []
        self.cycle = 0
        self._last_cycle_with_spill = -1
        self._bucket_size_ms = bucket_size_ms
        self._bucket_size_ns = self._bucket_size_ms*1000000
        self._buckets = np.zeros(
            (int(sampling_period_s*1000/self._bucket_size_ms),), dtype=int)
        self._sampling_period_ms = self._bucket_size_ms * self._buckets.size
        self._st = time.perf_counter_ns()

    def start(self):
        self._buckets.fill(0)
        self._st = time.perf_counter_ns()

    def update(self,n=1):
        b = int((time.perf_counter_ns()-self._st)/self._bucket_size_ns)
        if b >= self._buckets.size:
            self._process()
            if len(self.spills):
                self._last_cycle_with_spill = self.cycle
            self.start()
            b=0
            self.cycle += 1
        self._buckets[b] += n
    
    def _process(self):
        self.spills = []
        ss = -1
        b = 0
        while b < self._buckets.size:
            if ss < 0 and self._buckets[b] > self.threshold: ss = b
            elif np.all(self._buckets[b:b+self.spill_sep] <= self.threshold):
                if ss >= 0:
                    self.spills.append(self._buckets[ss:b].copy())
                    ss = -1
                b += self.spill_sep
            b += 1
        if ss>=0:
            self.spills.append(self._buckets[ss:])

    def get_status(self, duration_info: bool = False):
        self.update(0)
        if self.cycle == 0:
            perc = (time.perf_counter_ns()-self._st)/self._bucket_size_ns/self._buckets.size*100.
            status = f"Acquiring data {perc:.1f}%"
        elif len(self.spills)==0:
            n_cycles_wo_spill = self.cycle-self._last_cycle_with_spill-1
            n_min_wo_spill = n_cycles_wo_spill*self._sampling_period_ms/60000.
            status = f"No spills in last {n_cycles_wo_spill} cycle(s)"
            status += f" ({n_min_wo_spill:.1f} minutes)"
        elif len(self.spills)==1 and self.spills[0].size==self._buckets.size:
            eps = round(np.sum(self.spills[0])*1000./self._sampling_period_ms, 1)
            status = f"Continuous spill with {eps} events/s"
        else:
            spm = round(len(self.spills)*60000./self._sampling_period_ms, 1)
            pps = round(sum([np.sum(s) for s in self.spills])/len(self.spills), 1)
            status = f"{spm} spills/min"
            if duration_info:
                dur = round(np.mean([s.shape[0] for s in self.spills])*self._bucket_size_ms*0.001, 1)
                status += f" | {pps} events per <{dur} s> spill"
            else:
                status += f" | {pps} events/spill"
        return status


def test():
    # test cases
    sc=SpillCounter(bucket_size_ms=1000)
    sc._buckets=np.array([0,2,0,0,0,0,1,0,7,0,1,0,0,0,0,1],dtype=int)
    print(sc._buckets,sc._process(),sc.get_status(True))
    sc.spill_sep=5
    print(sc._buckets,sc._process(),sc.get_status(True))
    sc._buckets=np.array([0,0,0,0,2,0,0,0,0,1,0,7,0,1,0,0],dtype=int)
    print(sc._buckets,sc._process(),sc.get_status(True))
    sc=SpillCounter(bucket_size_ms=1000)
    sc._buckets=np.ones((16,),dtype=int)
    print(sc._buckets,sc._process(),sc.get_status(True))
    sc._buckets=np.zeros((16,),dtype=int)
    print(sc._buckets,sc._process(),sc.get_status(True))

    sc=SpillCounter(sampling_period_s=2, bucket_size_ms=200)
    loop=True
    while(loop):
        sc.update(int(input(sc.get_status(True)+" ?")))

if __name__=="__main__":
    from triggerboard import TriggerBoard, find_trigger, CounterCondition
    import argparse

    parser=argparse.ArgumentParser(description='Detect spill structure from trigger board counters')
    parser.add_argument('--port','-p',help='path to serial port', default=find_trigger())
    parser.add_argument('--sampling_period', '-s', default=60, type=int, help='Samplign period in seconds (default 60).')
    parser.add_argument('condition',type=CounterCondition,help='condition to be counted (examples: "xxx1", "xxxR", "x(01)x1")')
    args = parser.parse_args()

    sc = SpillCounter(sampling_period_s=args.sampling_period, bucket_size_ms=50, threshold=3)
    cycle = 0 
    tb=TriggerBoard(args.port)
    nl = tb.latch_and_read_cnts()
    
    while True:
        n = tb.latch_and_read_cnts()
        sc.update(args.condition.count(n-nl))
        nl = n
        print("\r", sc.get_status(True), " "*30, end="")
        time.sleep(0.005)
