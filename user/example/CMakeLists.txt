get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE ${EUDAQ_PREFIX}module_${EUDAQ_USERNAME})

option(USER_EXAMPLE_BUILD "build user/example" ON)

if(NOT USER_EXAMPLE_BUILD)
    message(STATUS "user/example is NOT to be built (USER_EXAMPLE_BUILD=OFF)")
  return()
endif()

message(STATUS "user/example is to be built (USER_EXAMPLE_BUILD=ON)")

add_subdirectory(module)
