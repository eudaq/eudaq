#!/usr/bin/env python3

import argparse
import pyeudaq
import mlr1daqboard
import numpy as np
import pathlib

parser=argparse.ArgumentParser(description='APTS data dumper')
parser.add_argument('input',help='EUDAQ raw file')
parser.add_argument('output',help='numpy file to dump events to')
parser.add_argument('--apts-id',default='APTS_0',help='ID of APTS in data (default: APTS_0)')
args=parser.parse_args()

fr=pyeudaq.FileReader('native',args.input)

evds =[]
trgs =[]
evns =[]
ts   =[]
while True:
    ev=fr.GetNextEvent()
    if ev is None: break
    sevs=ev.GetSubEvents()
    if sevs is None: break
    for sev in sevs:
        if sev.GetDescription()==args.apts_id:
            evd,t=mlr1daqboard.decode_apts_event(sev.GetBlock(0),decode_timestamps=True)
            ts.append(t)
            evds.append(evd)
            trgs.append(sev.GetTriggerN())
            evns.append(sev.GetEventN())
            break

np.savez(str(pathlib.PurePath(args.output).with_suffix('')) + '_ts_trg_evn',ts=ts,trgs=trgs,evns=evns)
evds = np.array(evds)
np.save(str(pathlib.PurePath(args.output).with_suffix('')),evds)
