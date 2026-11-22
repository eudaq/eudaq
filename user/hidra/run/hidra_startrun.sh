#!/usr/bin/env sh

# Paths configuration
BINPATH=../../../bin
TMUX_SESSION="hidra_run_monitoring"
DASHBOARD_DIR="$EUDAQHIDRA/misc/dashboard/rc_mon"
BASE_CONFIG="./config/hidra_default.conf"
TEMP_CONFIG="./config/hidra_active.conf"

# Ensure the config directory exists
mkdir -p "$(dirname "$BASE_CONFIG")"

# Quick Sanity check: Ensure base config exists before doing anything
if [ ! -f "$BASE_CONFIG" ]; then
    echo "Error: Master template '$BASE_CONFIG' missing. Create it first."
    exit 1
fi

# ---------------------------------------------------------
# 1. State Persistence Layer: Determine Defaults
# ---------------------------------------------------------
if [ -f "$TEMP_CONFIG" ]; then
    SOURCE_FOR_DEFAULTS="$TEMP_CONFIG"
else
    SOURCE_FOR_DEFAULTS="$BASE_CONFIG"
fi

# Parse existing PEDESTAL_ONLY
LAST_PEDESTAL=$(sed -n '/\[Producer.QTPDProducer\]/,/\[/p' "$SOURCE_FOR_DEFAULTS" | grep "PEDESTAL_ONLY" | cut -d'=' -f2 | tr -d ' ')
[ -z "$LAST_PEDESTAL" ] && LAST_PEDESTAL="0"

# Parse existing MAX_EVENTS
LAST_MAX_EVENTS=$(sed -n '/\[DataCollector.HidraDataCollector\]/,/\[/p' "$SOURCE_FOR_DEFAULTS" | grep "MAX_EVENTS" | cut -d'=' -f2 | tr -d ' ')
[ -z "$LAST_MAX_EVENTS" ] && LAST_MAX_EVENTS="0"

# Parse existing EXPECTED_SOURCES to pre-fill the checkbox list
LAST_SOURCES=$(sed -n '/\[DataCollector.HidraDataCollector\]/,/\[/p' "$SOURCE_FOR_DEFAULTS" | grep "EXPECTED_SOURCES" | cut -d'=' -f2 | tr -d ' ')

# Set checkbox statuses based on what was parsed (YAD uses TRUE/FALSE)
QTPD_STATUS="FALSE"; FERS2_STATUS="FALSE"; TRACKER_STATUS="FALSE"; MAXICC_STATUS="FALSE"
if echo "$LAST_SOURCES" | grep -q "QTPDProducer";    then QTPD_STATUS="TRUE"; fi
if echo "$LAST_SOURCES" | grep -q "FERS2Producer";   then FERS2_STATUS="TRUE"; fi
if echo "$LAST_SOURCES" | grep -q "TrackerProducer"; then TRACKER_STATUS="TRUE"; fi
if echo "$LAST_SOURCES" | grep -q "MAXICCProducer";  then MAXICC_STATUS="TRUE"; fi

# Determine dropdown sorting order for YAD combo box
if [ "$LAST_PEDESTAL" = "1" ]; then PED_COMBO_ORDER="Yes!No"; else PED_COMBO_ORDER="No!Yes"; fi


# ---------------------------------------------------------
# 2. Input Handling (YAD GUI vs Skip Flag)
# ---------------------------------------------------------
if [ "$1" = "--skip" ]; then
    echo "Skipping interactive prompts. Loading defaults directly from $BASE_CONFIG..."
    
    PEDESTAL_ONLY=$(sed -n '/\[Producer.QTPDProducer\]/,/\[/p' "$BASE_CONFIG" | grep "PEDESTAL_ONLY" | cut -d'=' -f2 | tr -d ' ')
    [ -z "$PEDESTAL_ONLY" ] && PEDESTAL_ONLY="0"
    
    MAX_EVENTS=$(sed -n '/\[DataCollector.HidraDataCollector\]/,/\[/p' "$BASE_CONFIG" | grep "MAX_EVENTS" | cut -d'=' -f2 | tr -d ' ')
    [ -z "$MAX_EVENTS" ] && MAX_EVENTS="0"
    
    BASE_SOURCES=$(sed -n '/\[DataCollector.HidraDataCollector\]/,/\[/p' "$BASE_CONFIG" | grep "EXPECTED_SOURCES" | cut -d'=' -f2 | tr -d ' ')
    PRODUCERS_SELECTED=""
    if echo "$BASE_SOURCES" | grep -q "QTPDProducer";    then PRODUCERS_SELECTED="$PRODUCERS_SELECTED QTPDProducer"; fi
    if echo "$BASE_SOURCES" | grep -q "FERS2Producer";   then PRODUCERS_SELECTED="$PRODUCERS_SELECTED FERS2Producer"; fi
    if echo "$BASE_SOURCES" | grep -q "TrackerProducer"; then PRODUCERS_SELECTED="$PRODUCERS_SELECTED TrackerProducer"; fi
    if echo "$BASE_SOURCES" | grep -q "MAXICCProducer";  then PRODUCERS_SELECTED="$PRODUCERS_SELECTED MAXICCProducer"; fi

else
    if [ "$LAST_PEDESTAL" = "1" ]; then
        PREVIEW_IMAGE="/home/eudaq/Bob1.jpg"
    else
        PREVIEW_IMAGE="/home/eudaq/Bob1.jpg"
    fi
    
    # Native Wayland layout using an output-free Read Only field (:RO)
    GUI_OUTPUT=$(yad --form --title="EUDAQ Shift Run Control Setup" \
        --text="Review and tweak parameters for " \
        --image="$PREVIEW_IMAGE" --align=center \
        --column=2 \
        --field="Pedestal-only run?:CB" "$PED_COMBO_ORDER" \
        --field="Max Events (0 for unlimited)" "$LAST_MAX_EVENTS" \
        --field="Enable QTPDProducer:CHK" "$QTPD_STATUS" \
        --field="Enable FERS2Producer:CHK" "$FERS2_STATUS" \
        --field="Enable TrackerProducer:CHK" "$TRACKER_STATUS" \
        --field="Enable MAXICCProducer:CHK" "$MAXICC_STATUS" \
        --button="Cancel:1" --button="OK:0" \
        --width=450)
    
    if [ $? -ne 0 ] || [ -z "$GUI_OUTPUT" ]; then echo "Execution cancelled by user."; exit 1; fi

    # Parse combined output (YAD returns fields separated by '|')
    IS_PEDESTAL=$(echo "$GUI_OUTPUT" | cut -d'|' -f1)
    PEDESTAL_ONLY=$([ "$IS_PEDESTAL" = "Yes" ] && echo "1" || echo "0")
    
    # Strip any potential accidental spaces or non-digits from the string input
    MAX_EVENTS=$(echo "$GUI_OUTPUT" | cut -d'|' -f2 | tr -cd '0-9')
    [ -z "$MAX_EVENTS" ] && MAX_EVENTS="0"
    
    USE_QTPD=$(echo "$GUI_OUTPUT" | cut -d'|' -f3)
    USE_FERS2=$(echo "$GUI_OUTPUT" | cut -d'|' -f4)
    USE_TRACKER=$(echo "$GUI_OUTPUT" | cut -d'|' -f5)
    USE_MAXICC=$(echo "$GUI_OUTPUT" | cut -d'|' -f6)

    PRODUCERS_SELECTED=""
    [ "$USE_QTPD" = "TRUE" ] && PRODUCERS_SELECTED="$PRODUCERS_SELECTED QTPDProducer"
    [ "$USE_FERS2" = "TRUE" ] && PRODUCERS_SELECTED="$PRODUCERS_SELECTED FERS2Producer"
    [ "$USE_TRACKER" = "TRUE" ] && PRODUCERS_SELECTED="$PRODUCERS_SELECTED TrackerProducer"
    [ "$USE_MAXICC" = "TRUE" ] && PRODUCERS_SELECTED="$PRODUCERS_SELECTED MAXICCProducer"
fi


# ---------------------------------------------------------
# 3. Format Expected Sources Configuration Value
# ---------------------------------------------------------
EXPECTED_SOURCES=""
if echo "$PRODUCERS_SELECTED" | grep -q "QTPDProducer"; then
    EXPECTED_SOURCES="1:QTPDProducer"
fi
if echo "$PRODUCERS_SELECTED" | grep -q "FERS2Producer"; then
    [ ! -z "$EXPECTED_SOURCES" ] && EXPECTED_SOURCES="$EXPECTED_SOURCES,"
    EXPECTED_SOURCES="${EXPECTED_SOURCES}2:FERS2Producer"
fi
if echo "$PRODUCERS_SELECTED" | grep -q "TrackerProducer"; then
    [ ! -z "$EXPECTED_SOURCES" ] && EXPECTED_SOURCES="$EXPECTED_SOURCES,"
    EXPECTED_SOURCES="${EXPECTED_SOURCES}3:TrackerProducer"
fi
if echo "$PRODUCERS_SELECTED" | grep -q "MAXICCProducer"; then
    [ ! -z "$EXPECTED_SOURCES" ] && EXPECTED_SOURCES="$EXPECTED_SOURCES,"
    EXPECTED_SOURCES="${EXPECTED_SOURCES}4:MAXICCProducer"
fi

echo "=========================================="
echo "  GENERATING COMPONENT CONFIGURATION:     "
echo "  Pedestal-Only Mode : $PEDESTAL_ONLY"
echo "  Maximum Run Events : $MAX_EVENTS"
echo "  Expected Sources   : $EXPECTED_SOURCES"
echo "=========================================="


# ---------------------------------------------------------
# 4. Configuration Template Injection
# ---------------------------------------------------------
awk -v ped="$PEDESTAL_ONLY" -v max="$MAX_EVENTS" -v src="$EXPECTED_SOURCES" '
    /^[ \t]*PEDESTAL_ONLY[ \t]*=/  { next }
    /^[ \t]*MAX_EVENTS[ \t]*=/     { next }
    /^[ \t]*EXPECTED_SOURCES[ \t]*=/ { next }

    /\[Producer.QTPDProducer\]/ { 
        print; print "PEDESTAL_ONLY = " ped; next 
    }
    /\[DataCollector.HidraDataCollector\]/ { 
        print; print "MAX_EVENTS = " max; print "EXPECTED_SOURCES = " src; next 
    }
    { print }
' "$BASE_CONFIG" > "$TEMP_CONFIG"

mkdir -p /home/eudaq/daq/TB2026_HidraData/HidraData /home/eudaq/daq/TB2026_HidraData/Logs

if [ -z "$EUDAQHIDRA" ]; then
    echo "Error: \$EUDAQHIDRA variable environment string is not populated."
    exit 1
fi

if [ ! -d "$DASHBOARD_DIR" ]; then
    echo "Error: Missing operational dashboard target: $DASHBOARD_DIR"
    exit 1
fi

if tmux has-session -t "$TMUX_SESSION" 2>/dev/null; then
    tmux kill-session -t "$TMUX_SESSION"
fi
tmux new-session -d -s "$TMUX_SESSION" \
    "cd \"$DASHBOARD_DIR\" && php -S 0.0.0.0:8080"

echo "Launching main EUDAQ GUI Runtime System..."
$BINPATH/euRun -n HidraRunControl &

sleep 1
echo "Launching main EUDAQ LOG System..."
$BINPATH/hidraLog &
sleep 1
echo "Launching main EUDAQ Monitoring System..."
$BINPATH/euCliMonitor  -n HidraHttpMonitor -t HidraHttpMonitor &
sleep 1
echo "Launching main EUDAQ DataCollector..."
$BINPATH/euCliCollector -n HidraDataCollector -t HidraDataCollector &
sleep 1

if echo "$PRODUCERS_SELECTED" | grep -q "QTPDProducer"; then
    echo "--> Spawning hardware link process: QTPDProducer"
    $BINPATH/euCliProducer -n HidraQTPDProducer -t QTPDProducer &
fi

if echo "$PRODUCERS_SELECTED" | grep -q "FERS2Producer"; then
    echo "--> Spawning hardware link process: FERS2Producer"
    $BINPATH/euCliProducer -n HidraFERS2Producer -t FERS2Producer &
fi

if echo "$PRODUCERS_SELECTED" | grep -q "TrackerProducer"; then
    echo "--> Spawning hardware link process: TrackerProducer"
    $BINPATH/euCliProducer -n HidraTrackerProducer -t TrackerProducer &
fi

if echo "$PRODUCERS_SELECTED" | grep -q "MAXICCProducer"; then
    echo "--> Spawning hardware link process: MAXICCProducer (Instance of FERS2Producer)"
    $BINPATH/euCliProducer -n HidraMAXICCProducer -t MAXICCProducer &
fi


while pgrep -f "euRun -n HidraRunControl" > /dev/null; do
    sleep 1
done

echo "Run Control GUI window tracking closed by shifter."
sleep 2
echo "Halting background link processes and core system layers..."
sleep 1

killall hidraLog euCliMonitor euCliCollector euCliProducer 2>/dev/null
sleep 1

echo "System cleanup sequence complete."