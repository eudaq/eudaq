#!/usr/bin/env python3

# General information:
# Maintainer:  Pascal BECHT
# Contact:     pascal.becht@cern.ch
# Description: Simple script to translate measured DPTS position calibration data to EUDAQ2 calibration_file format.

import numpy as np
import argparse
from argparse import RawTextHelpFormatter

def check_args(input): # False if problems with an argument
    # check input parameters
    if len(input)>4:
        print("Only input for 4 configurations (2 channels * 2 trains) allowed for the moment.")
        return False
    ch_trains=[c+'-'+t for c in ['0','1'] for t in ['a','d']]
    n_ch_train=[0]*len(ch_trains)
    given=[]
    for stage,params in enumerate(input):
        ch_train=params[0]+'-'+params[3]
        try:
            n_ch_train[ch_trains.index(ch_train)]+=1
            given.append(ch_train)
        except ValueError:
            print("Input ",stage+1, ". Choose channel-train combination from ",ch_trains,". Given ",ch_train)
            return False
    if max(n_ch_train)>1:
        print("Specify the same channel-train combination only once. Given ",given)
        return False

    return True

if __name__=="__main__":
    parser=argparse.ArgumentParser(description="Simple script to translate measured DPTS position calibration data to EUDAQ2 calibration_file format.\nMaintainer: Pascal Becht.\nIn case of questions or problems, please contact pascal.becht@cern.ch.",
    formatter_class=RawTextHelpFormatter)
    parser.add_argument('-i','--input',type=str,action='append',nargs=6,metavar=('channel','threshold','calibration_file','train', 'gid_scaling', 'pid_scaling'),
                        help='<channel number (0/1)>\n<threshold value (DAC)>\n<calibration file path (.npy)>\n<rising/falling edge train (a/d)>\n<gid scaling factor>\n<pid scaling factor>.\nTo be called separately for each configuration.\nExample: DPTS_calib.py -i 0 30 ./calibration_dpts1.npy a 1.0 1.0 \n-i 1 30 ./calibration_dpts2.npy a 1.05 1.05 \n-o test.calib')
    parser.add_argument('-o','--output',type=argparse.FileType('w'),metavar='output_file',help='File to write calibration data to.')
    parser.add_argument('-v',default=False,action='store_true',help="Enable verbose mode")
    args=parser.parse_args()

    if args.input is None or args.output is None:
        print("Either 'input' or 'output' or both unspecified. Nothing to do.")
        raise SystemExit(0)
    if not check_args(args.input):
        print("Argument error.")
        raise SystemExit(0)

    f=args.output

    ch_single_write=[]
    for stage,params in enumerate(args.input):
        ch=int(params[0])
        thresh=int(params[1])
        gid_scaling=float(params[4])
        pid_scaling=float(params[5])
        if not ch in ch_single_write:
            f.write(f'threshold_ch{ch} = {thresh}\n')
            ch_single_write.append(ch)
        try:
            calib_array=np.load(params[2])
            slope=params[3]
            if not slope=='a' and not slope=='d':
                print("Encountered invalid slope argument.")
                raise SystemExit(1)
            #print(calib_array)
            i=0 # total pixel number
            for (col,row) in np.ndindex(calib_array.shape[:2]):
                gid=calib_array[col][row][0]*1e9*gid_scaling
                pid=calib_array[col][row][1]*1e9*pid_scaling
                if args.v:
                    print("COL: ",col)
                    print("ROW: ",row)
                    print("GID: ",gid)
                    print("PID: ",pid)
                    print(" ")
                # write the found values to calibration file in the right format
                f.write(f'gidpid2xy_ch{ch}_{slope}_{i}_gid = {gid}\n')
                f.write(f'gidpid2xy_ch{ch}_{slope}_{i}_pid = {pid}\n')
                f.write(f'gidpid2xy_ch{ch}_{slope}_{i}_x   = {col}\n')
                f.write(f'gidpid2xy_ch{ch}_{slope}_{i}_y   = {row}\n')
                i+=1
        except Exception as e:
            print(e.message, e.args)
            raise Exception("Make sure you passed the correct .npy calibation files for the channels.") from e

    if args.v:
        print("The following arguments have been parsed:")
        print("Input parameters:")
        for stage,params in enumerate(args.input):
            print("    Stage: ", stage)
            print("    Channel: ", params[0])
            print("    Threshold: ", params[1], " DAC")
            print("    Calibration file: ", params[2])
            print("    Train type: ", params[3])
            print("    GID scaling: ", params[4])
            print("    PID scaling: ", params[5])
            print(" ")
        print("Output file:")
        print("    ", str(args.output))
    raise SystemExit(0)
