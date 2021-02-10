# - Try to find ZestSC1 driver package needed for accessing the TLU
# Once done this will define
#  ZESTSC1_FOUND - System has ZestSC1
#  ZESTSC1_INCLUDE_DIRS - The ZestSC1 include directories
#  ZESTSC1_LIBRARIES - The libraries needed to use ZestSC1
#  ZESTSC1_DEFINITIONS - Compiler switches required for using ZestSC1

###
macro(find_zestsc1_in_extern arg)
# disable a warning about changed behaviour when traversing directories recursively (wrt symlinks)
IF(COMMAND cmake_policy)
  CMAKE_POLICY(SET CMP0009 NEW)
  CMAKE_POLICY(SET CMP0011 NEW) # disabling a warning about policy changing in this scope
ENDIF(COMMAND cmake_policy)

# determine path to zestsc1 package in ./extern folder
file(GLOB_RECURSE extern_file ${CMAKE_CURRENT_LIST_DIR}/../extern/*ZestSC1.h)
if (extern_file)
  # should have found multiple files of that name, take root of folder (no 'windows*7/inc' string)
  FOREACH (this_file ${extern_file})
  	  IF( (NOT "${this_file}" MATCHES ".*windows.?7/Inc/") AND (NOT "${this_file}" MATCHES ".*windows/Inc/") AND (NOT "${this_file}" MATCHES ".*macosx/Inc/") AND (NOT "${this_file}" MATCHES ".*linux/Inc/"))
	        SET(zest_inc_path "${this_file}")
	  ENDIF()
  ENDFOREACH(this_file)
  IF (zest_inc_path)
    # strip the file and 'Inc' path away:
    get_filename_component(extern_lib_path "${zest_inc_path}" PATH)
    get_filename_component(extern_lib_path "${extern_lib_path}" PATH)
    MESSAGE(STATUS "Found ZestSC1 package in 'extern' subfolder: ${extern_lib_path}")
  ENDIF()
endif(extern_file)

IF(WIN32)
 find_path(ZESTSC1_INCLUDE_DIR ZestSC1.h
    HINTS "${extern_lib_path}/windows_7/Inc"
          "${extern_lib_path}/windows 7/Inc"
	  "${extern_lib_path}/Inc"
    ${arg}
	  )
ELSE(WIN32)
  find_path(ZESTSC1_INCLUDE_DIR ZestSC1.h
    HINTS "${extern_lib_path}/Inc" ${arg})
ENDIF(WIN32)

if (WIN32)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    find_library(ZESTSC1_LIBRARY NAMES ZestSC1
      HINTS "${extern_lib_path}/Lib/amd64"
            "${extern_lib_path}/windows_7/Lib/amd64"
      	    "${extern_lib_path}/windows 7/Lib/amd64"
      ${arg}
      )
  else() #32bit
    find_library(ZESTSC1_LIBRARY NAMES ZestSC1
      HINTS "${extern_lib_path}/Lib/x86"
            "${extern_lib_path}/windows_7/Lib/x86"
      	    "${extern_lib_path}/windows 7/Lib/x86"
      ${arg})
  endif()
elseif (UNIX)
    #MESSAGE(STATUS "UNIX OS found. extern_lib_path = ${extern_lib_path}" )

    if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
      find_library(ZESTSC1_LIBRARY NAMES ZestSC1
	HINTS "${extern_lib_path}/macosx/Lib" ${arg})
    else()
      find_library(ZESTSC1_LIBRARY NAMES ZestSC1
	HINTS "${extern_lib_path}/Lib"
	      "${extern_lib_path}/linux/Lib"
        ${arg})
    endif()
else()
  MESSAGE( "WARNING: Platform not defined in FindZestSC1.txt -- assuming Unix/Linux (good luck)." )
  find_library(ZESTSC1_LIBRARY NAMES ZestSC1
      HINTS "${extern_lib_path}/linux" ${arg})
endif()
endmacro()
###

find_zestsc1_in_extern("")

# could not find the package at the usual locations -- try to copy from AFS if accessible
if (NOT ZESTSC1_LIBRARY)
  IF (EXISTS "/afs/desy.de/group/telescopes/tlu/ZestSC1")
    MESSAGE(STATUS "Could not find ZestSC1 driver package required by tlu producer; downloading it now via AFS to ${CMAKE_CURRENT_LIST_DIR}/../extern  ....")
    if(DEFINED ENV{TRAVIS})
    MESSAGE(STATUS "Running on travis and therefore downloading only the absolutely necessary part of the ZestSC1 driver.")
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../extern/ZestSC1)
    copy_files("/afs/desy.de/group/telescopes/tlu/ZestSC1/Inc" ${CMAKE_CURRENT_LIST_DIR}/../extern/ZestSC1)
    if("$ENV{TRAVIS_OS_NAME}" STREQUAL "linux")
	copy_files("/afs/desy.de/group/telescopes/tlu/ZestSC1/linux" ${CMAKE_CURRENT_LIST_DIR}/../extern/ZestSC1)
    else()
	copy_files("/afs/desy.de/group/telescopes/tlu/ZestSC1/macosx" ${CMAKE_CURRENT_LIST_DIR}/../extern/ZestSC1)
    endif()
    else()
    copy_files("/afs/desy.de/group/telescopes/tlu/ZestSC1" ${CMAKE_CURRENT_LIST_DIR}/../extern)
    endif()
    find_zestsc1_in_extern(NO_DEFAULT_PATH)
  ELSE()
    MESSAGE(STATUS "Obtaining ZestSC1 driver package from git")
    execute_process(COMMAND git clone git@github.com:eudaq/tlu-dependencies.git extern WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/..)
    find_zestsc1_in_extern(NO_DEFAULT_PATH)    
  ENDIF()
endif()

set(ZESTSC1_LIBRARIES ${ZESTSC1_LIBRARY})
set(ZESTSC1_INCLUDE_DIRS ${ZESTSC1_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set ZESTSC1_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(ZestSC1  DEFAULT_MSG
                                  ZESTSC1_LIBRARY ZESTSC1_INCLUDE_DIR)

mark_as_advanced(ZESTSC1_INCLUDE_DIR ZESTSC1_LIBRARY )
