project(EUDAQ)
set(${PROJECT_NAME}_VERSION_MAJOR 2 )
set(${PROJECT_NAME}_VERSION_MINOR 0 )
set(${PROJECT_NAME}_VERSION_PATCH 0 )

cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
include(CMakeDependentOption)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(CMakeCompatibility)
include(Platform)

set(CMAKE_SHARED_LIBRARY_PREFIX libeudaq_)
set(CMAKE_SHARED_MODULE_PREFIX  libeudaq_)
set(EUDAQ_CORE_LIBRARY core)
# set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS)

include_directories(main/lib/core/include)
include_directories(extern/include)

set(EUDAQ_LIB_VERSION ${EUDAQ_VERSION_MAJOR}.${EUDAQ_VERSION_MINOR}.${EUDAQ_VERSION_PATCH})
set(EUDAQ_LIB_VERSION_BASE ${EUDAQ_VERSION_MAJOR}.${EUDAQ_VERSION_MINOR}.${EUDAQ_VERSION_PATCH})
find_package(Git QUIET)
if(GIT_FOUND AND (IS_DIRECTORY ${PROJECT_SOURCE_DIR}/.git))
  execute_process(COMMAND ${GIT_EXECUTABLE} --work-tree=${PROJECT_SOURCE_DIR} describe --tags HEAD OUTPUT_VARIABLE EUDAQ_GIT_VERSION ERROR_VARIABLE EUDAQ_GIT_ERROR_DESCRIBE)
  execute_process(COMMAND ${GIT_EXECUTABLE} --work-tree=${PROJECT_SOURCE_DIR} status --porcelain ${PROJECT_SOURCE_DIR}/main OUTPUT_VARIABLE EUDAQ_GIT_MAIN_STATUS ERROR_VARIABLE EUDAQ_GIT_ERROR_STATUS)
  if(EUDAQ_GIT_ERROR_DESCRIBE OR EUDAQ_GIT_ERROR_STATUS)
    message(WARNING "Could not determine EUDAQ version from git -- git execution resulted in error: ${EUDAQ_GIT_ERROR_DESCRIBE} : ${EUDAQ_GIT_ERROR_STATUS}")
  else()
    string(REGEX REPLACE "([v0-9.]+[-a-zA-Z0-9]+)-([0-9]+)-([A-Za-z0-9]+)" "\\1+\\2~\\3" EUDAQ_GIT_LIB_VERSION ${EUDAQ_GIT_VERSION})
    string(REGEX REPLACE "\n" "" EUDAQ_GIT_VERSION ${EUDAQ_GIT_VERSION}) # remove potential line breaks
    string(REGEX REPLACE "v([v0-9.]+)(.*)" "\\1" EUDAQ_GIT_VERSION_BASE ${EUDAQ_GIT_VERSION})
    if("${EUDAQ_GIT_VERSION_BASE}" VERSION_LESS "${EUDAQ_LIB_VERSION_BASE}")
      message(WARNING "Your do not seem to have fetched the latest tags in your git repository -- please consider running 'git fetch upstream'")
      message("EUDAQ_GIT_VERSION_BASE=${EUDAQ_GIT_VERSION_BASE} EUDAQ_LIB_VERSION_BASE=${EUDAQ_LIB_VERSION_BASE}")
    endif()
    if(NOT (EUDAQ_GIT_MAIN_STATUS STREQUAL ""))
      message("the directory ${PROJECT_SOURCE_DIR}/main is dirty (uncommitted changes present). ${EUDAQ_GIT_MAIN_STATUS}")
    endif()
    set(EUDAQ_LIB_VERSION ${EUDAQ_GIT_VERSION})
  endif()
endif()
message(STATUS "Determined EUDAQ version ${EUDAQ_LIB_VERSION}")

configure_file("${PROJECT_SOURCE_DIR}/cmake/Config.cmake.hh" "${CMAKE_CURRENT_BINARY_DIR}/include/eudaq/Config.hh" @ONLY)
include_directories("${CMAKE_CURRENT_BINARY_DIR}/include")
file(GLOB INC_FILES "${CMAKE_CURRENT_BINARY_DIR}/include/eudaq/*.hh")
install(FILES ${INC_FILES} DESTINATION include/eudaq)

# set the path to which we will install later: default project home, can be changed using
set(EUDAQ_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}" CACHE PATH "Prefix prepended to install directories")
set(CMAKE_INSTALL_PREFIX "${EUDAQ_INSTALL_PREFIX}" CACHE INTERNAL "Prefix prepended to install directories" FORCE)
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_INSTALL_PREFIX}/extern)
## by mengqing: to have eudaq run with qt5 installed somewhere else@pc-kpix1
option (USE_CUSTOMIZED_QT5 "use customized qt5 directory, other than the centralized lib path."  ON)
if(USE_CUSTOMIZED_QT5)
  list(APPEND CMAKE_PREFIX_PATH "/opt/Qt5.9.3/")
else()
  message("QT5 used from central libraries, not a customized one!")
endif()
  
# Set the correct build type and allow command line options:
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'RelWithDebInfo' as none was specified.")
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build." FORCE)
endif()

# find necessary threading libraries (using C++11 threading)
find_package(Threads REQUIRED)
set(EUDAQ_THREADS_LIB ${CMAKE_THREAD_LIBS_INIT})

# see http://www.cmake.org/Wiki/CMake_RPATH_handling
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(EUDAQ_INSTALL_RPATH "@loader_path/../lib;@loader_path/../extern/lib")
else()
  set(EUDAQ_INSTALL_RPATH "\$ORIGIN/../lib:\$ORIGIN/../extern/lib")
endif()
set(CMAKE_SKIP_BUILD_RPATH  FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH ${EUDAQ_INSTALL_RPATH})
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

add_subdirectory(main)
add_subdirectory(user)
add_subdirectory(monitors)
add_subdirectory(extra)
add_subdirectory(gui)
add_subdirectory(doc)

# Uninstall Target #
if(NOT TARGET uninstall)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)
  add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()


# Python-based regression tests (http://www.cmake.org/Wiki/CMake/Testing_With_CTest).
# enable_testing()
# include(CTest)
# find_package(PythonInterp)
# if(PYTHONINTERP_FOUND AND BUILD_python)
#   execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import numpy" OUTPUT_QUIET ERROR_QUIET RESULT_VARIABLE NUMPYRC)
#   if(NUMPYRC EQUAL 1)
#     message(WARNING "Python interpreter found but 'numpy' package not available -- cannot use either Python wrapper nor regression tests.")
#   else()
#     message(STATUS "Found Python with installed numpy package -- making regression tests available through CTest")
#     include(etc/tests/testing.cmake)
#   endif()
# endif()

include(PrintSystemInformation)
