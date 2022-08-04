#!/usr/bin/env python3

import argparse, os
import pyeudaq
import numpy as np
from pathlib import Path


parser=argparse.ArgumentParser(description='DPTS data dumper')
parser.add_argument('filename', help='eudaq .raw filepath')
parser.add_argument('dpts', help='EUDAQ ID of the picoscope producer (example: "DPTS" or "DPTS_0")')
parser.add_argument('--threshold','-t',type=int,default=18,help='only dump events with signals above this threshold')
parser.add_argument('-i',type=int,default=0,help='Event offset')
parser.add_argument('-n',type=int,default=1,help='Number of events to look at. Dump all the events if 0')
parser.add_argument('--output-dir','-o',type=str,default="./dumped_waveforms",help='output directory path where to save dumped waveforms')
args=parser.parse_args()


# extract the run from the file name and create a folder where to save dumped waveforms (if it does not exist)
parent_folder = args.output_dir
if not os.path.isdir(parent_folder):
    os.makedirs(parent_folder)
data_folder = parent_folder + r"/{}".format(Path(args.filename).stem)
if not os.path.isdir(data_folder):
    os.makedirs(data_folder)
# read the .raw file and then extract the waveforms
fr=pyeudaq.FileReader('native',args.filename)
# skip some events as passed by the argument -i
for _ in range(args.i):  # _ can be used as a variable in looping
    fr.GetNextEvent()
# if n=0 dump all the events over thr, otherwise dump only the events over thr found in the first n frames
counter = args.n if args.n else -1
while counter != 0:
    try:
        ev=fr.GetNextEvent()
        sevs=ev.GetSubEvents()
        if sevs is None: break
        for sev in sevs:
            if sev.GetDescription()==args.dpts:
                e=sev.GetBlock(0)
                d=np.frombuffer(e,dtype=np.int8)
                if args.threshold is None or np.max(d)>args.threshold:
                    d.shape=(2,len(d)//2)
                    np.save(data_folder + '/dump%04d.npy'%ev.GetEventN(),d)
        counter -= 1
    except AttributeError as ae:
        print("All the events in {} are dumped".format(args.filename))
        break