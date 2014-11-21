#!/usr/bin/env python

import curses
import time
from subprocess import *
import os

import smtplib
from email.mime.text import MIMEText

from watchdog_config import *

PADS=[
  {'x': 5, 'y': 2,'name':'Run Control','bit':0},
  {'x': 20,'y': 2,'name':'Last Event Recorded','bit':0},
  {'x': 35,'y': 2,'name':'File Size','bit':0},
  {'x': 50,'y': 2,'name':'Out of Sync','bit':0},
]

def get_status(scr):
  status = call("pgrep euRun.exe > /dev/null 2> /dev/null", shell=True)
  PADS[0]['bit'] = (status == 0);
  
  last_run = os.popen("ls data/*.raw | tail -n 1").read().rstrip()
  mod_time = os.stat(last_run).st_mtime
  now = time.time()
  mod_time_ago = now - mod_time;
  
  scr.addstr(11, 0, "Current run: %s" % last_run)
  
  PADS[1]['bit'] = (mod_time_ago < LAST_EVENT_TIME);
  PADS[1]['info'] = "%dsec" % int(mod_time_ago)
  
  file_size = mod_time = os.stat(last_run).st_size
  PADS[2]['bit'] = (file_size < FILE_SIZE_LIMIT);
  PADS[2]['info'] = "%dMB" % int(file_size / 1024 / 1024)
  
  last_log = os.popen("ls logs/*.log | tail -n 1").read().rstrip()
  out_of_syncs = os.popen("grep 'Out of sync' %s | tail -n 200" % last_log).read()
  
  scr.addstr(12, 0, "Current log file: %s" % last_log)

  # WARN    Event 831. Out of sync: Timestamp of current event (device 0) is 2048104 while smallest is 360. Excluding layer.        2014-11-15 17:39:23.454 Producer.pALPIDEfs      /home/palpidefs/eudaq/producers/palpidefs/src/PALPIDEFSProducer.cxx:912 int PALPIDEFSProducer::BuildEvent()
  #[2] is date, [1] is text
  
  count=0
  for i in out_of_syncs.split('\n'):
    words = i.split('\t')
    if (len(words) > 2):
      log_time = time.mktime(time.strptime(words[2], "%Y-%m-%d %H:%M:%S.%f"))
      if (now - log_time < OUT_OF_SYNC_TIME):
	count+=1
  
  PADS[3]['bit'] = (count < OUT_OF_SYNC_LIMIT)
  PADS[3]['info'] = "%s%d (%ssec)" % (">=" if (count == 200) else "", count, OUT_OF_SYNC_TIME)
  
def print_pad(scr,pad):
    x=pad['x']
    y=pad['y']
    name=pad['name']
    if ('info' in pad):
      name+="  "+pad['info']
    
    if pad['bit']:
        style=curses.color_pair(2)
    else:
        style=curses.color_pair(1)

    for i in range(8):
      for j in range(10):
        scr.addstr(y+i,x+j," ",style)
        
    i=0
    y+=1
    for c in name:
      scr.addstr(y,x+i,c,style)
      if (c == " "):
	i=0
	y+=1
      else:
	i+=1

def print_alive(scr):
    print_alive.n=(print_alive.n+1)%59
    for x in xrange(59):
         scr.addstr(23,3+x,'-' if print_alive.n==x else ' ')
    scr.addstr(23,2,'[')
    scr.addstr(23,62,']')

print_alive.n=0

def send_alert():

    text = ""
    for pad in PADS:
      if (pad['bit'] == 0):
	text += "%s: STATUS %d (%s)\n" % (pad['name'], pad['bit'], pad['info'] if 'info' in pad else "")

    for target in ALERT_TARGET:
      msg = MIMEText(text)

      msg['Subject'] = "pALPIDEfs Watchdog: STATUS %d %d %d %d" % (PADS[0]['bit'], PADS[1]['bit'], PADS[2]['bit'], PADS[3]['bit'])
      msg['From'] = "noreply@cern.ch"
      msg['To'] = target

      # Send the message via our own SMTP server, but don't include the
      # envelope header.
      s = smtplib.SMTP('cernmx.cern.ch')
      #s.set_debuglevel(True)
      s.sendmail(msg['From'], [ target ], msg.as_string())
      s.quit()
  

def main(stdscr):
    curses.resizeterm(25,80)
    stdscr.nodelay(1)
    curses.init_pair(1,curses.COLOR_WHITE,curses.COLOR_RED)
    curses.init_pair(2,curses.COLOR_BLACK,curses.COLOR_GREEN)

    not_ok_timestamp = -1
    alert_sent = -1
    alert_masked = False
    while True:
	get_status(stdscr)
	stdscr.addstr(0,20,'==== pALPIDEfs Watchdog ====')
	stdscr.addstr(13, 0, "Alert target: %s" % ",".join(ALERT_TARGET))
	stdscr.addstr(17, 0, "X: Mask alerts for %d seconds" % ALERT_REPEAT_TIME)
	stdscr.addstr(18, 0, "E: Unmask alerts")
	stdscr.addstr(19, 0, "CTRL-C: Exit")

	all_ok = True
        for pad in PADS:
            print_pad(stdscr,pad)
	    if (pad['bit'] == 0):
	      all_ok = False
	      
	stdscr.addstr(15, 0, " ")
	stdscr.clrtoeol()
	
	if (alert_sent > 0 and (ALERT_REPEAT_TIME - (time.time() - alert_sent) < ALERT_GRACE_TIME)):
	  not_ok_timestamp = -1
	  alert_sent = -1
	  alert_masked = False
	  
	if (all_ok):
	  not_ok_timestamp = -1
	  stdscr.addstr(15, 0, "All OK", curses.color_pair(2))
	else:
	  if alert_masked == False:
	    if (not_ok_timestamp < 0):
	      not_ok_timestamp = time.time()
	      
	    if alert_sent < 0:
	      time_to_alert = ALERT_GRACE_TIME - (time.time() - not_ok_timestamp)
	      stdscr.addstr(15, 0, "Sending alert message in %d seconds. Press X to prevent alert for %d seconds!" % (time_to_alert, ALERT_REPEAT_TIME), curses.color_pair(1))
	  
	      if time_to_alert < 0:
		send_alert()
		alert_sent = time.time()
	    else:
	      stdscr.addstr(15, 0, "Alert sent %d seconds ago" % (time.time() - alert_sent), curses.color_pair(1))
	      stdscr.clrtoeol()

	if alert_masked:
	  stdscr.addstr(15, 0, "Alerts masked for %d seconds. Press E to unmask." % (ALERT_REPEAT_TIME - (time.time() - alert_sent)), curses.color_pair(1))
	      
        print_alive(stdscr)
        stdscr.refresh()
        time.sleep(1)
        x=stdscr.getch()
        if x>0 and x<256:
            c=chr(x).upper()
            if c=='X':
	      alert_masked = True
	      alert_sent = time.time()
            if c=='E':
	      alert_masked = False
	      alert_sent = -1
	      not_ok_timestamp = -1


curses.wrapper(main)

