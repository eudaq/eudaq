#!/usr/bin/env python3

import argparse, os
import pyeudaq
import numpy as np
from pathlib import Path
import logging

def DPTSDump(filename,dpts_name,threshold,event_offset,n_events,output_dir):
    # extract the run from the file name and create a folder where to save dumped waveforms (if it does not exist)
    parent_folder = output_dir
    if not os.path.isdir(parent_folder):
        os.makedirs(parent_folder)
    data_folder = parent_folder + r"/{}".format(Path(filename).stem)
    if not os.path.isdir(data_folder):
        os.makedirs(data_folder)
    logging.info("Created outdir and outdir to save dumped waveforms : "+output_dir+", "+data_folder)
    # read the .raw file and then extract the waveforms
    fr=pyeudaq.FileReader('native',filename)
    # skip some events as passed by the argument -i
    for _ in range(event_offset):  # _ can be used as a variable in looping
        fr.GetNextEvent()
    # if n=0 dump all the events over thr, otherwise dump only the events over thr found in the first n frames
    counter = event_offset if event_offset else -1
    logging.info("Processing events in {} ...".format(filename))
    while counter != 0:
        try:
            ev=fr.GetNextEvent()
            sevs=ev.GetSubEvents()
            if sevs is None: break
            for sev in sevs:
                if sev.GetDescription()==dpts_name:
                    e=sev.GetBlock(0)
                    d=np.frombuffer(e,dtype=np.int8)
                    if threshold is None or np.max(d)>threshold:
                        d.shape=(2,len(d)//2)
                        np.save(data_folder + '/dump%04d.npy'%ev.GetEventN(),d)
            counter -= 1
        except AttributeError as ae:
            logging.info("All the events in {} are dumped".format(filename))
            break

if __name__ == "__main__":
    parser=argparse.ArgumentParser(description='DPTS data dumper')
    parser.add_argument('filename', help='eudaq .raw filepath')
    parser.add_argument('dpts_name', help='EUDAQ ID of the picoscope producer (example: "DPTS" or "DPTS_0")')
    parser.add_argument('--threshold','-t',type=int,default=18,help='only dump events with signals above this threshold')
    parser.add_argument('--event_offset','-i',type=int,default=0,help='Event offset')
    parser.add_argument('--n_events','-n',type=int,default=0,help='Number of events to look at. Dump all the events if 0')
    parser.add_argument('--output-dir','-o',type=str,default="./dumped_waveforms",help='output directory path where to save dumped waveforms')
    args=parser.parse_args()

    logging.basicConfig(level=logging.DEBUG, format='%(asctime)s - %(levelname)s - %(message)s')
    DPTSDump(args.filename,args.dpts_name,args.threshold,args.event_offset,args.n_events,args.output_dir)