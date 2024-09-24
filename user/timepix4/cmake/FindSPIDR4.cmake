# - Try to find SPIDR
# Once done this will define
#  SPIDR4_FOUND - System has SPIDR4
#  SPIDR4_INCLUDE_DIR - The SPIDR4 main include directory
#  SPIDR4_LIB - The library for SPIDR4

MESSAGE(STATUS "Looking for SPIDR4...")

FIND_PATH(SPIDR4_INCLUDE_DIR NAMES "spidr4.h" PATHS "/usr/include" "${SPIDR4INCLUDE}" "$ENV{SPIDR4PATH}/software/SpidrTpx3Lib")
FIND_LIBRARY(SPIDR4_LIB NAMES "SpidrTpx3Lib" HINTS "/usr/lib" "${SPIDR4LIBS}" "$ENV{SPIDR4PATH}/software/Release")

MESSAGE(STATUS "-- SPIDR include at ${SPIDR4_INCLUDE_DIR} -- Found")
MESSAGE(STATUS "-- SPIDR library at ${SPIDR4_LIB} -- Found")

FIND_PACKAGE_HANDLE_STANDARD_ARGS(SPIDR4 REQUIRED_VARS SPIDR_LIB SPIDR_INCLUDE_DIR FAIL_MESSAGE "Could not find SPIDR4 from tpx4tools, make sure all necessary components are compiled and that the variable SPIDR4PATH points to the installation location:\n\n\t$ export SPIDR4PATH=/your/path/to/SPIDR\n\n")
