# - Check for the presence of RT
#
# The following variables are set when RT is found:
#  rt_FOUND       = Set to true, if all components of RT
#                          have been found.
#  rt_INCLUDE_DIRS   = Include path for the header files of RT
#  rt_LIBRARIES  = Link these to use RT

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(rt_PKGCONF rt)

## -----------------------------------------------------------------------------
## Check for the header files

find_path (rt_INCLUDE_DIR 
  NAMES time.h
  PATHS ${rt_PKGCONF_INCLUDE_DIRS} /usr/local/include /usr/include ${CMAKE_EXTRA_INCLUDES}
  )

## -----------------------------------------------------------------------------
## Check for the library

find_library (rt_LIBRARY 
  NAMES rt
  PATHS ${rt_PKGCONF_LIBRARY_DIRS} /usr/local/lib /usr/lib /lib ${CMAKE_EXTRA_LIBRARIES}
  )

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
libfind_process(rt)

IF (rt_INCLUDE_DIR)
	MESSAGE(" -- Found rt header files in ${rt_INCLUDE_DIR}.")
ENDIF (rt_INCLUDE_DIR)

IF (rt_LIBRARY)
	MESSAGE(" -- Found shared library rt in ${rt_LIBRARY}.")	
ELSE (rt_LIBRARY)
	MESSAGE("Looked for dynamic rt libraries named ${rt_NAMES}.")
ENDIF (rt_LIBRARY)