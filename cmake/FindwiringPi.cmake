# - Try to find wiringPi 
# Once done this will define
#  wiringPi_FOUND - System has wiringPi
#  wiringPi_INCLUDE_DIRS - The wiringPi include directory
#  wiringPi_LIBRARIES - The libraries needed to use wiringPi

include(LibFindMacros)

# Dependencies
libfind_package(wiringPi rt)
libfind_package(wiringPi crypt)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(wiringPi_PKGCONF wiringPi)

find_path(wiringPi_INCLUDE_DIR 
  NAMES wiringPi.h
  HINTS wiringPi_PKGCONF_INCLUDE_DIRS  "/usr/local/include" "/usr/include" "${PROJECT_SOURCE_DIR}/extern/WiringPi")

find_library(wiringPi_LIBRARY NAMES 
  NAMES wiringPi
  HINTS wiringPi_PKGCONF_LIBRARY_DIRS "/usr/local/lib" "/usr/lib" "${PROJECT_SOURCE_DIR}/extern/WiringPi")

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
#set(wiringPi_PROCESS_INCLUDES wiringPi_INCLUDE_DIR RT_INCLUDE_DIRS)
#set(wiringPi_PROCESS_LIBS wiringPi_LIBRARY RT_LIBRARIES)
libfind_process(wiringPi)

IF (wiringPi_INCLUDE_DIR)
	MESSAGE(" -- Found wiringPi header files in ${wiringPi_INCLUDE_DIR}.")
ENDIF (wiringPi_INCLUDE_DIR)

IF (wiringPi_LIBRARY)
	MESSAGE(" -- Found shared library wiringPi in ${wiringPi_LIBRARY}.")	
ELSE (wiringPi_LIBRARY)
	MESSAGE("Looked for dynamic wiringPi libraries named ${wiringPi_NAMES}.")
ENDIF (wiringPi_LIBRARY)
