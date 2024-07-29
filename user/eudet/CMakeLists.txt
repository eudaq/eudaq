list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_LIST_DIR}/cmake)
get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)

set(EUDAQ_MODULE ${EUDAQ_PREFIX}module_${EUDAQ_USERNAME})
set(USER_HARDWARE_LIBRARY ${EUDAQ_USERNAME}_static)

option(USER_EUDET_BUILD "build eudet" ON)

if(NOT USER_EUDET_BUILD)
  message(STATUS "user/eudet is NOT to be built (USER_EUDET_BUILD=OFF)")
  return()
endif()
message(STATUS "user/eudet is to be built (USER_EUDET_BUILD=ON)")

include_directories(${EUDAQ_INCLUDE_DIRS})

# for the NI producer
#option(USER_EUDET_BUILD_NI "build user/eudet Ni Mimosa DAQ" ON)
#set(USER_BUILD_NI ${USER_EUDET_BUILD_NI})

include_directories(hardware/include)
add_subdirectory(hardware)
add_subdirectory(module)
add_subdirectory(exe)
