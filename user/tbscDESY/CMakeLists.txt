list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_LIST_DIR}/cmake)

get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)

set(EUDAQ_MODULE module_${EUDAQ_USERNAME})

option(USER_TBSCDESY "build DESY Test Beam Slow Control system" ON)

if(NOT USER_TBSCDESY)
  message("disable building of user/tbscDESY  (USER_TBSCDESY=OFF)")
  return()
endif()

add_subdirectory(module)
