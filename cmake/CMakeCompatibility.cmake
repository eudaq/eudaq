# -*- mode: cmake; -*-
# This file contains macros to ensure compatibility with older CMake versions such as 2.6
# 

# Copy files from source directory to destination directory, substituting any
# variables.  Create destination directory if it does not exist.
# Defined twice: once for CMake > 2.8 and once for 2.6
# CMake < 2.8 does not know about FILE(COPY ..) function
if( ${CMAKE_VERSION} VERSION_GREATER 2.8 )
# define macro as alias to FILE(COPY ..)
macro(copy_files srcDir destDir)
  FILE(COPY ${srcDir} DESTINATION ${destDir})
endmacro(copy_files)
else() # CMake 2.6
# disable a warning about changed behaviour when traversing directories recursively (wrt symlinks)
IF(COMMAND cmake_policy)
  CMAKE_POLICY(SET CMP0009 NEW)
  CMAKE_POLICY(SET CMP0011 NEW) # disabling a warning about policy changing in this scope
ENDIF(COMMAND cmake_policy)
# actual copy macro
macro(copy_files srcDir destDir)
  get_filename_component(targetDir ${srcDir} NAME)
  set (fullDest "${destDir}/${targetDir}")
    make_directory(${fullDest})
    file(GLOB_RECURSE templateFiles RELATIVE ${srcDir} ${srcDir}/*)
    foreach(templateFile ${templateFiles})
      set(srcTemplatePath ${srcDir}/${templateFile})
      if(NOT IS_DIRECTORY ${srcTemplatePath})
        configure_file(
          ${srcTemplatePath}
          ${fullDest}/${templateFile}
          COPYONLY)
      endif(NOT IS_DIRECTORY ${srcTemplatePath})
    endforeach(templateFile)
  endmacro(copy_files)
endif()

# newer versions of CMake set the compiler version variable;
# if not set, check manually
if (CMAKE_CXX_COMPILER_VERSION)
  set(COMPILER_VERSION ${CMAKE_CXX_COMPILER_VERSION}) # use info from CMake
else(CMAKE_CXX_COMPILER_VERSION)
  # check compiler version number manually
  # CHECK CLANG VERSION NUMBER
  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    exec_program(${CMAKE_C_COMPILER} ARGS "-v" OUTPUT_VARIABLE _clang_version_info)
    string(REGEX REPLACE "^.*[ ]([0-9]+)\\.[0-9].*$" "\\1" CLANG_MAJOR "${_clang_version_info}")
    string(REGEX REPLACE "^.*[ ][0-9]+\\.([0-9]).*$" "\\1" CLANG_MINOR "${_clang_version_info}")
    set(COMPILER_VERSION "${CLANG_MAJOR}.${CLANG_MINOR}")
    #CHECK GCC VERSION NUMBER
  elseif(CMAKE_COMPILER_IS_GNUCXX)
    exec_program(${CMAKE_C_COMPILER} ARGS "-dumpversion" OUTPUT_VARIABLE _gcc_version_info)
    string(REGEX REPLACE "^([0-9]+).*$"                   "\\1" GCC_MAJOR ${_gcc_version_info})
    string(REGEX REPLACE "^[0-9]+\\.([0-9]+).*$"          "\\1" GCC_MINOR ${_gcc_version_info})
    string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+).*$" "\\1" GCC_PATCH ${_gcc_version_info})
    if(GCC_PATCH MATCHES "\\.+")
      set(GCC_PATCH "")
    endif()
    if(GCC_MINOR MATCHES "\\.+")
      set(GCC_MINOR "")
    endif()
    if(GCC_MAJOR MATCHES "\\.+")
      set(GCC_MAJOR "")
    endif()
    set(COMPILER_VERSION "${GCC_MAJOR}.${GCC_MINOR}.${GCC_PATCH}")
  elseif(CMAKE_CXX_COMPILER_ID MATCHES Intel)
    exec_program(${CMAKE_CXX_COMPILER} ARGS -V OUTPUT_VARIABLE _intel_version_info)
    string(REGEX REPLACE ".*Version ([0-9]+(\\.[0-9]+)+).*" "\\1" _intel_version "${_intel_version_info}")
    set(COMPILER_VERSION _intel_version)
    # MICROSOFT VISUAL C++
  elseif(MSVC)
    set(COMPILER_VERSION MSVC_VERSION)
  else()
    message(STATUS "Could not identify compiler version")
  endif()
endif(CMAKE_CXX_COMPILER_VERSION)
