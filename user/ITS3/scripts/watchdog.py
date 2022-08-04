#!/usr/bin/env python3

import os
import json
import time
import requests
import argparse
from rich import print
from rich.rule import Rule


def send_to_mm(channel, msg, hook_url):
    print(msg)
    r = requests.post(
        hook_url,
        headers={"Content-Type": "application/json"},
        data=json.dumps({"text":msg,"channel":channel})
    )
    if r.text == "ok":
        return True
    print(r.json())
    return False

def ls(dir,recursive=False):
    ret = []
    for d,_,files in os.walk(dir):
        for f in files:
            ret.append(os.path.join(d,f))
        if not recursive: break
    return ret

def get_size(files):
    return sum(os.path.getsize(f) for f in files)

if __name__=="__main__":
    parser = argparse.ArgumentParser("Watch if directory contents chage and inform on Mattermost.")
    parser.add_argument("directory", help="Directory to monitor for changes.")
    parser.add_argument("channel", help="Mattermost channel to write to.")
    parser.add_argument("--interval", "-i", default=5, type=float, help="Monitoring interval in minutes (default 5).")
    parser.add_argument("--recursive", "-r", action="store_true", help="Monitor also sub directories.")
    parser.add_argument("--only-critical", "-c", action="store_true", help="Send only critical messages.")
    parser.add_argument("--test", action="store_true", help="Test connection to Mattermost and exit.")
    parser.add_argument("--hook", default="https://mattermost.web.cern.ch/hooks/xqc5mcwe9pnoum5aoo9j148xde", help="Mattermost webhook URL")
    args = parser.parse_args()

    if args.test:
        if send_to_mm(args.channel, "Testing watchdog", hook_url=args.hook):
            print("[green]Test successful, check MM channel![/green]")
        else:
            print("[bold red]Test failed![/bold red]")
        exit()

    dir = os.path.abspath(args.directory)

    flist = ls(dir,True)
    fsize = get_size(flist)

    print(Rule("Watchdog operational! Bark bark!",characters="="))
    if not send_to_mm(args.channel, "Watching: " + dir, args.hook):
        print("[bold red]Problem with sending message to Mattermost![/bold red]")
        exit()

    try:
        while True:
            time.sleep(args.interval*60)
            flist_new = ls(dir,True)
            new_files = [f for f in flist_new if f not in flist]
            flist = flist_new
            if new_files and not args.only_critical:
                msg = "New file(s) found: "+", ".join(f for f in new_files)
                send_to_mm(args.channel, msg, args.hook)
            fsize_new = get_size(flist)
            if fsize_new == fsize:
                msg = f"@all `{dir}` size unchanged in the past {args.interval} minutes!"
                send_to_mm(args.channel, msg, args.hook)
            fsize=fsize_new
    except KeyboardInterrupt:
        send_to_mm(args.channel, "Watchdog stopped.", args.hook)
    except Exception as e:
        send_to_mm(args.channel, f"Exception: {e}", args.hook)
        raise e
