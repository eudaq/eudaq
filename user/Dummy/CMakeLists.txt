get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE ${EUDAQ_PREFIX}module_${EUDAQ_USERNAME})

option(USER_DUMMY_BUILD "build user/DUMMY" OFF)

if(NOT USER_DUMMY_BUILD)
    message(STATUS "user/Dummy is NOT to be built (USER_DUMMY_BUILD=OFF)")
  return()
endif()

message(STATUS "user/Dummy is to be built (USER_DUMMY_BUILD=ON)")

add_subdirectory(module)
