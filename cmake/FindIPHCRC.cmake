# - Try to find pxarCore used in native reader library for exporting RAW to pxarCore format needed for reconstruction
# Once done this will define
#  IPHCRC_FOUND - System has pxarCore
#  IPHCRC_INCLUDE_DIRS - The pxarCore include directories
#  IPHCRC_LIBRARIES - The libraries needed to use pxarCore
#  IPHCRC_DEFINITIONS - Compiler switches required for using pxarCore

MESSAGE(STATUS "Looking for IPHCRC dependencies: lib and header")

SET(CMAKE_FIND_LIBRARY_SUFFIXES ".lib" ".dll")

find_path(IPHCRC_INCLUDE_DIR iphc_run_ctrl_exp.h
  HINTS "${CMAKE_CURRENT_SOURCE_DIR}/../../extern/IPHCRC/include")

# looks for lib[name]
find_library(IPHCRC_LIBRARY NAMES iphc
  HINTS "${CMAKE_CURRENT_SOURCE_DIR}/../../extern/IPHCRC/lib")

set(IPHCRC_LIBRARIES ${IPHCRC_LIBRARY} )
set(IPHCRC_INCLUDE_DIRS ${IPHCRC_INCLUDE_DIR})

IF(IPHCRC_LIBRARY AND IPHCRC_INCLUDE_DIR)
   SET(IPHCRC_FOUND TRUE)
   MESSAGE(STATUS "Found IPHCRC library and headers.")
ELSE(IPHCRC_LIBRARY AND IPHCRC_INCLUDE_DIR)
  MESSAGE(STATUS "dir: " ${IPHCRC_INCLUDE_DIR} " Lib: " ${IPHCRC_LIBRARY})
ENDIF(IPHCRC_LIBRARY AND IPHCRC_INCLUDE_DIR)

mark_as_advanced(IPHCRC_LIBRARY IPHCRC_INCLUDE_DIR)
