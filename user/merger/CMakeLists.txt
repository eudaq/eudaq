#list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_LIST_DIR}/cmake)
get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)

set(EUDAQ_MODULE module_${EUDAQ_USERNAME})
#set(USER_HARDWARE_LIBRARY ${EUDAQ_USERNAME}_static)

option(USER_MERGER_BUILD "build merger" OFF)

if(NOT USER_MERGER_BUILD)
    message(STATUS "user/merger is NOT to be built (USER_MERGER_BUILD=OFF)")
  return()
endif()
message(STATUS "user/merger is to be built (USER_MERGER_BUILD=ON)")

#include_directories(${EUDAQ_INCLUDE_DIRS})

add_subdirectory(exe)
