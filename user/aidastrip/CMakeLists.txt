
##-- Check module dependencies:
find_package(LibXml2 QUIET)
message(STATUS "Its LIBXML2_INCLUDE_DIR is ${LIBXML2_INCLUDE_DIR}")

find_package(kpix QUIET)
set(KPIX_LIBRARY kpix::kpix)

##-- OPT: Compile AIDA strip?
cmake_dependent_option(USER_AIDASTRIP_BUILD "build AIDA strip" ON "LibXml2_FOUND;kpix_FOUND" OFF)


if(NOT USER_AIDASTRIP_BUILD)
    message(STATUS "user/aidastrip is NOT to be built (USER_AIDASTRIP_BUILD=OFF)")
  return()
endif()
message(STATUS "user/aidastrip is to be built (USER_AIDASTRIP_BUILD=ON)")


  
##-- Global Preparation:
##-- Choose which kpix version to use:
option(USER_AIDASTRIP_KPIX_OLD "Use old C++ kpix DAQ not NEW ROGUE-kpix DAQ." OFF)

#-- needed by EUDAQ_MODULE for compilation at lower level CMakeLists
get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE module_${EUDAQ_USERNAME})

##-- Adding sub-directories:
## 1) Main Producer + DC:
add_subdirectory(module)

## 2) ROOT analyzer/converter dependent on ROOT:

##- In case sometimes cmake did not find ROOT:
list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})

find_package(ROOT QUIET)
cmake_dependent_option(USER_AIDASTRIP_EXE "build aux. AIDASTRIP executables (requires ROOT)." ON
  "ROOT_FOUND" OFF)

if(USER_AIDASTRIP_EXE)
  add_subdirectory(exe)
endif()

##-- 3) customized GUI dependent on QT5:
find_package(Qt5Widgets QUIET)
cmake_dependent_option(USER_AIDASTRIP_GUI "Compile AIDA Strip GUI executables (requires QT5)" ON
  "Qt5Widgets_FOUND" OFF)

if(USER_AIDASTRIP_GUI)
  add_subdirectory(gui)
endif()

