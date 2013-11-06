# - Try to find ZestSC1
# Once done this will define
#  ZESTSC1_FOUND - System has ZestSC1
#  ZESTSC1_INCLUDE_DIRS - The ZestSC1 include directories
#  ZESTSC1_LIBRARIES - The libraries needed to use ZestSC1
#  ZESTSC1_DEFINITIONS - Compiler switches required for using ZestSC1

find_path(ZESTSC1_INCLUDE_DIR ZestSC1.h
          HINTS ${PROJECT_SOURCE_DIR}/extern/ZestSC1/Inc )


if (WIN32) 
  find_library(ZESTSC1_LIBRARY NAMES ZestSC1 SetupAPI Ws2_32
    HINTS ${PROJECT_SOURCE_DIR}/extern/ZestSC1/windows/Lib )
elseif (UNIX)
  if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    find_library(ZESTSC1_LIBRARY NAMES ZestSC1
      HINTS ${PROJECT_SOURCE_DIR}/extern/ZestSC1/macosx/Lib )
  else()
    find_library(ZESTSC1_LIBRARY NAMES ZestSC1
      HINTS ${PROJECT_SOURCE_DIR}/extern/ZestSC1/linux/Lib )
  endif()
else()
  MESSAGE( "WARNING: Platform not defined in FindZestSC1.txt -- assuming Unix/Linux (good luck)." )
    find_library(ZESTSC1_LIBRARY NAMES ZestSC1
      HINTS ${PROJECT_SOURCE_DIR}/extern/ZestSC1/linux )
endif()

set(ZESTSC1_LIBRARIES ${ZESTSC1_LIBRARY} )
set(ZESTSC1_INCLUDE_DIRS ${ZESTSC1_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set ZESTSC1_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(ZestSC1  DEFAULT_MSG
                                  ZESTSC1_LIBRARY ZESTSC1_INCLUDE_DIR)

mark_as_advanced(ZESTSC1_INCLUDE_DIR ZESTSC1_LIBRARY )