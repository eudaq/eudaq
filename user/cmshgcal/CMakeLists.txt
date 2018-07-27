get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE module_${EUDAQ_USERNAME})

option(USER_EXAMPLE_BUILD "build user example" ON)

if(NOT USER_EXAMPLE_BUILD)
  message("disable building of user/example  (USER_EXAMPLE_BUILD=OFF)")
  return()
endif()

add_subdirectory(module)
