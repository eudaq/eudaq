#!/usr/bin/env python3
import os
import time
import sys
import socket
from datetime import datetime
import psutil

# ANSI color and symbol configurations
GREEN_CHECK = "\033[92m✔\033[0m"
RED_CROSS = "\033[91m✘\033[0m"
YELLOW_WARN = "\033[93m⚠\033[0m"
RESET = "\033[0m"
BOLD = "\033[1m"
DIM = "\033[2m"

# --- CONFIGURATION ---
REFRESH_INTERVAL = 3  # seconds
PROCESS_TO_CHECK = "cernbox"
DIRECTORIES_TO_CHECK = [
    os.path.expanduser("~/cernbox/TB2026_H8"),
    os.path.expanduser("~/TB2026_TrackerData")
]
DISK_THRESHOLD = 90.0  # % percentage threshold to trigger warning
RAM_THRESHOLD = 80.0   # % percentage threshold to trigger warning


def check_process(process_name):
    """
    Robustly checks if a specific process is running.
    Isolates the actual executable name to prevent false positives from scripts/IDEs.
    """
    proc_name_lower = process_name.lower()
    for proc in psutil.process_iter(['cmdline', 'name']):
        try:
            cmdline = proc.info['cmdline']
            if not cmdline:
                if proc_name_lower in proc.info['name'].lower():
                    return GREEN_CHECK
                continue
            
            executable_name = os.path.basename(cmdline[0]).lower()
            if proc_name_lower in executable_name:
                if "binfmt" in executable_name:
                    continue
                return GREEN_CHECK
        except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
            continue
    return RED_CROSS


def check_directory(path):
    """Checks if a directory exists and is not empty."""
    try:
        if os.path.isdir(path):
            with os.scandir(path) as it:
                if any(it):
                    return GREEN_CHECK
    except PermissionError:
        return RED_CROSS
    return RED_CROSS


def check_network(host="1.1.1.1", port=53, timeout=1):
    """Checks network connectivity using a quick TCP socket connection."""
    try:
        socket.setdefaulttimeout(timeout)
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((host, port))
            return GREEN_CHECK
    except OSError:
        return RED_CROSS


def check_disk_space(path="/"):
    """Monitors disk usage percentage and flags warnings if critical."""
    try:
        usage = psutil.disk_usage(path)
        percent = usage.percent
        if percent >= DISK_THRESHOLD:
            return f"{YELLOW_WARN} {percent}% (Near Capacity)"
        return f"{GREEN_CHECK} {percent}%"
    except Exception:
        return RED_CROSS

def check_ram_usage():
    """Monitors RAM usage percentage and flags warnings if >= RAM_THRESHOLD."""
    try:
        memory = psutil.virtual_memory()
        percent = memory.percent
        if percent >= RAM_THRESHOLD:
            return f"{YELLOW_WARN} {percent}% (High Usage)"
        return f"{GREEN_CHECK} {percent}%"
    except Exception:
        return RED_CROSS

def get_ssh_connections():
    """Counts active inbound SSH sessions connected to this machine."""
    count = 0
    try:
        for conn in psutil.net_connections(kind='tcp'):
            if conn.status == 'ESTABLISHED' and conn.laddr.port == 22:
                count += 1
        if count > 0:
            return f"{GREEN_CHECK} {count} active"
        return f"{RESET}0 active"
    except (psutil.AccessDenied, AttributeError):
        return f"{YELLOW_WARN} N/A (Requires sudo?)"


def get_load_average():
    """Returns the system load average for the last 1, 5, and 15 minutes."""
    try:
        load1, load5, load15 = os.getloadavg()
        return f"{RESET}{load1:.2f}, {load5:.2f}, {load15:.2f}"
    except Exception:
        return RED_CROSS


def display_health():
    """Gathers all checks and prints a formatted dashboard to the terminal."""
    os.system('clear')
    
    # Generate current timestamp
    current_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    
    # Header with Last Update timestamp
    print(f"{BOLD}SYSTEM HEALTH MONITOR{RESET}  {DIM}(Last Update: {current_time}){RESET}\n")
    print(f"{BOLD}{'METRIC':<45}{'STATUS':<15}{RESET}")
    print("-" * 65)
    
    # Processes & Directories
    print(f"{'Process running: ' + PROCESS_TO_CHECK:<45}{check_process(PROCESS_TO_CHECK)}")
    for path in DIRECTORIES_TO_CHECK:
        print(f"{'Dir: ' + path:<45}{check_directory(path)}")
            
    # Network & Core System Metrics
#    print(f"{'Network (Internet)':<25}{check_network()}")
#    print(f"{'Disk Space (/)':<25}{check_disk_space('/')}")
#    print(f"{'Inbound SSH Sessions':<25}{get_ssh_connections()}")
    print(f"{'RAM Usage':<45}{check_ram_usage()}")
    print(f"{'Load Average (1,5,15m)':<45}{get_load_average()}")



def main():
    try:
        while True:
            display_health()
            time.sleep(REFRESH_INTERVAL)
    except KeyboardInterrupt:
        print("\nMonitoring stopped.")
        sys.exit(0)

if __name__ == "__main__":
    main()