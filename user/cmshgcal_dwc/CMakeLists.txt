get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE module_${EUDAQ_USERNAME})

option(CMSHGCAL_DWC_BUILD "build cmshgcal dwc producer" OFF)

if(NOT CMSHGCAL_DWC_BUILD)
  message("disable building of user/cmshgcal_dwc  (CMSHGCAL_DWC_BUILD=OFF)")
  return()
endif()

add_subdirectory(module)
