#!/bin/bash

####################################################################################
# EUDAQ code beautifier script
# This script uses clang-format to standardize our indentation/braces etc
# according to LLVM specifications.
#
# DO NOT RUN THIS SCRIPT on anything else than your own producer code!
# It will be executed for the main EUDAQ libraries regularly by the main developers.
####################################################################################

# change if your executable is named different
CLANG_FORMAT=clang-format-3.6

# add all the files and directories that may not be reformatted, 
# relative to the project's root directory
IGNORE_SET=(
    main/lib/plugins
    extern
    etc
    producers
)

####################################################################################

function join { local IFS="$1"; shift; echo "$*"; }
IGNORE_STRING=$(join \| "${IGNORE_SET[@]}")

SOURCES=$(find . | egrep -v ${IGNORE_STRING} | egrep "\.h$|\.hh$|\.c$|\.cc$")

for FILE in $SOURCES
do
  ${CLANG_FORMAT} -i $FILE
done
