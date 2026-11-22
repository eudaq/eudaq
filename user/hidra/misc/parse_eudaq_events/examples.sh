#!/bin/bash
# Usage examples for the EUDAQ Event Parser
# Location: user/hidra/misc/parse_eudaq_events/

PARSER="./misc/parse_eudaq_events/parse_eudaq_events.py"
DATA_DIR="./run/out_data"

echo "=== EUDAQ Event Parser Examples ==="
echo ""
echo "Setup:"
echo "  cd user/hidra/"
echo "  chmod +x misc/parse_eudaq_events/parse_eudaq_events.py"
echo ""

echo "=== EXAMPLE 1: Quick file overview ==="
echo "  $PARSER $DATA_DIR/run024_260422162755.raw -s"
echo ""
echo "Output: File statistics without viewing individual events"
echo "Useful for: Quick checks of file contents, timing analysis"
echo ""

echo "=== EXAMPLE 2: View first event in batch mode ==="
echo "  $PARSER $DATA_DIR/run024_260422162755.raw -n 1"
echo ""
echo "Output: Full event display with header and hex dump"
echo "Useful for: Checking data structure, verifying file integrity"
echo ""

echo "=== EXAMPLE 3: View events with headers only ==="
echo "  $PARSER $DATA_DIR/run024_260422162755.raw -n 5 -H"
echo ""
echo "Output: Event headers without raw data blocks"
echo "Useful for: Fast browsing, analyzing event metadata"
echo ""

echo "=== EXAMPLE 4: View specific event range ==="
echo "  $PARSER $DATA_DIR/run024_260422162755.raw -e 100 -n 3"
echo ""
echo "Output: Events 100, 101, 102"
echo "Useful for: Examining specific events, investigating issues"
echo ""

echo "=== EXAMPLE 5: Interactive exploration ==="
echo "  $PARSER $DATA_DIR/run024_260422162755.raw"
echo ""
echo "Output: Interactive viewer - use SPACE for next, B for previous, G to go to event"
echo "Useful for: Detailed investigation, exploring the data"
echo ""

echo "=== EXAMPLE 6: Pipe to file ==="
echo "  $PARSER $DATA_DIR/run024_260422162755.raw -n 10 > events_dump.txt"
echo "  $PARSER $DATA_DIR/run024_260422162755.raw -s > file_info.txt"
echo ""
echo "Output: Saved to text files for sharing/archiving"
echo "Useful for: Reports, documentation, remote analysis"
echo ""

echo "=== EXAMPLE 7: Filter and analyze ==="
echo "  $PARSER $DATA_DIR/run024_260422162755.raw -n 1 | grep 'dataWords'"
echo "  $PARSER $DATA_DIR/run024_260422162755.raw -s | grep 'Duration'"
echo ""
echo "Output: Extract specific information from events"
echo "Useful for: Automated analysis, scripting"
echo ""

echo "=== EXAMPLE 8: Compare multiple runs ==="
echo "  echo 'Run 24:' && $PARSER $DATA_DIR/run024_260422162755.raw -s | grep 'Total Events'"
echo "  echo 'Run 25:' && $PARSER $DATA_DIR/run025_260422164432.raw -s | grep 'Total Events'"
echo ""
echo "Output: Side-by-side comparison of file statistics"
echo "Useful for: Run comparison, data quality checks"
echo ""

echo "=== FILE STATISTICS (Example) ==="
$PARSER $DATA_DIR/run024_260422162755.raw -s 2>&1 | head -25
echo ""

echo "=== FIRST EVENT (Example) ==="
$PARSER $DATA_DIR/run024_260422162755.raw -n 1 -H 2>&1 | head -30
echo ""

echo "=== HEX DUMP SAMPLE ==="
$PARSER $DATA_DIR/run024_260422162755.raw -n 1 2>&1 | grep -A 10 "BLOCK #0"
echo ""

echo "=== SCRIPT HELP ==="
$PARSER --help
