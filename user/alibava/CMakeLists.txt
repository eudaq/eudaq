get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE ${EUDAQ_PREFIX}module_${EUDAQ_USERNAME})

option(USER_ALIBAVA_BUILD "build user/alibava" OFF)

if(NOT USER_ALIBAVA_BUILD)
    message(STATUS "user/alibava is NOT to be built (USER_ALIBAVA_BUILD=OFF)")
  return()
endif()

message(STATUS "user/alibava is to be built (USER_ALIBAVA_BUILD=ON)")

add_subdirectory(module)
