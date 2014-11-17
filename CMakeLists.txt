########################################################
CMAKE_MINIMUM_REQUIRED(VERSION 2.6 FATAL_ERROR)
########################################################
# project name
PROJECT( EUDAQ )

# project version
SET( ${PROJECT_NAME}_VERSION_MAJOR 2 )
SET( ${PROJECT_NAME}_VERSION_MINOR 0 )
SET( ${PROJECT_NAME}_VERSION_PATCH 0 )

LIST ( APPEND CMAKE_CXX_FLAGS "-fPIC -O2" )
LIST ( APPEND CMAKE_LD_FLAGS "-fPIC -O2" )

# some macros are redefined to keep compatability with CMake 2.6
include(${PROJECT_SOURCE_DIR}/cmake/CMakeCompatibility.cmake)

# Determine platform- and compiler-specific settings
include(${PROJECT_SOURCE_DIR}/cmake/Platform.cmake)

# Check if this is a source tarball build
IF(NOT IS_DIRECTORY ${CMAKE_SOURCE_DIR}/.git)
  SET(SOURCE_PACKAGE 1)
ENDIF(NOT IS_DIRECTORY ${CMAKE_SOURCE_DIR}/.git)

# Set package version
IF(NOT SOURCE_PACKAGE)
  # Get the version from last git tag plus numer of additional commits:
  FIND_PACKAGE(Git QUIET)
  IF(GIT_FOUND)
    EXECUTE_PROCESS(COMMAND ${GIT_EXECUTABLE} describe --tags HEAD WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE EUDAQ_LIB_VERSION ERROR_VARIABLE GIT_ERROR)
    IF(GIT_ERROR) # git execution resulted in error message -- cannot trust result but construct version from hard-set variable
      MESSAGE(WARNING "Could not determine EUDAQ version from git -- git execution resulted in error: ${GIT_ERROR}")
      SET(EUDAQ_LIB_VERSION ${EUDAQ_VERSION_MAJOR}.${EUDAQ_VERSION_MINOR}.${EUDAQ_VERSION_PATCH})
    ELSE(GIT_ERROR)
      STRING(REGEX REPLACE "([v0-9.]+[-a-zA-Z0-9]+)-([0-9]+)-([A-Za-z0-9]+)" "\\1+\\2~\\3" EUDAQ_LIB_VERSION ${EUDAQ_LIB_VERSION})
      STRING(REGEX REPLACE "\n" "" EUDAQ_LIB_VERSION ${EUDAQ_LIB_VERSION}) # remove potential line breaks
      STRING(REGEX REPLACE "v([v0-9.]+)(.*)" "\\1" EUDAQ_LIB_BASEVERSION ${EUDAQ_LIB_VERSION})
      IF("${EUDAQ_LIB_BASEVERSION}" VERSION_LESS "${EUDAQ_VERSION_MAJOR}.${EUDAQ_VERSION_MINOR}.${EUDAQ_VERSION_PATCH}")
	MESSAGE(WARNING "Your do not seem to have fetched the latest tags in your git repository -- please consider running 'git fetch upstream'")
      ENDIF("${EUDAQ_LIB_BASEVERSION}" VERSION_LESS "${EUDAQ_VERSION_MAJOR}.${EUDAQ_VERSION_MINOR}.${EUDAQ_VERSION_PATCH}")
      EXEC_PROGRAM(git ARGS status --porcelain ${CMAKE_CURRENT_SOURCE_DIR}/main OUTPUT_VARIABLE EUDAQ_MAIN_STATUS)
      IF(EUDAQ_MAIN_STATUS STREQUAL "")
	MESSAGE(STATUS "Main EUDAQ library directory is clean (all changes have been commited).")
      ELSE(EUDAQ_MAIN_STATUS STREQUAL "")
	MESSAGE(STATUS "Main EUDAQ library directory is dirty (uncommitted changes present):\n ${EUDAQ_MAIN_STATUS}.")
	SET(EUDAQ_LIB_VERSION ${EUDAQ_LIB_VERSION}*)
      ENDIF(EUDAQ_MAIN_STATUS STREQUAL "")
    ENDIF(GIT_ERROR)
  ELSE(GIT_FOUND)
    # If we don't have git we take the hard-set version.
    SET(EUDAQ_LIB_VERSION ${EUDAQ_VERSION_MAJOR}.${EUDAQ_VERSION_MINOR}.${EUDAQ_VERSION_PATCH})
  ENDIF(GIT_FOUND)
ELSE(NOT SOURCE_PACKAGE)
  # If we don't have git we take the hard-set version.
  SET(EUDAQ_LIB_VERSION "v${EUDAQ_VERSION_MAJOR}.${EUDAQ_VERSION_MINOR}.${EUDAQ_VERSION_PATCH}")
ENDIF(NOT SOURCE_PACKAGE)
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.cmake.h" "${CMAKE_CURRENT_BINARY_DIR}/config.h" @ONLY)
INCLUDE_DIRECTORIES("${CMAKE_CURRENT_BINARY_DIR}")
MESSAGE("-- Determined EUDAQ version ${EUDAQ_LIB_VERSION}")

# set the path to which we will install later: default project home, can be changed using
# cmake -DINSTALL_PREFIX=/some/dir ..
SET(INSTALL_PREFIX "${PROJECT_SOURCE_DIR}" CACHE PATH "Prefix prepended to install directories")
SET(CMAKE_INSTALL_PREFIX "${INSTALL_PREFIX}" CACHE INTERNAL "Prefix prepended to install directories" FORCE)

# optional debug defines
# add_definitions("-DDEBUG_NOTIMEOUT=1 -DDEBUG_TRANSPORT=1")

# include directories
INCLUDE_DIRECTORIES( ./main/include )
INCLUDE_DIRECTORIES( ./extern/jsoncons-0.93/src )

# additional packages to be searched for by cmake
LIST( APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake )

# find necessary threading libraries (using C++11 threading)
Find_Package(Threads REQUIRED)
set(EUDAQ_THREADS_LIB ${CMAKE_THREAD_LIBS_INIT})

# search for the packages needed for the main library when building the native reader library
option(BUILD_nreader "Compile native reader shared library (requires LCIO/EUTelescope)?" OFF)
IF(BUILD_nreader)
  FIND_PACKAGE(LCIO REQUIRED)
  INCLUDE_DIRECTORIES(${LCIO_INCLUDE_DIRS})
  FIND_PACKAGE(EUTelescope REQUIRED)
  INCLUDE_DIRECTORIES(${EUTELESCOPE_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/nreader/include)
  add_definitions(${EUTELESCOPE_DEFINITIONS} ${LCIO_DEFINITIONS})
  AUX_SOURCE_DIRECTORY( ${PROJECT_SOURCE_DIR}/nreader/detdescription detdescr_library_sources )
  ADD_LIBRARY( DetectorDescriptons SHARED ${detdescr_library_sources} )
  INSTALL(TARGETS DetectorDescriptons RUNTIME DESTINATION bin
                                      LIBRARY DESTINATION lib
                                      ARCHIVE DESTINATION lib)
  set(ADDITIONAL_LIBRARIES ${EUTELESCOPE_LIBRARIES} DetectorDescriptons)
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

# build EUDAQ python exe bindings
option(BUILD_python "Compile Python EUDAQ binding library?" ON)
IF(BUILD_python)
  add_subdirectory(main/python)
ENDIF()

# build EUDAQ pybindgen bindings
option(BUILD_pybindgen "Compile pybindgen binding libraries?" OFF)
IF(BUILD_pybindgen)
  add_subdirectory(etc/pybindgen)
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
option(BUILD_onlinemon "Compile Online Monitor executable (requires ROOT)?" ON)
IF(BUILD_onlinemon)
  add_subdirectory(monitors/onlinemon)
ENDIF()

# build resender Producer
option(BUILD_resender "Compile resender Producer?" OFF)
IF(BUILD_resender)
  add_subdirectory(reSendProducer)
ENDIF()

# build Offline Monitor
option(BUILD_offlinemon "Compile Offline Monitor executable (requires ROOT)?" OFF)
IF(BUILD_offlinemon)
  add_subdirectory(monitors/offlinemon)
ENDIF()

# build Metadata Monitor
option(BUILD_metamon "Compile MetaData Monitor executable" OFF)
IF(BUILD_metamon)
  add_subdirectory(monitors/metamon)
ENDIF()



###############################################
# LaTeX target to compile the PDF user manual #
###############################################
# Information and the manual on the UseLATEX.cmake package can be found here
# http://www.cmake.org/Wiki/CMakeUserUseLATEX
# The code resides here:
# https://github.com/kmorel/UseLATEX
option(BUILD_manual "Generate user's manual as PDF? (requires LaTeX)?" OFF)
IF(BUILD_manual)
  add_subdirectory(doc/manual)
ENDIF()

#############################################
# Doxygen target to generate HTML reference #
#############################################
FIND_PACKAGE(Doxygen)
IF(DOXYGEN_FOUND)
  CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/doc/doxygen/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/doc/doxygen/Doxyfile @ONLY)
  ADD_CUSTOM_TARGET(doxygen
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doc/doxygen/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/doc/doxygen
    COMMENT "Generating HTML documentation with Doxygen" VERBATIM
    )
ENDIF(DOXYGEN_FOUND)


# include processors (will not be included in target 'all')
add_subdirectory(producers)

#######################################################
# Print out system information useful for bug reports #
############################################### #######
include(PrintSystemInformation)


###############################################
# Python-based regression tests
###############################################
# if the Python interpreter can be found and the numpy package is
# installed, we can make available the Python-based regression tests
# through CMake/CTest; for more detailed information see the CMake manual or
#web page (http://www.cmake.org/Wiki/CMake/Testing_With_CTest).
if (${CMAKE_VERSION} VERSION_GREATER 2.8)
  # git support only available with CMake greater 2.8
  ENABLE_TESTING()
  INCLUDE(CTest)
  FIND_PACKAGE(PythonInterp)
  IF(PYTHONINTERP_FOUND AND BUILD_python)
    # test for numpy package
    EXECUTE_PROCESS( COMMAND ${PYTHON_EXECUTABLE} -c "import numpy" OUTPUT_QUIET ERROR_QUIET RESULT_VARIABLE NUMPYRC)
    IF(NUMPYRC EQUAL 1)
      MESSAGE(WARNING "Python interpreter found but 'numpy' package not available -- cannot use either Python wrapper nor regression tests.")
    ELSE(NUMPYRC EQUAL 1)
      MESSAGE(STATUS "Found Python with installed numpy package -- making regression tests available through CTest")
      INCLUDE(etc/tests/testing.cmake)
    ENDIF(NUMPYRC EQUAL 1)
  ENDIF(PYTHONINTERP_FOUND AND BUILD_python)
ELSE(${CMAKE_VERSION} VERSION_GREATER 2.8)
  MESSAGE(WARNING "Cannot setup CTest: requires CMake version 2.8 or later (found ${CMAKE_VERSION})")
ENDIF(${CMAKE_VERSION} VERSION_GREATER 2.8)

MESSAGE (STATUS "*** Build Type ${CMAKE_BUILD_TYPE} ***" )
MESSAGE (STATUS "*** Compiler Flags:  ${CMAKE_CXX_FLAGS}"  )
MESSAGE (STATUS "*** Compiler Flags:  ${CMAKE_LD_FLAGS}"  )
