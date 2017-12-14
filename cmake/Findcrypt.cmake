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
find_library(crypt_LIBRARY
  NAMES crypt
  PATHS ${crypt_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
libfind_process(crypt)

IF (crypt_INCLUDE_DIR)
	MESSAGE(" -- Found crypt header files in ${crypt_INCLUDE_DIR}.")
ENDIF (crypt_INCLUDE_DIR)

IF (crypt_LIBRARY)
	MESSAGE(" -- Found library crypt in ${crypt_LIBRARY}.")	
ELSE (crypt_LIBRARY)
	MESSAGE("Looked for crypt libraries named ${CRYPT_NAMES}.")
	MESSAGE("Could NOT find crypt library")
ENDIF (crypt_LIBRARY)
