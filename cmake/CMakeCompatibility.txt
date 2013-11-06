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
