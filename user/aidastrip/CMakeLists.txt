##-- EUDAQ_MODULE needed for compilation at lower level CMakeLists
get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE module_${EUDAQ_USERNAME})

option(USER_AIDASTRIP "build AIDA strip" ON)

if(NOT USER_AIDASTRIP)
  message("disable building of user/aidastrip  (USER_AIDASTRIP=OFF)")
  return()
endif()

add_subdirectory(module)

##-- root analyzer needs ROOT:
find_package(ROOT)
cmake_dependent_option(USER_AIDASTRIP_EXE "build aux. AIDASTRIP executables." ON
  "ROOT_FOUND" OFF)

if(USER_AIDASTRIP_EXE)
  add_subdirectory(exe)
endif()

##-- GUI needs QT5:
find_package(Qt5Widgets QUIET)

cmake_dependent_option(USER_AIDASTRIP_GUI "Compile AIDA Strip GUI executables (requires QT5)" ON
  "Qt5Widgets_FOUND" OFF)

if(USER_AIDASTRIP_GUI)
  add_subdirectory(gui)
endif()
