#!/usr/bin/env python3

# Interface
import argparse, subprocess, re
# Event
import pyeudaq
import numpy as np
from tqdm import tqdm
import signal as sys_signal

# Constants and Options
NX, NY, N_FRAME     = 64, 32, 9
FIXED_TRIGGER_FRAME = 2
FIXED_TRIGGER_WIDTH = 1
SUBMATRIX_N         = 3
SUBMATRIX_EDGE      = [21, 42, 64]
SUBMATRIX_POLARITY  = ['+', '+', '-']
SUBMATRIX_CUT       = [1500, 1800, 500]
SIGNAL_METHOD       = ['cds', 'max', 'fix', 'fix_diff','fix_max']
N_EVENTS_MAX        = int(1e6)

# Arguments
parser = argparse.ArgumentParser(description='CE65 data dumper')
parser.add_argument('input', help='EUDAQ raw file')
parser.add_argument(
    '-o', '--output', help='numpy file to dump events (EV,NX,NY,FR)', default='ce65_dump')
parser.add_argument('--dut-id', dest='dut_id', default='CE65Raw',
                    help='ID of CE65 in data')
parser.add_argument('-e', '--nev', default=1000000,
                    type=int, help='N events to read')
parser.add_argument('-s', '--signal', default='cds',
                    type=str, choices=SIGNAL_METHOD, help='Signal extraction method')
parser.add_argument('-n', '--noise', default=False,
                    help='Noise run - no signal cuts', action='store_true')
parser.add_argument('-a', '--all', default=False,
                    help='All events - no signal cuts', action='store_true')
parser.add_argument('-d', '--dump', default=False,
                    help='save np.array into file', action='store_true')
parser.add_argument('-v', '--debug', default=False,
                    help='Print debug info.', action='store_true')
parser.add_argument('--qa', default=False,
                    help='Process QA result.', action='store_true')
parser.add_argument('--cut', default='1500,1800,500',
                    help='Simple threshold for each submatrix')
parser.add_argument('--skip', default=0,
                    help='Skip total events in raw file')

args = parser.parse_args()

fr = pyeudaq.FileReader('native', args.input)

def eudaqGetNEvents(rawDataPath):
  eudaqExe = 'euCliReader'
  cmd = [eudaqExe,"-i",rawDataPath,"-e", "10000000"]
  try:
    result = subprocess.run(cmd, stdout=subprocess.PIPE)
    cfg = re.findall(r'\d+',result.stdout.decode('utf-8'))
    print(f'[-] INFO - {cfg[0]} events found by {eudaqExe}')
    return int(cfg[0])
  except FileNotFoundError:
    print(f'[X] {eudaqExe} not found, set max. events {N_EVENTS_MAX}')
    return N_EVENTS_MAX

# N events
eudaq_nev = eudaqGetNEvents(args.input)
if(args.nev < 0):
  args.nev = min(eudaq_nev, N_EVENTS_MAX)
else:
  args.nev = min(args.nev, eudaq_nev)

# Define baseline for subtraction
def baseline(frdata):
  #sumRegion = sum(frdata[0:N_BASELINE])
  # return (sumRegion / N_BASELINE)
  return frdata[0]

# Signal extraction from analogue wavefrom by each pixel
# Submatrix-SF Negative signal (min)
def findMaxChargeFrame(frdata, pol='+'):
  if(pol == '+'):
    return np.argmax(frdata)
  elif(pol == '-'):
    return np.argmin(frdata)
  else:
    print('[X] ERROR - UNKNOWN signal polarity : ' + pol)
  return 0
def signalAmp(frdata, opt='cds', pol='+'):
  val, ifr = 0, 0
  opt = opt.lower().strip()
  # CDS (last - 1st)
  if(opt == 'cds'):
    val = frdata[-1] - frdata[0]
    ifr = len(frdata) -1
  # MAX (max - baseline)
  elif(opt == 'max'):
    ifr = findMaxChargeFrame(frdata, pol)
    val = frdata[ifr] - baseline(frdata)
  # FIX (TriggerFrame - baseline)
  elif(opt == 'fix'):
    ifr = FIXED_TRIGGER_FRAME
    val = frdata[ifr] - baseline(frdata)
  # FAX - MAX in FIXED trigger window +/- 1
  elif(opt == 'fix_diff'):
    ifr = FIXED_TRIGGER_FRAME
    val = frdata[ifr] - frdata[ifr-1]
  elif(opt == 'fix_max'):
    lower = max(0, FIXED_TRIGGER_FRAME-FIXED_TRIGGER_WIDTH)
    upper = min(len(frdata), FIXED_TRIGGER_FRAME+FIXED_TRIGGER_WIDTH+1)
    trigWindow = frdata[lower:upper]
    ifr = lower + findMaxChargeFrame(trigWindow, pol)
    val = frdata[ifr] - baseline(frdata)
  else:
    print(f'[X] UNKNOWN signal extraction method - {opt}')
  return (val, ifr)

def eventCut(evdata):
  # ADCu = val - baseline [1st frame]
  eventPass = False
  sigMax, frMax = 0, 0
  submatrixIdx = 0
  for ix in range(NX):
    if(ix > SUBMATRIX_EDGE[submatrixIdx]):
      submatrixIdx += 1
    for iy in range(NY):
      frdata = list(evdata[ix][iy])
      pol = SUBMATRIX_POLARITY[submatrixIdx]
      val, ifr = signalAmp(frdata, args.signal, pol)
      if(abs(val) > SUBMATRIX_CUT[submatrixIdx] and ifr == findMaxChargeFrame(frdata, pol)):
        eventPass = True
  return eventPass

if(args.qa):
  from ROOT import TFile, TH2F
  global h2qa, qaOut
  h2qa = {}
  qaOut = TFile(args.output + '-qa.root','RECREATE')
  for sigTag in SIGNAL_METHOD:
    h2qa[sigTag] = TH2F(f'h2qa_{sigTag}',f'Noise distribution (method={sigTag});Pixel ID;ADCu;#ev',
      NX*NY, -0.5, NX*NY-0.5,
      4000, -2000, 2000)
def analogue_qa(evdata):
  for ix in range(NX):
    for iy in range(NY):
      iPx = iy + ix * NY
      for sigTag in SIGNAL_METHOD:
        val, ifr = signalAmp(list(evdata[ix][iy]), sigTag)
        h2qa[sigTag].Fill(iPx, val)

def decode_event(raw):
    global N_FRAME
    nFrame = raw.GetNumBlock()
    if(nFrame != N_FRAME):
      print(f'[X] WARNING - Number of frames changes from {N_FRAME} to {nFrame}')
      N_FRAME = nFrame
    evdata = np.empty((NX, NY, N_FRAME),dtype=np.short)
    for ifr in range(nFrame):
        rawfr = raw.GetBlock(ifr)
        assert(len(rawfr) == 2*NX*NY)
        for ix in range(NX):
            for iy in range(NY):
                iPx = iy + ix * NY
                # uint8_t *2 => short
                try:
                  evdata[ix][iy][ifr] = np.short(int(rawfr[2*iPx+1]<<8) + int(rawfr[2*iPx]))
                except OverflowError: # sign flip
                  pass
    return evdata

cmd_STOP = False
def stop_event(sig_num, frame):
  global cmd_STOP
  cmd_STOP = True
  print('[+] STOP event processing by CTRL+C')

sys_signal.signal(sys_signal.SIGINT, stop_event)

evds = []
nEvent_DUT = 0
nEvent_Pass = 0
EV_TRIGGER_DIFF = 0
evNo, trigNo = 0, 0
subName = ''
if(args.skip > 0):
  print(f'[-] Skip {args.skip} total events in RAW file (not data events)')
  for _ in range(args.skip):
    ev = fr.GetNextEvent()
for iev in tqdm(range(args.nev)):
  ev = fr.GetNextEvent()
  if ev is None: break
  sevs = ev.GetSubEvents()
  if sevs is None: break
  for sev in sevs:
    # Event number check (mismatch during data taking)
    if(args.debug and abs(sev.GetTriggerN() - trigNo) > EV_TRIGGER_DIFF):
      print(f'[X] WARNING : Event/Trigger number - {subName} {evNo}/{trigNo}, {sev.GetEventN()}/{sev.GetTriggerN()} - {sev.GetDescription()}')
      EV_NUMBER_DIFF = abs(sev.GetTriggerN() - trigNo)
    if sev.GetDescription() != args.dut_id:
      continue
    else:
      subName = sev.GetDescription()
      evNo = sev.GetEventN()
      trigNo = sev.GetTriggerN()
    # Process raw data from DUT sub-event
    nEvent_DUT += 1
    evdata = decode_event(sev)
    # DEBUG
    if(args.noise or args.all or eventCut(evdata)):
      nEvent_Pass += 1
      if(args.dump): evds.append(evdata)
      if(args.debug):
        print('[+] DEBUG - CE65 events found')
    if(args.qa):
      analogue_qa(evdata)
    break
  if(cmd_STOP): break

if(args.dump): np.save(args.output, evds)

if(args.qa):
  for sigTag in SIGNAL_METHOD:
    h2qa[sigTag].SetDrawOption("colz")
    h2qa[sigTag].Write()
  qaOut.Close()
print(f'[+] CE65 event found : {nEvent_DUT}')
print(f'[+] CE65 event pass cut : {nEvent_Pass}')