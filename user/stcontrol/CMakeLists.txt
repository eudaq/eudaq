list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_LIST_DIR}/cmake)
get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE module_${EUDAQ_USERNAME})

option(USER_STCONTROL_BUILD "build stcontrol" ON)

include_directories(${EUDAQ_INCLUDE_DIRS})

add_subdirectory(module)
