get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE module_${EUDAQ_USERNAME})

option(USER_EXPERIMENTAL_BUILD "build user/experimental" OFF)

if(NOT USER_EXPERIMENTAL_BUILD)
    message(STATUS "user/experimental is NOT to be built (USER_EXPERIMENTAL_BUILD=OFF)")
  return()
endif()

message(STATUS "user/experimental is to be built (USER_EXPERIMENTAL_BUILD=ON)")

add_subdirectory(module)
