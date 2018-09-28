get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE module_${EUDAQ_USERNAME})

option(CMSHGCAL_MCP_BUILD "build cmshgcal mcp producer" ON)

if(NOT CMSHGCAL_MCP_BUILD)
  message("disable building of user/cmshgcal_dwc  (CMSHGCAL_MCP_BUILD=OFF)")
  return()
endif()

add_subdirectory(module)
