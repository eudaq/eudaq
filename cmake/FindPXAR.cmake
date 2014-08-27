# - Find PXAR installation
# This module tries to find the libftdi installation on your system.
# Once done this will define
#
#  PXAR_FOUND - system has ftdi
#  PXAR_INCLUDE_DIR - ~ the ftdi include directory 
#  PXAR_LIBRARY - Link these to use ftdi

FIND_PATH(PXAR_PATH
	NAMES core/api/api.h
	PATHS ~/pxar
)

FIND_LIBRARY(PXAR_LIBRARY
NAMES libpxar.so 
PATHS ~/pxar/lib
      /usr/lib
      /usr/local/lib
      /opt/local/lib
)

IF(PXAR_LIBRARY)
	IF(PXAR_PATH)
		set(PXAR_FOUND TRUE)
		MESSAGE(STATUS "Found libPXAR: ${PXAR_PATH}, ${PXAR_LIBRARY}")
	ELSE(PXAR_PATH)
		set(PXAR_FOUND FALSE)
		MESSAGE(STATUS "libPXAR headers NOT FOUND. Make sure to install the development headers!")
	ENDIF(PXAR_PATH)
ELSE(PXAR_LIBRARY)
	MESSAGE(STATUS "libPXAR NOT FOUND.")
ENDIF (PXAR_LIBRARY)
