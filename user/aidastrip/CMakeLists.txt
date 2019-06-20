##-- OPT: Compile AIDA strip?
option(USER_AIDASTRIP_BUILD "build AIDA strip" OFF)
if(NOT USER_AIDASTRIP_BUILD)
    message(STATUS "user/aidastrip is NOT to be built (USER_AIDASTRIP_BUILD=OFF)")
  return()
endif()
message(STATUS "user/aidastrip is to be built (USER_AIDASTRIP_BUILD=ON)")


##-- Choose which kpix version to use:
option(USER_AIDASTRIP_KPIX_OLD "Use old libkpix.so.1 with old kpix DAQ." OFF)
  
##-- Global Preparation:
#-- EUDAQ_MODULE needed for compilation at lower level CMakeLists
get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE module_${EUDAQ_USERNAME})

if(USE_KPIX_OLD)
  set(KPIX_INCLUDE_DIR /usr/local/include/kpixSw_3.10)
  set(KPIX_LIB_DIR /usr/local/lib/kpixSw_3.10)
else()
  set(KPIX_INCLUDE_DIR /usr/local/include/kpix)
  set(KPIX_LIB_DIR /usr/local/lib/kpix)
endif(USE_KPIX_OLD)
#message("[mengqing] KPIX_INCLUDE_DIR is ${KPIX_INCLUDE_DIR}")

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

