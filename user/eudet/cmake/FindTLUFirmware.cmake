# - Try to find TLUFirmware binary files needed for accessing the TLU
#  TLUFIRMWARE_FOUND - System has TLUFirmware
#  TLUFIRMWARE_DEFINITIONS - Compiler switches required for using TLUFirmware

file(GLOB_RECURSE EUDET_TLU_FIRMWARE_FOUND_LIST
  ${CMAKE_CURRENT_LIST_DIR}/../extern/*/TLU_Toplevel.bit)

# could not find the package at the usual locations -- try to copy from AFS if accessible
if (NOT EUDET_TLU_FIRMWARE_FOUND_LIST)
  if (EXISTS "/afs/desy.de/group/telescopes/tlu/tlufirmware")
    message(STATUS "Could not find tlu firmware package required by tlu producer; downloading it now via AFS to ${CMAKE_CURRENT_LIST_DIR}/../extern  ....")
    if(DEFINED ENV{TRAVIS})
      message(STATUS "Running on travis and therefore downloading only the absolutely necessary part of the tlu firmware.")
      file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../extern/tlufirmware)
      copy_files("/afs/desy.de/group/telescopes/tlu/tlufirmware/TLU_Toplevel.bit" ${CMAKE_CURRENT_LIST_DIR}/../extern/tlufirmware)
    else()
      copy_files("/afs/desy.de/group/telescopes/tlu/tlufirmware" ${CMAKE_CURRENT_LIST_DIR}/../extern)
    endif()
    find_zestsc1_in_extern(NO_DEFAULT_PATH)
  else()
      message(STATUS "Could not find tlu firmware package required by tlu producer. Please refer to the documentation on how to obtain the software.")
      return()
  endif()
  set(EUDET_TLU_FIRMWARE_FILE ${CMAKE_CURRENT_LIST_DIR}/../extern/tlufirmware)
  get_filename_component(EUDET_TLU_FIRMWARE_FILE ${EUDET_TLU_FIRMWARE_FILE} ABSOLUTE)
  get_filename_component(EUDET_TLU_FIRMWARE_DIR ${EUDET_TLU_FIRMWARE_FILE} DIRECTORY)
else()
  list(GET EUDET_TLU_FIRMWARE_FOUND_LIST 0 EUDET_TLU_FIRMWARE_FILE)
  get_filename_component(EUDET_TLU_FIRMWARE_FILE ${EUDET_TLU_FIRMWARE_FILE} ABSOLUTE)
  get_filename_component(EUDET_TLU_FIRMWARE_DIR ${EUDET_TLU_FIRMWARE_FILE} DIRECTORY)
endif()

if(EUDET_TLU_FIRMWARE_DIR)
  set(TLUFIRMWARE_PATH ${EUDET_TLU_FIRMWARE_DIR})
  set(TLUFIRMWARE_DEFINITIONS "-DTLUFIRMWARE_PATH=\"${EUDET_TLU_FIRMWARE_DIR}\"" )
  message(STATUS "Check for EUDET_TLU_FIRMWARE_DIR(TLU_Toplevel.bit): ${EUDET_TLU_FIRMWARE_DIR} -- ok")
else()
  message(WARNING "Check for EUDET_TLU_FIRMWARE_DIR (TLU_Toplevel.bit) -- false")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TLUFirmware DEFAULT_MSG EUDET_TLU_FIRMWARE_DIR)
