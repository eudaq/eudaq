list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_LIST_DIR}/cmake)
get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)

set(EUDAQ_MODULE module_${EUDAQ_USERNAME})

option(USER_FRANKENSERVER_BUILD "build user/Frankenserver" ON)

if(NOT USER_FRANKENSERVER_BUILD)
    message(STATUS "user/Frankenserver is NOT to be built (USER_FRANKENSERVER_BUILD=OFF)")
  return()
endif()
message(STATUS "user/Frankenserver is to be built (USER_FRANKENSERVER_BUILD=ON)")

include_directories(${EUDAQ_INCLUDE_DIRS})

add_subdirectory(module)
