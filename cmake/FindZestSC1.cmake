# - Try to find ZestSC1 driver package needed for accessing the TLU
# Once done this will define
#  ZESTSC1_FOUND - System has ZestSC1
#  ZESTSC1_INCLUDE_DIRS - The ZestSC1 include directories
#  ZESTSC1_LIBRARIES - The libraries needed to use ZestSC1
#  ZESTSC1_DEFINITIONS - Compiler switches required for using ZestSC1

macro(find_zestsc1_in_extern arg)
IF(WIN32)
 find_path(ZESTSC1_INCLUDE_DIR ZestSC1.h
    HINTS ${PROJECT_SOURCE_DIR}/extern/ZestSC1/windows_7/Inc ${arg})
ELSE(WIN32)
  find_path(ZESTSC1_INCLUDE_DIR ZestSC1.h
    HINTS ${PROJECT_SOURCE_DIR}/extern/ZestSC1/Inc ${arg})
ENDIF(WIN32)

if (WIN32) 
  if (${EX_PLATFORM} EQUAL 64)
    find_library(ZESTSC1_LIBRARY NAMES ZestSC1 SetupAPI Ws2_32
      HINTS ${PROJECT_SOURCE_DIR}/extern/ZestSC1/windows_7/Lib/amd64 ${arg})
  else() #32bit
    find_library(ZESTSC1_LIBRARY NAMES ZestSC1 SetupAPI Ws2_32
      HINTS ${PROJECT_SOURCE_DIR}/extern/ZestSC1/windows_7/Lib/x86 ${arg})
  endif(${EX_PLATFORM} EQUAL 64)
  elseif (UNIX)
    if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
      find_library(ZESTSC1_LIBRARY NAMES ZestSC1
	HINTS ${PROJECT_SOURCE_DIR}/extern/ZestSC1/macosx/Lib ${arg})
    else()
      find_library(ZESTSC1_LIBRARY NAMES ZestSC1
	HINTS ${PROJECT_SOURCE_DIR}/extern/ZestSC1/linux/Lib ${arg})
    endif()
  else()
    MESSAGE( "WARNING: Platform not defined in FindZestSC1.txt -- assuming Unix/Linux (good luck)." )
    find_library(ZESTSC1_LIBRARY NAMES ZestSC1
      HINTS ${PROJECT_SOURCE_DIR}/extern/ZestSC1/linux ${arg})
  endif()
endmacro()

find_zestsc1_in_extern("")

# could not find the package at the usual locations -- try to copy from AFS if accessible
if (NOT ZESTSC1_LIBRARY)
  IF (EXISTS "/afs/desy.de/group/telescopes/tlu/ZestSC1")
    MESSAGE(STATUS "Could not find ZestSC1 driver package required by tlu producer; downloading it now via AFS to ./extern ....")
    copy_files("/afs/desy.de/group/telescopes/tlu/ZestSC1" ${PROJECT_SOURCE_DIR}/extern)
    find_zestsc1_in_extern(NO_DEFAULT_PATH)
  ELSE()
    MESSAGE(WARNING "Could not find ZestSC1 driver package required by tlu producer. Please refer to the documentation on how to obtain the software.")
  ENDIF()
endif()

set(ZESTSC1_LIBRARIES ${ZESTSC1_LIBRARY} )
set(ZESTSC1_INCLUDE_DIRS ${ZESTSC1_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set ZESTSC1_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(ZestSC1  DEFAULT_MSG
                                  ZESTSC1_LIBRARY ZESTSC1_INCLUDE_DIR)

mark_as_advanced(ZESTSC1_INCLUDE_DIR ZESTSC1_LIBRARY )