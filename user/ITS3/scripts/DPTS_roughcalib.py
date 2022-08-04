#!/usr/bin/env python3

import numpy as np
import matplotlib.pyplot as plt
import simplejson as json
import argparse

parser=argparse.ArgumentParser()
parser.add_argument('threshold',type=float)
parser.add_argument('gid_min',type=float)
parser.add_argument('gid_max',type=float)
parser.add_argument('pid_min',type=float)
parser.add_argument('pid_max',type=float)
parser.add_argument('--channel','-c',choices=[0,1],type=int)
parser.add_argument('-x',default=False,action='store_true')
parser.add_argument('calibration_file',type=argparse.FileType('w'),help='file to write calibration data to.')
args=parser.parse_args()

chs=[args.channel] if args.channel is not None else [0,1]

f=args.calibration_file
for ch in chs:
    i=0
    f.write(f'threshold_ch{ch} = {args.threshold}\n')
    for igid,gid in enumerate(np.linspace(args.gid_min,args.gid_max,32)):
        for ipid,pid in enumerate(np.linspace(args.pid_min,args.pid_max,32)):
            x,y=igid,ipid
            if x%4>=2:
                y=31-y
            if args.x and y%2==1:
                x=x//2*2+(1-x%2)
            for slope in ['a','d']:
                f.write(f'gidpid2xy_ch{ch}_{slope}_{i}_gid = {gid}\n')
                f.write(f'gidpid2xy_ch{ch}_{slope}_{i}_pid = {pid}\n')
                f.write(f'gidpid2xy_ch{ch}_{slope}_{i}_x   = {x}\n')
                f.write(f'gidpid2xy_ch{ch}_{slope}_{i}_y   = {y}\n')
            i+=1
 
