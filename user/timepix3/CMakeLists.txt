list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_LIST_DIR}/cmake)
get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE module_${EUDAQ_USERNAME})
set(USER_HARDWARE_LIBRARY ${EUDAQ_USERNAME}_static)

option(USER_TIMEPIX3_BUILD "build timepix3" OFF)

if(NOT USER_TIMEPIX3_BUILD)
  message("disable building of user/timepix3 (USER_TIMEPIX3_BUILD=OFF)")
  return()
endif()

include_directories(${EUDAQ_INCLUDE_DIRS})

find_package(SPIDR REQUIRED)
find_package(ROOT REQUIRED)

add_subdirectory(module)
