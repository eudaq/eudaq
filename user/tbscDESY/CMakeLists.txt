list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_LIST_DIR}/cmake)

get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)

set(EUDAQ_MODULE module_${EUDAQ_USERNAME})

option(USER_TBSCDESY_BUILD "build DESY Test Beam Slow Control" OFF)

if(NOT USER_TBSCDESY_BUILD)
    message(STATUS "user/tbscDESY is NOT to be built (USER_TBSCDESY_BUILD=OFF)")
  return()
endif()
message(STATUS "user/tbscDESY is to be built (USER_TBSCDESY_BUILD=ON)")

add_subdirectory(module)

##-- csv command line converter:
option(USER_TBSCDESY_EXE "build CSV Converter for TBSC DATA" ON)

if(USER_TBSCDESY_EXE)
  add_subdirectory(exe)
endif()
