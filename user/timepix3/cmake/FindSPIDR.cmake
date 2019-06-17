# - Try to find SPIDR
# Once done this will define
#  SPIDR_FOUND - System has SPIDR
#  SPIDR_INCLUDE_DIR - The SPIDR main include directory
#  SPIDR_LIB - The library for SPIDR

MESSAGE(STATUS "Looking for SPIDR...")

FIND_PATH(SPIDR_INCLUDE_DIR NAMES "SpidrController.h" PATHS "/usr/include" "${SPIDRINCLUDE}" "$ENV{SPIDRPATH}/software/SpidrTpx3Lib")
FIND_LIBRARY(SPIDR_LIB NAMES "SpidrTpx3Lib" HINTS "/usr/lib" "${SPIDRLIBS}" "$ENV{SPIDRPATH}/software/Release")

MESSAGE(STATUS "-- SPIDR include at ${SPIDR_INCLUDE_DIR} -- Found")
MESSAGE(STATUS "-- SPIDR library at ${SPIDR_LIB} -- Found")

FIND_PACKAGE_HANDLE_STANDARD_ARGS(SPIDR REQUIRED_VARS SPIDR_LIB SPIDR_INCLUDE_DIR FAIL_MESSAGE "Could not find SPIDR, make sure all necessary components are compiled and that the variable SPIDRPATH points to the installation location:\n$ export SPIDRPATH=/your/path/to/SPIDR\n")
