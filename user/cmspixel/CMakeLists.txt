get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE module_${EUDAQ_USERNAME})
set(USER_HARDWARE_LIBRARY ${EUDAQ_USERNAME}_static)

option(USER_CMSPIXEL_BUILD "build cmspixel support" OFF)

if(NOT USER_CMSPIXEL_BUILD)
  message(STATUS "user/cmspixel is NOT to be built (USER_CMSPIXEL_BUILD=OFF)")
  return()
else()
  message(STATUS "user/cmspixel is to be built (USER_CMSPIXEL_BUILD=ON)")
endif()

include_directories(${EUDAQ_INCLUDE_DIRS})

FIND_PACKAGE(PXAR REQUIRED)

add_subdirectory(module)
