get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE module_${EUDAQ_USERNAME})

option(USER_EXPERIMENTAL_BUILD "build experimental modules" ON)

if(NOT USER_EXPERIMENTAL_BUILD)
    message("disable building experimental modules (USER_EXPERIMENTAL_BUILD=OFF)")
  return()
endif()

add_subdirectory(module)
