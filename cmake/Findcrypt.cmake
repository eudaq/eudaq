# - Try to find crypt
#  Once done, this will define
#
#  crypt_FOUND - system has crypt
#  crypt_INCLUDE_DIRS - the crypt include directories
#  crypt_LIBRARIES - link these to use crypt
#  refer to:
#  https://cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(crypt_PKGCONF crypt)

# Include dir
find_path(crypt_INCLUDE_DIR
  NAMES crypt.h
  PATHS ${crypt_PKGCONF_INCLUDE_DIRS}
)

# Finally the library itself
find_library(crypt_LIBRARY_SHARED
  NAMES ${CMAKE_SHARED_LIBRARY_PREFIX}crypt${CMAKE_SHARED_LIBRARY_SUFFIX}
  PATHS ${crypt_PKGCONF_LIBRARY_DIRS}
)

find_library(crypt_LIBRARY_STATIC
  NAMES ${CMAKE_STATIC_LIBRARY_PREFIX}crypt${CMAKE_STATIC_LIBRARY_SUFFIX}
  PATHS ${crypt_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(crypt_INCLUDE_DIR crypt_INCLUDE_DIRS)
set(crypt_LIBRARY_SHARED crypt_LIBRARIES_SHARED)
set(crypt_LIBRARY_STATIC crypt_LIBRARIES_STATIC)
libfind_process(crypt)

IF (crypt_INCLUDE_DIR)
	MESSAGE(" -- Found crypt header files in ${crypt_INCLUDE_DIR}.")
	MESSAGE(" -- Found crypt header (and dependencies) files in ${crypt_INCLUDE_DIRS}.")
ENDIF (crypt_INCLUDE_DIR)

IF (crypt_LIBRARY_SHARED)
	MESSAGE(" -- Found shared library crypt in ${crypt_LIBRARY_SHARED}.")	
	MESSAGE(" -- Found shared library  (and dependencies)  crypt in ${crypt_LIBRARIES_SHARED}.")	
ELSE (crypt_LIBRARY_SHARED)
	MESSAGE("Looked for dynamic crypt libraries named ${CRYPT_NAMES}.")
	MESSAGE("Could NOT find dynamic crypt library")
ENDIF (crypt_LIBRARY_SHARED)

IF (crypt_LIBRARY_STATIC)
	MESSAGE(" -- Found static library crypt in ${crypt_LIBRARY_STATIC}.")	
	MESSAGE(" -- Found static library  (and dependencies)  crypt in ${crypt_LIBRARIES_STATIC}.")	
ELSE (crypt_LIBRARY_STATIC)
	MESSAGE("Looked for static crypt libraries named ${CRYPT_NAMES}.")
	MESSAGE("Could NOT find static crypt library")
ENDIF (crypt_LIBRARY_STATIC)