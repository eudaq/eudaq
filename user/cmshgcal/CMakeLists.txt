get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE module_${EUDAQ_USERNAME})

option(CMSHGCAL_BUILD "build user example" OFF)

if(NOT CMSHGCAL_BUILD)
  message("disable building of user/cmshgcal  (CMSHGCAL_BUILD=OFF)")
  return()
endif()

add_subdirectory(module)
