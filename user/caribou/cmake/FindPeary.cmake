# - Try to find the Peary framework
# Once done this will define
#  Peary_FOUND - System has Peary
#  Peary_INCLUDE_DIRS - The Peary main include directory
#  Peary_LIBRARIES - The libraries for Peary and the required components

MESSAGE(STATUS "Looking for Peary...")
FIND_PATH(Peary_INCLUDE_DIR NAMES "peary/device/device.hpp" PATHS "/usr/lib" "${PEARYINCLUDE}" "$ENV{PEARYPATH}")
FIND_LIBRARY(Peary_LIBRARIES NAMES "peary" HINTS "${PEARYLIBS}" "$ENV{PEARYPATH}/lib")

LIST(APPEND Peary_INCLUDE_DIRS "${Peary_INCLUDE_DIR}/peary")
LIST(APPEND Peary_INCLUDE_DIRS "${Peary_INCLUDE_DIR}/devices")

IF(Peary_FIND_COMPONENTS)
   FOREACH(component ${Peary_FIND_COMPONENTS})
      FIND_PATH(Peary_COMP_INCLUDE NAMES "devices/${component}/${component}Device.hpp" PATHS "${PEARYINCLUDE}" "$ENV{PEARYPATH}")
      FIND_LIBRARY(Peary_COMP_LIB NAMES "PearyDevice${component}" HINTS "${PEARYLIBS}" "$ENV{PEARYPATH}/lib")
      IF(Peary_COMP_INCLUDE AND Peary_COMP_LIB)
         LIST(APPEND Peary_LIBRARIES "${Peary_COMP_LIB}")
         LIST(APPEND Peary_INCLUDE_DIRS "${Peary_COMP_INCLUDE}/devices/${component}")
         SET(Peary_${component}_FOUND TRUE)
         MESSAGE(STATUS "Looking for Peary component: ${component} -- Found")
      ELSE()
         MESSAGE(STATUS "Looking for Peary component: ${component} -- NOTFOUND")
      ENDIF()
   ENDFOREACH()
ENDIF()

FIND_PACKAGE_HANDLE_STANDARD_ARGS(Peary REQUIRED_VARS Peary_INCLUDE_DIRS Peary_LIBRARIES HANDLE_COMPONENTS FAIL_MESSAGE "Could not find Peary or a required component, make sure all necessary components are compiled and that the variable PEARYPATH points to the installation location:\n$ export PEARYPATH=/your/path/to/Peary\n")
