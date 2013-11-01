########################################################
CMAKE_MINIMUM_REQUIRED(VERSION 2.6 FATAL_ERROR)
########################################################


# project name
PROJECT( EUDAQ )

# project version; before changing this, read README.release_procedure
SET( ${PROJECT_NAME}_VERSION_MAJOR 1 )
SET( ${PROJECT_NAME}_VERSION_MINOR 2 )
SET( ${PROJECT_NAME}_VERSION_PATCH 0 )


# include directories
INCLUDE_DIRECTORIES( ./main/include )

# platform dependent preprocessor defines
if (WIN32) 
  if (CYGWIN)
    ADD_DEFINITIONS("-DEUDAQ_PLATFORM=PF_CYGWIN")
  elseif (MINGW)
    ADD_DEFINITIONS("-DEUDAQ_PLATFORM=PF_MINGW")
  else()
    ADD_DEFINITIONS("-DEUDAQ_PLATFORM=PF_WIN32")
  endif()
elseif (UNIX)
  if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    ADD_DEFINITIONS("-DEUDAQ_PLATFORM=PF_MACOSX")
  else()
    ADD_DEFINITIONS("-DEUDAQ_PLATFORM=PF_LINUX")
  endif()
else()
  MESSAGE( "WARNING: Platform not defined in CMakeLists.txt -- assuming Unix/Linux (good luck)." )
  ADD_DEFINITIONS("-DEUDAQ_PLATFORM=PF_LINUX")
endif()

# set the path to which we will install later: default project home, can be changed using
# cmake -DINSTALL_PREFIX=/some/dir ..
SET(INSTALL_PREFIX "${PROJECT_SOURCE_DIR}" CACHE PATH "Prefix prepended to install directories")
SET(CMAKE_INSTALL_PREFIX "${INSTALL_PREFIX}" CACHE INTERNAL "Prefix prepended to install directories" FORCE)

# additional packages to be searched for by cmake
LIST( APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake )

# search for the packages needed for the main library
FIND_PACKAGE(Threads)


# SET THE RPATH SO THAT OUR EXECUTABLES FIND THE EUDAQ LIBRARY EVEN WHEN INSTALLED INTO NON-DEFAULT LOCATIONS
# see http://www.cmake.org/Wiki/CMake_RPATH_handling
# use, i.e. don't skip the full RPATH for the build tree
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)
# when building, don't use the install RPATH already
# (but later on when installing)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) 
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
# *ALTERNATIVELY*: USE RELATIVE PATHS
# see http://www.semipol.de/archives/356
# for all binaries created in a CMake project:
#SET(CMAKE_INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN/")
# for certain targets
#SET_TARGET_PROPERTIES(target 1 target2 ...
#    PROPERTIES INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN/")
# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
# the RPATH to be used when installing, but only if it's not a system directory
LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
IF("${isSystemDir}" STREQUAL "-1")
   SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
ENDIF("${isSystemDir}" STREQUAL "-1")

# build main EUDAQ library
add_subdirectory(main/lib)

# build main EUDAQ executables (with option to disable them, use 'cmake -DMainExec=OFF ..'
option(BUILD_main "Compile main EUDAQ executables?" ON)
IF(BUILD_main)
  add_subdirectory(main/exe)
ENDIF()

# build GUI executables
option(BUILD_gui "Compile Qt4 GUI?" ON)
IF(BUILD_gui)
  add_subdirectory(gui)
ENDIF()


# include processors (will not be included in target 'all')
add_subdirectory(producers)
