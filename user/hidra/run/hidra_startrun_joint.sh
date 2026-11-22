#!/usr/bin/env sh

BINPATH=../../../bin
TMUX_SESSION="hidra_run_monitoring"
DASHBOARD_DIR="$EUDAQHIDRA/misc/dashboard/rc_mon"

mkdir -p /home/eudaq/daq/TB2026_HidraData/HidraData /home/eudaq/daq/TB2026_HidraData/Logs

if [ -z "$EUDAQHIDRA" ]; then
    echo "Error: EUDAQHIDRA is not set."
    exit 1
fi

if [ ! -d "$DASHBOARD_DIR" ]; then
    echo "Error: dashboard directory does not exist: $DASHBOARD_DIR"
    exit 1
fi

# Close existing tmux session if it exists
if tmux has-session -t "$TMUX_SESSION" 2>/dev/null; then
    tmux kill-session -t "$TMUX_SESSION"
fi

# Start monitoring dashboard in a new tmux session
tmux new-session -d -s "$TMUX_SESSION" \
    "cd \"$DASHBOARD_DIR\" && php -S 0.0.0.0:8080"

$BINPATH/euRun -n HidraRunControl &
sleep 1
$BINPATH/hidraLog &
sleep 1
$BINPATH/euCliMonitor  -n HidraHttpMonitor -t HidraHttpMonitor &
sleep 1
$BINPATH/euCliCollector -n HidraDataCollector -t HidraDataCollector &
sleep 1

$BINPATH/euCliProducer -n HidraQTPDProducer -t QTPDProducer &

#$BINPATH/euCliProducer -n HidraFERS2Producer -t FERS2Producer 

# $BINPATH/euCliProducer -n HidraTrackerProducer -t TrackerProducer 
