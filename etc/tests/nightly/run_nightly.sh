#!/bin/bash
# this script runs continuous test using CMake's testing mechanisms and submits the results
# to the dashboard.
# use GNU screen to start this script from ssh sessions and then detach the session.

if [ -n "$1" ] 
then 
    WAKEUPAT="$1" # time to wake up every day in HH:MM in UTC
else
    WAKEUPAT="02:55" # time to wake up every day in HH:MM in UTC
fi


if [ -z "$DISPLAY" ]
then
    # if $DISPLAY is not set (e.g. on a VM or server), ROOT sents a warning to stderr e.g. when loading Marlin libraries; this might fail tests
    echo " Variable \$DISPLAY not set, set to 'localhost:0' (otherwise tests might fail due to ROOT error message)"
    export DISPLAY=localhost:0
fi

EUDAQDIR="$(readlink -f $(dirname $0)/../../..)"
BUILDDIR="$(readlink -f $(dirname $0)/../../../build)"

if [ ! -f "${EUDAQDIR}/main/include/eudaq/Documentation.hh" -o ! -d "$BUILDDIR" ]
then
    echo " ERROR: Could not identify EUDAQ source and/or build directory!";
    exit;
fi

echo " Using EUDAQ source directory: $EUDAQDIR"
echo " Using EUDAQ build directory: $BUILDDIR"

cd $EUDAQDIR
if (( $? )); then
    {
        echo " Could not change into EUDAQ source directory!";
        exit;
    }
fi;

# setup done!
echo " Waiting for my time to wake up ($WAKEUPAT UTC)... "

# infinite loop
while :; do
    now="$(date --utc +%H:%M)"
    if [[ "$now" = "$WAKEUPAT" ]]; then
	echo " it's $now, time to wake up!"
	echo " .. cleaning up .."
	make clean
	echo " .. running nightly checks .."
        ctest -S "$(dirname $0)/nightly.cmake" -D CTEST_CMAKE_GENERATOR="Unix Makefiles" -D WITH_MEMCHECK=ON -D CTEST_SOURCE_DIRECTORY="$EUDAQDIR" -D CTEST_BINARY_DIRECTORY="$BUILDDIR"
	echo " .. my job is done done for now, going to sleep on $(date) .. "
	sleep 59
    fi
    sleep 30
done
