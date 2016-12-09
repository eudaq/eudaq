# - Try to find TLUFirmware binary files needed for accessing the TLU
# Once done this will define
#  TLUFIRMWARE_FOUND - System has TLUFirmware
#  TLUFIRMWARE_DEFINITIONS - Compiler switches required for using TLUFirmware

macro(find_bitfiles_in_extern arg)
  find_path(TLUFIRMWARE_PATH TLU_Toplevel.bit
    HINTS ${CMAKE_CURRENT_LIST_DIR}/../extern/tlufirmware ${arg})
endmacro()

find_bitfiles_in_extern("")

if (NOT TLUFIRMWARE_PATH)
  MESSAGE(WARNING "Could not find TLUFirmware bitfiles required by tlu producer. Please refer to the documentation on how to obtain them.")
else()
  set(TLUFirmware_FOUND TRUE)
endif()

set(TLUFIRMWARE_DEFINITIONS "-DTLUFIRMWARE_PATH=\"${TLUFIRMWARE_PATH}\"" )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set TLUFIRMWARE_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(TLUFirmware  DEFAULT_MSG
                                  TLUFIRMWARE_PATH)

mark_as_advanced(TLUFIRMWARE_PATH )
