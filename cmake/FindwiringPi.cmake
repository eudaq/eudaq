# - Try to find wiringPi 
# Once done this will define
#  WIRINGPI_FOUND - System has wiringPi
#  WIRINGPI_INCLUDE_DIR - The wiringPi include directory
#  WIRINGPI_LIBRARY - The libraries needed to use wiringPi
#  WIRINGPI_DEFINITIONS - Compiler switches required for using wiringPi

MESSAGE(STATUS "Looking for RPiController dependencies: wiringPi...")

find_path(WIRINGPI_INCLUDE_DIR wiringPi.h
  HINTS "/usr/local/include" "/usr/include" "${PROJECT_SOURCE_DIR}/extern/wiringPi")

find_library(WIRINGPI_LIBRARY NAMES wiringPi
  HINTS "/usr/local/lib" "/usr/lib" "${PROJECT_SOURCE_DIR}/extern/wiringPi")

#include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set WIRINGPI_FOUND to TRUE
# if all listed variables are TRUE
#find_package_handle_standard_args(wiringPi
#  REQUIRED_VARS WIRINGPI_LIBRARY WIRINGPI_API_INCLUDE_DIR WIRINGPI_UTILS_INCLUDE_DIR PXAR_UTIL_INCLUDE_DIR)

IF(WIRINGPI_LIBRARY AND WIRINGPI_INCLUDE_DIR)
   SET(WIRINGPI_FOUND TRUE)
   MESSAGE(STATUS "Found wiringPi library and headers.")
ENDIF(WIRINGPI_LIBRARY AND WIRINGPI_INCLUDE_DIR)

mark_as_advanced(WIRINGPI_LIBRARY WIRINGPI_INCLUDE_DIR)
