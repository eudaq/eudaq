MESSAGE(STATUS "Looking for PIWrapper dependencies: dll-lib and header")

SET(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
##MESSAGE(${CMAKE_FIND_LIBRARY_SUFFIXES})

find_path(PIWrapper_INCLUDE_DIR PIWrapper.h 
  HINTS "${CMAKE_CURRENT_SOURCE_DIR}/../../producers/pi/include/")

##MESSAGE(${PIWrapper_INCLUDE_DIR})

# looks for lib[name]
find_library(PIWrapper_LIBRARY PI_GCS2_DLL
  HINTS "${CMAKE_CURRENT_SOURCE_DIR}/../../producers/pi/include/lib")

MESSAGE(STATUS "Library (DLL) header: " ${PIWrapper_LIBRARY})

set(PIWrapper_LIBRARIES ${PIWrapper_LIBRARY})
set(PIWrapper_INCLUDE_DIRS ${PIWrapper_INCLUDE_DIR})
##MESSAGE(${PIWrapper_INCLUDE_DIR})

IF(PIWrapper_LIBRARY AND PIWrapper_INCLUDE_DIR)
   SET(PIWrapper_FOUND TRUE)
   MESSAGE(STATUS "Found PI dll-library and header")
ELSE(PIWrapper_LIBRARY AND PIWrapper_INCLUDE_DIR)
  MESSAGE(STATUS "dir: " ${PIWrapper_INCLUDE_DIR} " Lib: " ${PIWrapper_LIBRARY})
ENDIF(PIWrapper_LIBRARY AND PIWrapper_INCLUDE_DIR)

mark_as_advanced(PIWrapper_LIBRARY PIWrapper_INCLUDE_DIR)
