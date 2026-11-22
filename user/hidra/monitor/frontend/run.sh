#!/usr/bin/env bash
# Single-command launcher for the HiDRA monitor frontend.
#
# Creates the virtualenv on first run, syncs requirements, then serves the
# Dash app with gunicorn (production WSGI server — no "development server"
# warning).
#
# Usage:
#   ./run.sh                       # bind 0.0.0.0:8050
#   ./run.sh --port 8060           # override port
#   ./run.sh --host 127.0.0.1      # override bind host
#   ./run.sh --config other.yaml   # override config file
#   ./run.sh --workers 2           # gunicorn workers (default 1; see note below)
#   ./run.sh --reinstall           # force reinstall of requirements
#
# Note: Dash keeps per-process state (BackendClient, OverlayStore, perf
# counters) in memory. Multiple workers each hold their own copy — fine
# for read-only dashboards, but expect log duplication and independent
# overlay caches. Default is 1 worker.

set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)
cd "$SCRIPT_DIR"

VENV_DIR=".venv"
REQS_FILE="requirements.txt"
STAMP_FILE="$VENV_DIR/.requirements.stamp"

HOST="0.0.0.0"
PORT="8050"
WORKERS="1"
CONFIG=""
REINSTALL=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --host)        HOST="$2"; shift 2 ;;
        --port)        PORT="$2"; shift 2 ;;
        --workers)     WORKERS="$2"; shift 2 ;;
        --config)      CONFIG="$2"; shift 2 ;;
        --reinstall)   REINSTALL=1; shift ;;
        -h|--help)
            sed -n '2,20p' "$0"
            exit 0
            ;;
        *)
            echo "unknown option: $1" >&2
            exit 2
            ;;
    esac
done

if [[ ! -d "$VENV_DIR" ]]; then
    echo "[run.sh] creating virtualenv in $VENV_DIR (with --system-site-packages for PyROOT)"
    python3 -m venv --system-site-packages "$VENV_DIR"
fi

# shellcheck disable=SC1091
source "$VENV_DIR/bin/activate"

# Sync deps when the venv is fresh, requirements.txt changed, or --reinstall.
needs_install=0
if [[ "$REINSTALL" -eq 1 ]]; then
    needs_install=1
elif [[ ! -f "$STAMP_FILE" ]]; then
    needs_install=1
elif [[ "$REQS_FILE" -nt "$STAMP_FILE" ]]; then
    needs_install=1
fi

if [[ "$needs_install" -eq 1 ]]; then
    echo "[run.sh] installing/updating dependencies from $REQS_FILE"
    python -m pip install --upgrade pip
    python -m pip install -r "$REQS_FILE"
    touch "$STAMP_FILE"
fi

if [[ -n "$CONFIG" ]]; then
    export HIDRA_FRONTEND_CONFIG="$CONFIG"
fi

echo "[run.sh] starting gunicorn on http://$HOST:$PORT (workers=$WORKERS)"
exec gunicorn \
    --workers "$WORKERS" \
    --bind "$HOST:$PORT" \
    --access-logfile - \
    --error-logfile - \
    app:server
