#!/usr/bin/env python3
# Copied from APTSDump.py

import argparse
import pyeudaq
import mlr1daqboard
import numpy as np
import ujson
from os.path import exists
from os import remove
import time
import ROOT
import logging

def exec_time(start, end):
   diff_time = end - start
   m, s = divmod(diff_time, 60)
   h, m = divmod(m, 60)
   s,m,h = int(round(s, 0)), int(round(m, 0)), int(round(h, 0))
   print("Execution Time: " + "{0:02d}:{1:02d}:{2:02d}".format(h, m, s))

def hasSignal(array, thresholdSignal = 60):
    icount = 0
    maxcount = 100000
    array = array.tolist()
    for value in array:
        if icount > maxcount:
            break
        if value < float(thresholdSignal):
            return True
        icount += 1
    return False

def dump_data(inputFile, startEv, endEv, thresholdSignal):
    logging.info("Dumping data from file: " + inputFile)
    fr=pyeudaq.FileReader('native',inputFile)

    serverOPAMP = "OPAMP_0"

    evds =[]
    trgs =[]
    evns =[]
    ts   =[]
    osch1s = []
    osch2s = []
    osch3s = []
    osch4s = []
    header_ch1 = None
    header_ch2 = None
    header_ch3 = None
    header_ch4 = None
    while True:
        ev=fr.GetNextEvent()
        if ev is None: break
        sevs=ev.GetSubEvents()
        if sevs is None: break
        logging.debug(f"Event: {ev.GetEventN()}")
        if ev.GetEventN() < int(startEv):
            logging.debug(f"Skipping event: {ev.GetEventN()}")
            continue
        if ev.GetEventN() > int(endEv):
            logging.debug(f"Skipping event: {ev.GetEventN()}")
            break
        for sev in sevs:
            if sev.GetDescription()==serverOPAMP:
                logging.debug(f"Server OPAMP")
                evd,t=mlr1daqboard.decode_apts_event(sev.GetBlock(0),decode_timestamps=True)
                osch1 = np.frombuffer(sev.GetBlock(2), dtype=np.int8)
                osch2 = np.frombuffer(sev.GetBlock(3), dtype=np.int8)
                osch3 = np.frombuffer(sev.GetBlock(4), dtype=np.int8)
                osch4 = np.frombuffer(sev.GetBlock(5), dtype=np.int8)

                if header_ch1 is None:
                    header_ch1 = "5"
                if header_ch2 is None:
                    header_ch2 = "6"
                if header_ch3 is None:
                    header_ch3 = "9"
                if header_ch4 is None:
                    header_ch4 = "10"
                
                ts.append(t)
                evds.append(evd)
                trgs.append(sev.GetTriggerN())
                evns.append(sev.GetEventN())
                osch1s.append(osch1)
                osch2s.append(osch2)
                osch3s.append(osch3)
                osch4s.append(osch4)

    oschs = [osch1s, osch2s, osch3s, osch4s]
    headers = [header_ch1, header_ch2, header_ch3, header_ch4]

    dout = []
    selectedEvents = []
    for iEvt in range(len(evns)):
        logging.debug(f"Processing Event: {evns[iEvt]}")
        tempDict = {"EventN": iEvt}
        tempDict[headers[0]] = oschs[0][iEvt].tolist()
        tempDict[headers[1]] = oschs[1][iEvt].tolist()
        tempDict[headers[2]] = oschs[2][iEvt].tolist()
        tempDict[headers[3]] = oschs[3][iEvt].tolist()
        tempDict['ADCs'] = evds[iEvt].tolist()
        saveSignal = False
        if thresholdSignal != 0:
            logging.debug(f"Threshold: {thresholdSignal}")
            ich = 0
            for osch in oschs:
                if hasSignal(osch[iEvt], thresholdSignal):
                    logging.debug(f"Signal found in channel {headers[ich]}")
                    saveSignal = True
                    break
                ich += 1
        else:
            saveSignal = True
        if saveSignal:
            logging.debug(f"Saving event: {evns[iEvt]}")
            selectedEvents.append(evns[iEvt])
            dout.append(tempDict)
    logging.debug(f"selectedEvents: {selectedEvents}")
    return dout

def save_root_file(fOutputName, data):
    outfile = ROOT.TFile(fOutputName, "RECREATE")
    outfile.cd()
    for d in data:
        for key in d.keys():
            if key == "EventN":
                continue
            if key == 'ADCs':
                px = 0
                for i in range(4):
                    for j in range(4):
                        if px == 5 or px == 6 or px == 9 or px == 10: 
                            px = px+1
                            continue
                        else:
                            wf = np.array(d[key][i][j],dtype=np.float64)*0.0381
                            tp = np.arange(len(d[key][i][j]), dtype=np.float64)*250000
                            tempGr = ROOT.TGraph(len(d[key][i][j]), tp, wf)
                            tempGr.SetName(f"grEv{d['EventN']}Px{px}samp250000")
                            tempGr.Write()
                            px = px+1
            else:
                wf = np.array(d[key],dtype=np.float64)*0.625 #*1000. #
                tp = np.arange(len(d[key]), dtype=np.float64)*25
                tempGr = ROOT.TGraph(len(d[key]), tp, wf)
                tempGr.SetName(f"grEv{d['EventN']}Px{key}samp25")
                tempGr.Write()
    outfile.Close()

def save_json_file(fOutputName, data):
    with open(fOutputName, 'w') as f:
        ujson.dump(data, f)

if __name__=="__main__":
    parser=argparse.ArgumentParser(description='OPAMP data dumper')
    parser.add_argument('input',help='EUDAQ raw file')
    parser.add_argument('output',help='csv file to dump events to')
    parser.add_argument('--start',default=0,help='First event number to dump (Default: 0)')
    parser.add_argument('--end',default=999999,help='Last event number to dump (Default: almost infinity)')
    parser.add_argument('--threshold',default=0,help='Preselection based on the threshold (Default: 0)')
    parser.add_argument('--output-type', choices=['json','root'], default='root', help='Output type (Default: root)')
    args=parser.parse_args()

    fOutputName = f"{args.output}"
    if int(args.start) > 0: fOutputName += f"_{args.start}"
    if args.output_type == 'root': fOutputName += ".root"
    else: raise ValueError(f"Unknown output type: {args.output_type}")

    logging.info("Output file: " + fOutputName)

    if exists(fOutputName):
        Question = input("overwrite? (y/n)")
        if Question == "y":
            remove(fOutputName)
        else:
            logging.info("Aborting")
            exit(0)

    stime = time.time()
    dout = dump_data(args.input, args.start, args.end, args.threshold)
    if args.output_type == "json":
        save_json_file(fOutputName, dout)
    elif args.output_type == "root":
        save_root_file(fOutputName, dout)
    etime = time.time()
    exec_time(stime,etime)