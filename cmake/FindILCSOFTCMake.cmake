# - search environment for ILCSOFT_CMAKE variable defining location of ILCSOFT's central CMake file
#   This provides modules to search for Marlin and friends needed by native reader library
# Once done this will define
#  ILCSOFT_CMAKE_FOUND - System has ILCSOFTCMake
#  ILCSOFT_CMAKE_PATH  - path to ILCSOFTCMake

if ($ENV{ILCSOFT_CMAKE})
  set ( ILCSOFT_CMAKE_PATH $ENV{ILCSOFT_CMAKE})
else()
  set ( ILCSOFT_CMAKE_PATH $ENV{ILCSOFT_CMAKE})
endif()

find_package_handle_standard_args(ILCSOFTCMake  DEFAULT_MSG  ILCSOFT_CMAKE_PATH)
