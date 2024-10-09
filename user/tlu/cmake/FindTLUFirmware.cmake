# - Try to find TLUFirmware binary files needed for accessing the TLU
# Once done this will define
#  TLUFIRMWARE_FOUND - System has TLUFirmware
#  TLUFIRMWARE_DEFINITIONS - Compiler switches required for using TLUFirmware

include(FetchContent)

macro(find_bitfiles_in_extern arg)
  find_path(TLUFIRMWARE_PATH TLU_Toplevel.bit
    HINTS ${PROJECT_SOURCE_DIR}/extern/tlufirmware ${arg})
endmacro()

find_bitfiles_in_extern("")

# could not find the package at the usual locations -- it will try to copy from github
if (NOT TLUFIRMWARE_PATH)
#  IF (EXISTS "/afs/desy.de/group/telescopes/tlu/tlufirmware")
#    MESSAGE(STATUS "Could not find TLUFirmware bitfiles required by tlu producer; downloading them now via AFS to ./extern ....")
#    FILE(COPY "/afs/desy.de/group/telescopes/tlu/tlufirmware" DESTINATION ${PROJECT_SOURCE_DIR}/extern)
#    find_bitfiles_in_extern(NO_DEFAULT_PATH)
#  ELSE()
#    MESSAGE(WARNING "Could not find TLUFirmware bitfiles required by tlu producer. Please refer to the documentation on how to obtain them.  Converter will be built anyway.")
#  ENDIF()
  FetchContent_Declare(
    tlu-dependencies
    GIT_REPOSITORY    https://github.com/eudaq/tlu-dependencies.git
    GIT_TAG           origin/master
    GIT_PROGRESS TRUE
    GIT_SHALLOW  TRUE
  ) 
  
  IF(NOT tlu-dependencies_POPULATED)
    FetchContent_Populate( tlu-dependencies )
  ENDIF()
  
  FetchContent_GetProperties( tlu-dependencies )
  MESSAGE(${tlu-dependencies_SOURCE_DIR})
  find_bitfiles_in_extern(${tlu-dependencies_SOURCE_DIR}/tlufirmware)
endif()

set(TLUFIRMWARE_DEFINITIONS "-DTLUFIRMWARE_PATH=\"${TLUFIRMWARE_PATH}\"" )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set TLUFIRMWARE_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(TLUFirmware  DEFAULT_MSG
                                  TLUFIRMWARE_PATH)

mark_as_advanced(TLUFIRMWARE_PATH )
