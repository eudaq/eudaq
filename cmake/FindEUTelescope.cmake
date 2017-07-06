# - Try to find EUTelescope used in native reader library for exporting RAW to EUTelescope format needed for reconstruction
# Once done this will define
#  EUTELESCOPE_FOUND - System has EUTelescope
#  EUTELESCOPE_INCLUDE_DIRS - The EUTelescope include directories
#  EUTELESCOPE_LIBRARIES - The libraries needed to use EUTelescope
#  EUTELESCOPE_DEFINITIONS - Compiler switches required for using EUTelescope

  find_path(EUTELESCOPE_LIB_INCLUDE_DIR EUTELESCOPE.h
    HINTS "${EUTELESCOPE}/eutelescope/libraries/include" "$ENV{EUTELESCOPE}/eutelescope/libraries/include")
  
  find_path(EUTELESCOPE_INT_INCLUDE_DIR EUTelTrackerDataInterfacer.h
    HINTS "${EUTELESCOPE}/eutelescope/interfaces/pixel/include" "$ENV{EUTELESCOPE}/eutelescope/interfaces/pixel/include")

  find_library(EUTELESCOPE_LIBRARY NAMES Eutelescope
    HINTS "${EUTELESCOPE}/lib" "$ENV{EUTELESCOPE}/lib")

set(EUTELESCOPE_LIBRARIES ${EUTELESCOPE_LIBRARY} )
set(EUTELESCOPE_INCLUDE_DIRS ${EUTELESCOPE_LIB_INCLUDE_DIR} ${EUTELESCOPE_INT_INCLUDE_DIR} )
set(EUTELESCOPE_DEFINITIONS "-DUSE_EUTELESCOPE" )

IF(EUTELESCOPE_LIBRARIES AND EUTELESCOPE_INCLUDE_DIRS)
  MESSAGE(STATUS "Found EUTelescope: ${EUTELESCOPE_INCLUDE_DIRS}")
ELSE(EUTELESCOPE_LIBRARIES AND EUTELESCOPE_INCLUDE_DIRS)
  MESSAGE(STATUS "NOT found EUTelescope!")
ENDIF(EUTELESCOPE_LIBRARIES AND EUTELESCOPE_INCLUDE_DIRS)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set EUTELESCOPE_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(EUTelescope  DEFAULT_MSG
                                  EUTELESCOPE_LIBRARY EUTELESCOPE_LIB_INCLUDE_DIR EUTELESCOPE_INT_INCLUDE_DIR ) 

mark_as_advanced(EUTELESCOPE_INCLUDE_DIR EUTELESCOPE_LIBRARY )
