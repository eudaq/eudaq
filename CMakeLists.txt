########################################################
CMAKE_MINIMUM_REQUIRED(VERSION 2.6 FATAL_ERROR)
########################################################
# project name
PROJECT( EUDAQ )

# project version; before changing this, read README.release_procedure
SET( ${PROJECT_NAME}_VERSION_MAJOR 1 )
SET( ${PROJECT_NAME}_VERSION_MINOR 2 )
SET( ${PROJECT_NAME}_VERSION_PATCH 0 )

# some macros are redefined to keep compatability with CMake 2.6
include(${PROJECT_SOURCE_DIR}/cmake/CMakeCompatibility.cmake)

# Determine platform- and compiler-specific settings
include(${PROJECT_SOURCE_DIR}/cmake/Platform.cmake)

# optional debug defines
# add_definitions("-DDEBUG_NOTIMEOUT=1 -DDEBUG_TRANSPORT=1")

# set the path to which we will install later: default project home, can be changed using
# cmake -DINSTALL_PREFIX=/some/dir ..
SET(INSTALL_PREFIX "${PROJECT_SOURCE_DIR}" CACHE PATH "Prefix prepended to install directories")
SET(CMAKE_INSTALL_PREFIX "${INSTALL_PREFIX}" CACHE INTERNAL "Prefix prepended to install directories" FORCE)

# include directories
INCLUDE_DIRECTORIES( ./main/include )

# additional packages to be searched for by cmake
LIST( APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake )

#FIND_PACKAGE(Threads REQUIRED) # pthreads
# include checks for the available and necessary threading libraries (depending on platform)
include(${PROJECT_SOURCE_DIR}/cmake/EUDAQThreads.cmake)

# search for the packages needed for the main library when building the native reader library
option(BUILD_nreader "Compile native reader shared library (requires LCIO/EUTelescope)?" OFF)
IF(BUILD_nreader)
  FIND_PACKAGE(LCIO)
  INCLUDE_DIRECTORIES(${LCIO_INCLUDE_DIRS})
  FIND_PACKAGE(EUTelescope)
  INCLUDE_DIRECTORIES(${EUTELESCOPE_INCLUDE_DIRS})
  add_definitions(${EUTELESCOPE_DEFINITIONS} ${LCIO_DEFINITIONS})
ENDIF()


# SET THE RPATH SO THAT OUR EXECUTABLES FIND THE EUDAQ LIBRARY EVEN WHEN INSTALLED INTO NON-DEFAULT LOCATIONS
# see http://www.cmake.org/Wiki/CMake_RPATH_handling
# enable @rpath in the install name for any shared library being built
# note: it is planned that a future version of CMake will enable this by default
set(CMAKE_MACOSX_RPATH 1)
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

# build native reader used for data conversion into LCIO
IF(BUILD_nreader)
  add_subdirectory(nreader)
ENDIF()

# build GUI executables
option(BUILD_gui "Compile GUI executables (requires QT4/5)?" ON)
IF(BUILD_gui)
  add_subdirectory(gui)
ENDIF()

# build OnlineMon executable
option(BUILD_onlinemon "Compile Online Monitor executable (requires ROOT)?" OFF)
IF(BUILD_onlinemon)
  add_subdirectory(onlinemon)
ENDIF()


# include processors (will not be included in target 'all')
add_subdirectory(producers)
