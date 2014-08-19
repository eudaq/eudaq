# - Find PXAR installation
# This module tries to find the libftdi installation on your system.
# Once done this will define
#
#  PXAR_FOUND - system has ftdi
#  PXAR_INCLUDE_DIR - ~ the ftdi include directory 
#  PXAR_LIBRARY - Link these to use ftdi

FIND_PATH(PXAR_INCLUDE_DIR api/api.h
HINTS   /home/cmspix/arthur/pxar/core
        /usr/local/include
        /usr/include
        /opt/local/include
)

FIND_LIBRARY(PXAR_LIBRARY
NAMES libpxar.so 
PATHS /home/cmspix/arthur/pxar/lib
      /usr/lib
      /usr/local/lib
      /opt/local/lib
)

FIND_LIBRARY(PXARUTIL_LIBRARY
NAMES libpxarutil.so
PATHS /home/cmspix/arthur/pxar/lib
      /usr/lib
      /usr/local/lib
      /opt/local/lib
)


IF (PXAR_LIBRARY)
  IF(PXARUTIL_LIBRARY)
    IF(PXAR_INCLUDE_DIR)
      set(PXAR_FOUND TRUE)
      MESSAGE(STATUS "Found libPXAR: ${PXAR_INCLUDE_DIR}, ${PXAR_LIBRARY}, ${PXARUTIL_LIBRARY}")
    ELSE(PXAR_INCLUDE_DIR)
      set(PXAR_FOUND FALSE)
      MESSAGE(STATUS "libPXAR headers NOT FOUND. Make sure to install the development headers!")
    ENDIF(PXAR_INCLUDE_DIR)
  ELSE(PXARUTIL_LIBRARY)
    set(PXAR_FOUND FALSE)
    MESSAGE(STATUS "libpxarutil NOT FOUND.")
  ENDIF(PXARUTIL_LIBRARY)
ELSE (PXAR_LIBRARY)
    set(PXAR_FOUND FALSE)
    MESSAGE(STATUS "libPXAR NOT FOUND.")
ENDIF (PXAR_LIBRARY)
