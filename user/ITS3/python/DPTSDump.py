#!/usr/bin/env python3

import argparse
import pyeudaq
import numpy as np
from tqdm import tqdm

parser=argparse.ArgumentParser(description='DPTS data dumper')
parser.add_argument('filename')
parser.add_argument('--threshold','-t',type=int,help='only dump events with signals above this threshold')
parser.add_argument('-i',type=int,default=0,help='Event offset')
parser.add_argument('-n',type=int,default=1,help='Number of events to look at')
args=parser.parse_args()

fr=pyeudaq.FileReader('native',args.filename)

for _ in range(args.i):
    fr.GetNextEvent()
for _ in tqdm(range(args.n)):
    ev=fr.GetNextEvent()
    sevs=ev.GetSubEvents()
    if sevs is None: break
    for sev in sevs:
        if sev.GetDescription()=='DPTS':
            e=sev.GetBlock(0)
            d=np.frombuffer(e,dtype=np.int8)
            if args.threshold is None or np.max(d)>args.threshold:
                d.shape=(2,len(d)//2)
                np.save('dump%04d.npy'%ev.GetEventN(),d)

