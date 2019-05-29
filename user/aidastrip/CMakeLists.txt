##-- OPT: Compile AIDA strip?
option(USER_AIDASTRIP_BUILD "build AIDA strip" OFF)
if(NOT USER_AIDASTRIP_BUILD)
    message(STATUS "user/aidastrip is NOT to be built (USER_AIDASTRIP_BUILD=OFF)")
  return()
endif()
message(STATUS "user/aidastrip is to be built (USER_AIDASTRIP_BUILD=ON)")

##-- Global Preparation:
#-- EUDAQ_MODULE needed for compilation at lower level CMakeLists
get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE module_${EUDAQ_USERNAME})


##-- Main Producer + DC:
add_subdirectory(module)

##-- ROOT analyzer/converter needs ROOT:
list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
find_package(ROOT)

cmake_dependent_option(USER_AIDASTRIP_EXE "build aux. AIDASTRIP executables (requires ROOT)." ON
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
