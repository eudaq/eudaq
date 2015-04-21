# - Try to find pxarCore used in native reader library for exporting RAW to pxarCore format needed for reconstruction
# Once done this will define
#  PXARCORE_FOUND - System has pxarCore
#  PXARCORE_INCLUDE_DIRS - The pxarCore include directories
#  PXARCORE_LIBRARIES - The libraries needed to use pxarCore
#  PXARCORE_DEFINITIONS - Compiler switches required for using pxarCore

MESSAGE(STATUS "Looking for CMSPixel dependencies: pxarCore...")

find_path(PXARCORE_API_INCLUDE_DIR api.h
  HINTS "${PXARPATH}/core/api" "$ENV{PXARPATH}/core/api")
find_path(PXARCORE_UTILS_INCLUDE_DIR dictionaries.h
  HINTS "${PXARPATH}/core/utils" "$ENV{PXARPATH}/core/utils")
find_path(PXARCORE_HAL_INCLUDE_DIR datasource_evt.h
  HINTS "${PXARPATH}/core/hal" "$ENV{PXARPATH}/core/hal")
find_path(PXAR_UTIL_INCLUDE_DIR ConfigParameters.hh
  HINTS "${PXARPATH}/util" "$ENV{PXARPATH}/util")

find_library(PXARCORE_LIBRARY NAMES pxar
  HINTS "${PXARPATH}/lib" "$ENV{PXARPATH}/lib")

set(PXARCORE_LIBRARIES ${PXARCORE_LIBRARY} )
set(PXARCORE_INCLUDE_DIRS ${PXARCORE_API_INCLUDE_DIR} ${PXARCORE_UTILS_INCLUDE_DIR} ${PXARCORE_HAL_INCLUDE_DIR} ${PXAR_UTIL_INCLUDE_DIR})

#include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set PXARCORE_FOUND to TRUE
# if all listed variables are TRUE
#find_package_handle_standard_args(pxarCore
#  REQUIRED_VARS PXARCORE_LIBRARY PXARCORE_API_INCLUDE_DIR PXARCORE_UTILS_INCLUDE_DIR PXAR_UTIL_INCLUDE_DIR)

IF(PXARCORE_LIBRARY AND PXARCORE_API_INCLUDE_DIR AND PXARCORE_UTILS_INCLUDE_DIR AND PXAR_UTIL_INCLUDE_DIR)
   SET(PXARCORE_FOUND TRUE)
   MESSAGE(STATUS "Found CMSPixel pxarCore library and headers.")
ENDIF(PXARCORE_LIBRARY AND PXARCORE_API_INCLUDE_DIR AND PXARCORE_UTILS_INCLUDE_DIR AND PXAR_UTIL_INCLUDE_DIR)

mark_as_advanced(PXARCORE_LIBRARY PXARCORE_API_INCLUDE_DIR PXARCORE_UTILS_INCLUDE_DIR PXAR_UTIL_INCLUDE_DIR)