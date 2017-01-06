# - Try to find TLUFirmware binary files needed for accessing the TLU
#  TLUFIRMWARE_FOUND - System has TLUFirmware
#  TLUFIRMWARE_DEFINITIONS - Compiler switches required for using TLUFirmware

file(GLOB_RECURSE EUDET_TLU_FIRMWARE_FOUND_LIST
  ${CMAKE_CURRENT_LIST_DIR}/../extern/*/TLU_Toplevel.bit)

list(GET EUDET_TLU_FIRMWARE_FOUND_LIST 0 EUDET_TLU_FIRMWARE_FILE)
get_filename_component(EUDET_TLU_FIRMWARE_FILE ${EUDET_TLU_FIRMWARE_FILE} ABSOLUTE)
get_filename_component(EUDET_TLU_FIRMWARE_DIR ${EUDET_TLU_FIRMWARE_FILE} DIRECTORY)
if(EUDET_TLU_FIRMWARE_DIR)
  set(TLUFIRMWARE_PATH ${EUDET_TLU_FIRMWARE_DIR})
  set(TLUFIRMWARE_DEFINITIONS "-DTLUFIRMWARE_PATH=\"${EUDET_TLU_FIRMWARE_DIR}\"" )
  message(STATUS "Check for EUDET_TLU_FIRMWARE_DIR(TLU_Toplevel.bit): ${EUDET_TLU_FIRMWARE_DIR} -- ok")
else()
  message(WARNING "Check for EUDET_TLU_FIRMWARE_DIR (TLU_Toplevel.bit) -- false")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TLUFirmware DEFAULT_MSG EUDET_TLU_FIRMWARE_DIR)
