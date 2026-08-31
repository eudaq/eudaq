#!/bin/bash

# This script relies on bash features (arrays, [[ ]], $'...', BASH_SOURCE), so
# it cannot run under a POSIX sh/dash (or fish). Fail early with a clear message
# instead of a confusing parse/runtime error. The test itself is POSIX-safe.
if [ -z "${BASH_VERSION:-}" ]; then
    echo "setup.sh: please source this with bash (your current shell is not bash)." >&2
    return 1 2>/dev/null || exit 1
fi

# Resolve the script directory and repository root dynamically.
SCRIPT_DIR=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
# Export REPO_ROOT so it can be used by other scripts that source this one, e.g. the run scripts.
export REPO_ROOT=$(realpath "$SCRIPT_DIR/../../../")
REPO_RUN=$(realpath "$SCRIPT_DIR/../run/")

HIDRA_REQUIRED_CMAKE_VERSION="3.25.1"

# Compare two semantic versions: return 0 if $1 >= $2, otherwise 1.
version_ge() {
    local current="$1"
    local required="$2"
    local i
    local current_parts required_parts

    IFS='.' read -r -a current_parts <<< "$current"
    IFS='.' read -r -a required_parts <<< "$required"

    for ((i=0; i<3; i++)); do
        local c="${current_parts[i]:-0}"
        local r="${required_parts[i]:-0}"

        if ((10#$c > 10#$r)); then
            return 0
        fi
        if ((10#$c < 10#$r)); then
            return 1
        fi
    done

    return 0
}

get_cmake_version() {
    if ! command -v cmake >/dev/null 2>&1; then
        echo ""
        return 1
    fi

    cmake --version 2>/dev/null | head -n1 | awk '{print $3}'
}

supports_hidra_presets() {
    local cmake_version
    cmake_version=$(get_cmake_version)

    if [ -z "$cmake_version" ]; then
        return 1
    fi

    version_ge "$cmake_version" "$HIDRA_REQUIRED_CMAKE_VERSION"
}

# Helper to create the build directory if needed, run CMake with the expected
# options, and return to the caller's original directory.
hidra_cmake() {
    local original_dir=$(pwd)
    cd "$REPO_ROOT"

    if supports_hidra_presets; then
        if [ ! -f "$REPO_ROOT/CMakeUserPresets.json" ]; then
            setup_vscode_hidra
        fi

        cmake --preset hidra-configure --fresh
    else
        local cmake_version
        cmake_version=$(get_cmake_version)
        echo "CMake $cmake_version does not support HiDRA presets (required >= $HIDRA_REQUIRED_CMAKE_VERSION)."
        echo "Using the classic CMake configuration fallback."

        cmake --fresh -S "$REPO_ROOT" -B "$REPO_ROOT/build" -G "Unix Makefiles" \
            -DEUDAQ_BUILD_ONLINE_ROOT_MONITOR=OFF \
            -DEUDAQ_LIBRARY_BUILD_TTREE=OFF \
            -DUSER_HIDRA_BUILD=ON \
            -DUSER_HIDRA_DC_ROOT_OUTPUT=ON \
            -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    fi

    cd "$original_dir"
}

# Back-compatibility alias for the old name.
cmake_config() {
    hidra_cmake "$@"
}

# Helper to build the code without manually jumping into the build directory.
# It runs the HiDRA workflow when supported, otherwise it falls back to a
# regular build and install sequence.

hidra_build() {
    local original_dir=$(pwd)
    cd "$REPO_ROOT"

    if supports_hidra_presets; then
        if [ ! -f "$REPO_ROOT/CMakeUserPresets.json" ]; then
            setup_vscode_hidra
        fi

        cmake --workflow --preset hidra-full
    else
        local cmake_version
        cmake_version=$(get_cmake_version)
        echo "CMake $cmake_version does not support HiDRA presets/workflow (required >= $HIDRA_REQUIRED_CMAKE_VERSION)."
        echo "Using the classic build/install fallback."

        hidra_cmake
        cmake --build "$REPO_ROOT/build" -j 10
        cmake --build "$REPO_ROOT/build" --target install -j 10
    fi

    cd "$original_dir"
}

# Back-compatibility alias for the old name.
build_hidra() {
    hidra_build "$@"
}

runhidra(){
    cd $REPO_RUN

    if [[ "$1" == "dry" ]]; then
    ./hidra_startrun_dry.sh
    else
	./hidra_startrun.sh
	fi
}

# Launch the HiDRA monitor frontend (Dash app via gunicorn). Forwards any
# extra arguments to run.sh, so `hidra_frontend --port 8060` works.
hidra_frontend() {
    local frontend_dir="$SCRIPT_DIR/../monitor/frontend"
    if [ ! -x "$frontend_dir/run.sh" ]; then
        echo "hidra_frontend: $frontend_dir/run.sh not found or not executable" >&2
        return 1
    fi
    ( cd "$frontend_dir" && ./run.sh "$@" )
}

# Launch the frontend with the Flask dev server and hot-reload
# (`app.py --debug`), for development. Needs the venv that `hidra_frontend`
# (run.sh) creates on first launch; extra arguments are forwarded to app.py
# (e.g. `hidra_frontend_debug --port 8060`).
hidra_frontend_debug() {
    local frontend_dir="$SCRIPT_DIR/../monitor/frontend"
    local py="$frontend_dir/.venv/bin/python"
    if [ ! -x "$py" ]; then
        echo "hidra_frontend_debug: $py not found." >&2
        echo "Run 'hidra_frontend' once first to create the venv and install dependencies." >&2
        return 1
    fi
    ( cd "$frontend_dir" && "$py" app.py --debug "$@" )
}

# Stop a running HiDRA DAQ chain: terminate the EUDAQ processes started by the
# run scripts (RunControl, collector, monitor, producers) and close the tmux
# session hosting the PHP dashboard. Sends SIGTERM so EUDAQ can shut down
# cleanly. Safe to run when nothing is up. Does NOT touch the Dash frontend
# (stop that with Ctrl-C in its own shell).
hidra_stop() {
    local tmux_session="hidra_run_monitoring"
    local patterns=(
        "euRun -n HidraRunControl"
        "euCliCollector -n HidraDataCollector"
        "euCliMonitor -n HidraHttpMonitor"
        "euCliProducer -n Hidra"
    )
    # Limit kills to the current user's processes (and match the full,
    # fairly specific command lines) to avoid touching unrelated processes
    # or other users' jobs on a shared machine.
    local pat killed_any=0 uid
    uid=$(id -u)
    for pat in "${patterns[@]}"; do
        if pkill -u "$uid" -f "$pat" 2>/dev/null; then
            echo "  stopped: $pat"
            killed_any=1
        fi
    done
    if command -v tmux >/dev/null 2>&1 && tmux has-session -t "$tmux_session" 2>/dev/null; then
        tmux kill-session -t "$tmux_session" && echo "  stopped tmux session: $tmux_session"
        killed_any=1
    fi
    if [ "$killed_any" -eq 0 ]; then
        echo "hidra_stop: no running HiDRA DAQ processes found."
    fi
}

# Show which processes are listening on the HiDRA-related TCP ports. Without
# root the process name may be hidden (shown as '?') for ports owned by
# another user, but the LISTEN state is still reported.
hidra_ports() {
    local tool=""
    if command -v ss >/dev/null 2>&1; then
        tool=ss
    elif command -v lsof >/dev/null 2>&1; then
        tool=lsof
    else
        echo "hidra_ports: neither 'ss' nor 'lsof' is available" >&2
        return 1
    fi

    local ports=("44000:RunControl" "9090:monitor HTTP" "8050:frontend" "8080:dashboard")
    local entry port label proc line
    printf '%-7s %-13s %s\n' "PORT" "SERVICE" "STATUS"
    for entry in "${ports[@]}"; do
        port="${entry%%:*}"
        label="${entry#*:}"
        proc=""
        if [ "$tool" = ss ]; then
            line=$(ss -ltnHp "( sport = :$port )" 2>/dev/null)
            if [ -n "$line" ]; then
                proc=$(printf '%s' "$line" | grep -oE '"[^"]+"' | head -1 | tr -d '"')
                [ -z "$proc" ] && proc="?"
            fi
        else
            proc=$(lsof -nP -iTCP:"$port" -sTCP:LISTEN 2>/dev/null | awk 'NR>1{print $1; exit}')
        fi
        if [ -n "$proc" ]; then
            printf '%-7s %-13s LISTEN (%s)\n' "$port" "$label" "$proc"
        else
            printf '%-7s %-13s free\n' "$port" "$label"
        fi
    done
}

# Follow (tail -f) the most recent .log file in run/logs/.
hidra_logs() {
    local logdir="$REPO_RUN/logs"
    if [ ! -d "$logdir" ]; then
        echo "hidra_logs: $logdir not found" >&2
        return 1
    fi
    local latest
    latest=$(ls -t "$logdir"/*.log 2>/dev/null | head -1)
    if [ -z "$latest" ]; then
        echo "hidra_logs: no .log files in $logdir" >&2
        return 1
    fi
    echo "Following $latest  (Ctrl-C to stop)"
    tail -f "$latest"
}


# Create a local CMakeUserPresets.json file in the repository root so VSCode/CMake Tools
# can use the HiDRA presets. This file is meant for local use and should not be committed.
setup_vscode_hidra() {
    local settings_path="$REPO_ROOT/.vscode/settings.json"

    mkdir -p "$REPO_ROOT/.vscode"

    cat > "$REPO_ROOT/CMakeUserPresets.json" << 'EOF'
{
        "version": 6,
    "include": [
        "user/hidra/misc/CMakePresets.hidra.json"
    ]
}
EOF

    if [ -f "$settings_path" ]; then
        if command -v jq >/dev/null 2>&1; then
            local tmp_settings
            tmp_settings=$(mktemp)
            if jq '. + {
                "cmake.useCMakePresets": "always",
                "cmake.defaultConfigurePreset": "hidra-configure",
                "cmake.defaultBuildPreset": "hidra-build",
                "cmake.configureOnOpen": false
            }' "$settings_path" > "$tmp_settings"; then
                mv "$tmp_settings" "$settings_path"
                echo "Merged CMake keys into $settings_path"
            else
                rm -f "$tmp_settings"
                echo "Warning: $settings_path is not valid JSON, skipping merge"
                echo "Please fix JSON or remove file and run setup_vscode_hidra again"
            fi
        else
            echo "Warning: jq not found, cannot merge into existing $settings_path"
            echo "Install jq or update VSCode CMake settings manually"
        fi
    else
        cat > "$settings_path" << 'EOF'
{
    "cmake.useCMakePresets": "always",
    "cmake.defaultConfigurePreset": "hidra-configure",
    "cmake.defaultBuildPreset": "hidra-build",
    "cmake.configureOnOpen": false
}
EOF
        echo "Created $settings_path"
    fi

    echo "Created $REPO_ROOT/CMakeUserPresets.json"
    echo "Inside VSCode you can now select the presets: hidra-configure / hidra-build / hidra-install / hidra-full"
}

# Remove the local user presets file and restore the initial state.
clean_vscode_hidra() {
    local settings_path="$REPO_ROOT/.vscode/settings.json"

    rm -f "$REPO_ROOT/CMakeUserPresets.json"

    if [ -f "$settings_path" ]; then
        if command -v jq >/dev/null 2>&1; then
            local tmp_settings
            tmp_settings=$(mktemp)
            if jq 'del(
                ."cmake.useCMakePresets",
                ."cmake.defaultConfigurePreset",
                ."cmake.defaultBuildPreset",
                ."cmake.configureOnOpen"
            )' "$settings_path" > "$tmp_settings"; then
                mv "$tmp_settings" "$settings_path"
                echo "Removed HiDRA CMake keys from $settings_path"
            else
                rm -f "$tmp_settings"
                echo "Warning: $settings_path is not valid JSON, skipping cleanup of CMake keys"
            fi
        else
            echo "Warning: jq not found, cannot clean CMake keys from $settings_path"
        fi
    fi

    echo "Removed $REPO_ROOT/CMakeUserPresets.json"
}

# Check the state of the local VSCode/CMake configuration for HiDRA.
check_vscode_hidra() {
    local user_presets_path="$REPO_ROOT/CMakeUserPresets.json"
    local settings_path="$REPO_ROOT/.vscode/settings.json"
    local status=0

    echo "[HiDRA VSCode setup check]"

    if command -v jq >/dev/null 2>&1; then
        echo "- jq: OK"
    else
        echo "- jq: MISSING (non-destructive merge/cleanup disabled)"
        status=2
    fi

    if [ -f "$user_presets_path" ]; then
        if grep -q '"user/hidra/misc/CMakePresets.hidra.json"' "$user_presets_path"; then
            echo "- CMakeUserPresets.json: OK"
        else
            echo "- CMakeUserPresets.json: PRESENT but missing HiDRA include"
            status=1
        fi
    else
        echo "- CMakeUserPresets.json: MISSING"
        status=1
    fi

    if [ -f "$settings_path" ]; then
        if command -v jq >/dev/null 2>&1; then
            local use_presets
            local configure_preset
            local build_preset
            local configure_on_open

            use_presets=$(jq -r 'if has("cmake.useCMakePresets") then ."cmake.useCMakePresets" else "__missing__" end' "$settings_path" 2>/dev/null)
            configure_preset=$(jq -r 'if has("cmake.defaultConfigurePreset") then ."cmake.defaultConfigurePreset" else "__missing__" end' "$settings_path" 2>/dev/null)
            build_preset=$(jq -r 'if has("cmake.defaultBuildPreset") then ."cmake.defaultBuildPreset" else "__missing__" end' "$settings_path" 2>/dev/null)
            configure_on_open=$(jq -r 'if has("cmake.configureOnOpen") then (."cmake.configureOnOpen"|tostring) else "__missing__" end' "$settings_path" 2>/dev/null)

            if [[ "$use_presets" == "always" && "$configure_preset" == "hidra-configure" && "$build_preset" == "hidra-build" && "$configure_on_open" == "false" ]]; then
                echo "- .vscode/settings.json CMake keys: OK"
            else
                echo "- .vscode/settings.json CMake keys: NOT matching HiDRA defaults"
                echo "  current: usePresets=$use_presets, configure=$configure_preset, build=$build_preset, configureOnOpen=$configure_on_open"
                status=1
            fi
        else
            echo "- .vscode/settings.json: PRESENT (cannot validate CMake keys without jq)"
        fi
    else
        echo "- .vscode/settings.json: MISSING"
        status=1
    fi

    if [ $status -eq 0 ]; then
        echo "Result: OK"
    else 
        if [ $status -eq 1 ]; then
            echo "Result: NOT READY (run setup_vscode_hidra)"
        else
            echo "Result: MISSING DEPENDENCIES (run sudo apt install jq)"
        fi
    fi

    return $status
}

# Helper to clean the build directory and related output folders (lib, etc.).
# It runs make_clean.sh from the repository root and returns to the caller's directory.
hidra_cmake_clean() {
    local original_dir=$(pwd)
    cd "$REPO_ROOT"
    sh "$REPO_ROOT/make_clean.sh"
    cd "$original_dir"
}

# Back-compatibility alias for the old name.
cmake_clean() {
    hidra_cmake_clean "$@"
}

# Convenience aliases for quickly jumping to common directories.
alias build_dir='cd "$REPO_ROOT/build"'
alias hidra_run='cd "$REPO_ROOT/user/hidra/run"'
alias hidra_dir='cd "$REPO_ROOT/user/hidra"'
if [ -z "$EUDAQHIDRA" ]; then
    export EUDAQHIDRA="$REPO_ROOT/user/hidra"
fi
alias hidra_backup_data='$REPO_ROOT/user/hidra/misc/hidra_backup_data.sh'
alias hidra_health='$REPO_ROOT/user/hidra/misc/hidra_health.py'

# Print a short, user-friendly summary of the commands this script provides.
# Shown automatically when the script is sourced; run `hidra_help` to see it
# again at any time.
hidra_help() {
    local B='' C='' D='' R=''
    if [ -t 1 ]; then
        B=$'\033[1m'; C=$'\033[36m'; D=$'\033[2m'; R=$'\033[0m'
    fi

    printf '%sHiDRA helpers loaded%s  (repo: %s)\n' "$B" "$R" "$REPO_ROOT"
    printf 'Available commands:\n\n'

    printf '  %sBuild & install%s\n' "$D" "$R"
    printf '    %s%-20s%s configure the build (CMake presets, or classic fallback) %s(alias: cmake_config)%s\n' "$C" "hidra_cmake" "$R" "$D" "$R"
    printf '    %s%-20s%s configure + build + install everything %s(alias: build_hidra)%s\n' "$C" "hidra_build" "$R" "$D" "$R"
    printf '    %s%-20s%s remove build/ and installed outputs %s(alias: cmake_clean)%s\n' "$C" "hidra_cmake_clean" "$R" "$D" "$R"
    printf '\n'

    printf '  %sRun & monitor%s\n' "$D" "$R"
    printf '    %s%-20s%s start a run: joint XDC+FERS2, or "dry" (no hardware)\n' "$C" "runhidra [dry]" "$R"
    printf '    %s%-20s%s launch the Dash web monitor (args forwarded to run.sh)\n' "$C" "hidra_frontend" "$R"
    printf '    %s%-20s%s launch the web monitor with Flask dev server + hot-reload\n' "$C" "hidra_frontend_debug" "$R"
    printf '    %s%-20s%s stop the running DAQ chain (EUDAQ processes + tmux dashboard)\n' "$C" "hidra_stop" "$R"
    printf '    %s%-20s%s show listeners on the HiDRA ports (44000/9090/8050/8080)\n' "$C" "hidra_ports" "$R"
    printf '    %s%-20s%s tail -f the most recent run log (run/logs/)\n' "$C" "hidra_logs" "$R"
    printf '\n'

    printf '  %sVSCode / CMake presets%s\n' "$D" "$R"
    printf '    %s%-20s%s create local CMakeUserPresets.json + .vscode settings\n' "$C" "setup_vscode_hidra" "$R"
    printf '    %s%-20s%s check the local VSCode/CMake preset setup\n' "$C" "check_vscode_hidra" "$R"
    printf '    %s%-20s%s remove the local presets/settings\n' "$C" "clean_vscode_hidra" "$R"
    printf '\n'

    printf '  %sShortcuts%s\n' "$D" "$R"
    printf '    %s%-20s%s cd to user/hidra\n' "$C" "hidra_dir" "$R"
    printf '    %s%-20s%s cd to the run/ directory\n' "$C" "hidra_run" "$R"
    printf '    %s%-20s%s cd to the build/ directory\n' "$C" "build_dir" "$R"
    printf '\n'

    printf '  %sBackup data%s\n' "$D" "$R"
    printf '    %s%-20s%s Backup data to cernbox\n' "$C" "hidra_backup_data" "$R"
    printf '\n'

    printf '  %sMount folders %s\n' "$D" "$R"
    printf '    %s%-20s%s Mount tracker data folder locally to /home/eudaq/daq/TB2026_TrackerData\n' "$C" "hidra_mount_tracker" "$R"
    printf '\n'


    printf '    %s%-20s%s show this help again\n' "$C" "hidra_help" "$R"
}

# Show the summary when sourced into an interactive shell (skip it for
# non-interactive use, e.g. a script that sources this only for the helpers).
if [[ $- == *i* ]]; then
    hidra_help
fi
