list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_LIST_DIR}/cmake)
get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE module_${EUDAQ_USERNAME})

option(USER_STCONTROL_BUILD "build user/stcontrol" OFF)

if(NOT USER_STCONTROL_BUILD)
    message(STATUS "user/stcontrol is NOT to be built (USER_STCONTROL_BUILD=OFF)")
  return()
else()
  message(STATUS "user/stcontrol is to be built (USER_STCONTROL_BUILD=ON)")
endif()

include_directories(${EUDAQ_INCLUDE_DIRS})

add_subdirectory(module)
