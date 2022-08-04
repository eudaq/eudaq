#!/usr/bin/env python3

import subprocess
import argparse
import glob
import os
import re
import tqdm
import xml.etree.ElementTree as ET

EUCLIREADER = os.path.dirname(os.path.realpath(__file__))+"/../../../bin/euCliReader"

def read_raw_file(fpath):
    output = subprocess.check_output([EUCLIREADER,"-i", fpath],encoding='utf-8').strip()
    totev = int(re.findall("There are ([0-9]+)Events",output)[0])
    output = subprocess.check_output([EUCLIREADER,"-i", fpath,"-e","0","-E","100"],encoding='utf-8').strip()
    try:
        start_time = re.findall("<Tag>Time=(.*)</Tag>",output)[0]
    except IndexError:
        start_time = None
    try:
        conf = re.findall("Name = (.*.conf)\n",output)
        conf = list(set(conf))[0]
    except IndexError:
        conf = None
    try:
        ini = re.findall("Name = (.*.ini)\n",output)
        ini = list(set(ini))[0]
    except IndexError:
        ini = None
    output = subprocess.check_output([EUCLIREADER,"-i", fpath,"-e",str(max(totev-300,0)),"-E",str(totev)],encoding='utf-8').strip()
    tree = ET.fromstring("<top>"+output.replace("&","&amp;").replace("<<","&lt;&lt;").replace(">>","&gt;&gt;")+"</top>")
    trgN = {}
    evN = {}
    ntrgacc = {}
    end_time=None
    for event in tree:
        e = {k.tag: k.text for k in event}
        if "ITS3global"==e["Description"]:
            for subevent in next(k for k in event if k.tag=="SubEvents"):
                se = {k.tag: k.text for k in subevent}
                if 'Description' in se and 'TriggerN' in se:
                    trgN[se['Description']] = int(se['TriggerN'])
                    evN[se['Description']] = int(se['EventN'])
        elif 'ALPIDE' in e["Description"] and int(e["Flag"],16)==0x2:
            for tag in next(k for k in event if k.tag=="Tags"):
                if 'TRGMON_NTRGACC' in tag.text:
                    ntrgacc[e["Description"]]=int(tag.text.split("=")[1])
                if 'Time' in tag.text:
                    end_time=tag.text.split("=")[1]
    return totev,start_time,end_time,conf,ini,trgN,evN,ntrgacc

if __name__=="__main__":
    parser = argparse.ArgumentParser("Small script to help creating run lists")
    parser.add_argument("dir", help="Directory containing raw files.")
    parser.add_argument("--output-file", "-o", default=None, help="Path of the output file.")
    args = parser.parse_args()
    if args.output_file is None:
        args.output_file = os.path.join(args.dir,"run_list.csv")

    fout = open(args.output_file,'w')
    fout.write("run,start time,end time,ini file,conf file,total events,n triggers,n events,alpide ntrgacc\n")

    for run in tqdm.tqdm(glob.glob(os.path.join(args.dir,'*.raw'))):
        totev,st,et,conf,ini,trgN,evN,ntrgacc = read_raw_file(run)
        trgN = list(set(trgN.values()))[0]+1 if len(set(trgN.values()))==1 else str(trgN).replace(',','')
        evN = list(set(evN.values()))[0]+1 if len(set(evN.values()))==1 else str(evN).replace(',','')
        ntrgacc = list(set(ntrgacc.values()))[0] if len(set(ntrgacc.values()))==1 else str(ntrgacc).replace(',','')
        fout.write(f"{os.path.basename(run)},{st},{et},{ini},{conf},{totev},{trgN},{evN},{ntrgacc}\n")
        #print(os.path.basename(run),totev,st,et,conf,trgN)
        #print(trgN,ntrgacc)

    fout.close()
    print("Run list written to", args.output_file)