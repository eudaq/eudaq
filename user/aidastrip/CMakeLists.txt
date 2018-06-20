list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_LIST_DIR}/cmake)

get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)

set(EUDAQ_MODULE module_${EUDAQ_USERNAME})

option(USER_AIDASTRIP "build AIDA strip" ON)

if(NOT USER_AIDASTRIP)
  message("disable building of user/aidastrip  (USER_AIDASTRIP=OFF)")
  return()
endif()

add_subdirectory(module)
add_subdirectory(gui)

find_package(ROOT)
cmake_dependent_option(USER_AIDASTRIP_EXE "build aux. AIDASTRIP executables." ON
  "ROOT_FOUND" OFF)

if(USER_AIDASTRIP_EXE)
  add_subdirectory(exe)
endif()
