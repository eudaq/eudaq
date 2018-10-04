if(WIN32)
  message("user/calice BIF_AHCAL is not to be built on windows (fixme)")
  return()
endif()

find_package(CACTUS)
if (NOT CACTUS_FOUND)
  message("user/calice BIF_AHCAL cannot be used, because CACTUS was not found")
endif()

cmake_dependent_option(USER_CALICE_BUILD_BIF "build user/calice BIF" ON
  "USER_CALICE_BUILD;CACTUS_FOUND" OFF)

get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE module_${EUDAQ_USERNAME}) 

if(NOT USER_CALICE_BUILD_BIF)
  message("user/calice BIF is not to be built")
  return()
endif()

add_subdirectory(module)
