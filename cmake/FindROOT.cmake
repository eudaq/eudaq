# - Find ROOT instalation
# This module tries to find the ROOT installation on your system.
# It tries to find the root-config script which gives you all the needed information.
# If the system variable ROOTSYS is set this is straight forward.
# If not the module uses the pathes given in ROOT_CONFIG_SEARCHPATH.
# If you need an other path you should add this path to this varaible.  
# The root-config script is then used to detect basically everything else.
# This module defines a number of key variables and macros.

# F.Uhlig@gsi.de (fairroot.gsi.de)


MESSAGE(STATUS "Looking for Root...")

SET(ROOT_CONFIG_SEARCHPATH
  ${SIMPATH}/tools/root/bin
  $ENV{ROOTSYS}/bin
  /opt/local/bin
  /root/bin
  /usr/bin
)

SET(ROOT_DEFINITIONS "")

SET(ROOT_INSTALLED_VERSION_TOO_OLD FALSE)

SET(ROOT_CONFIG_EXECUTABLE ROOT_CONFIG_EXECUTABLE-NOTFOUND)

FIND_PROGRAM(ROOT_CONFIG_EXECUTABLE NAMES root-config PATHS
   ${ROOT_CONFIG_SEARCHPATH}
   NO_DEFAULT_PATH)
    
 IF (${ROOT_CONFIG_EXECUTABLE} MATCHES "ROOT_CONFIG_EXECUTABLE-NOTFOUND")
   IF (ROOT_FIND_REQUIRED)
     MESSAGE( FATAL_ERROR "ROOT not installed in the searchpath and ROOTSYS is not set. Please
 set ROOTSYS or add the path to your ROOT installation in the Macro FindROOT.cmake in the
 subdirectory cmake/modules.")
   ELSE(ROOT_FIND_REQUIRED)
     MESSAGE( STATUS "Could not find ROOT.")
   ENDIF(ROOT_FIND_REQUIRED)
ELSE (${ROOT_CONFIG_EXECUTABLE} MATCHES "ROOT_CONFIG_EXECUTABLE-NOTFOUND")
  STRING(REGEX REPLACE "(^.*)/bin/root-config" "\\1" test ${ROOT_CONFIG_EXECUTABLE}) 
  SET( ENV{ROOTSYS} ${test})
  set( ROOTSYS ${test})
ENDIF (${ROOT_CONFIG_EXECUTABLE} MATCHES "ROOT_CONFIG_EXECUTABLE-NOTFOUND")  

# root config is a bash script and not commonly executable under Windows
# make some static assumptions instead
IF (WIN32)
  SET(ROOT_FOUND FALSE)
  IF (ROOT_CONFIG_EXECUTABLE)
    SET(ROOT_FOUND TRUE)
    set(ROOT_INCLUDE_DIR ${ROOTSYS}/include)
    set(ROOT_LIBRARY_DIR ${ROOTSYS}/lib)
    SET(ROOT_BINARY_DIR ${ROOTSYS}/bin)
    set(ROOT_LIBRARIES -LIBPATH:${ROOT_LIBRARY_DIR} libGpad.lib libHist.lib libGraf.lib libGraf3d.lib libTree.lib libRint.lib libPostscript.lib libMathCore.lib libRIO.lib libNet.lib libThread.lib libCore.lib libCint.lib libGui.lib libGuiBld.lib)
    FIND_PROGRAM(ROOT_CINT_EXECUTABLE
      NAMES rootcint
      PATHS ${ROOT_BINARY_DIR}
      NO_DEFAULT_PATH
      )
    MESSAGE(STATUS "Found ROOT: $ENV{ROOTSYS}/bin/root (WIN32/version not identified)")
  ENDIF (ROOT_CONFIG_EXECUTABLE)
  
ELSE(WIN32)

  IF (ROOT_CONFIG_EXECUTABLE)
    
    SET(ROOT_FOUND FALSE)

    EXEC_PROGRAM(${ROOT_CONFIG_EXECUTABLE} ARGS "--version" OUTPUT_VARIABLE ROOTVERSION)

    MESSAGE(STATUS "Found ROOT: $ENV{ROOTSYS}/bin/root (found version ${ROOTVERSION})")

    # we need at least version 5.00/00
    IF (NOT ROOT_MIN_VERSION)
      SET(ROOT_MIN_VERSION "5.00/00")
    ENDIF (NOT ROOT_MIN_VERSION)
    
    # now parse the parts of the user given version string into variables
    STRING(REGEX REPLACE "^([0-9]+)\\.[0-9][0-9]+\\/[0-9][0-9]+" "\\1" req_root_major_vers "${ROOT_MIN_VERSION}")
    STRING(REGEX REPLACE "^[0-9]+\\.([0-9][0-9])+\\/[0-9][0-9]+.*" "\\1" req_root_minor_vers "${ROOT_MIN_VERSION}")
    STRING(REGEX REPLACE "^[0-9]+\\.[0-9][0-9]+\\/([0-9][0-9]+)" "\\1" req_root_patch_vers "${ROOT_MIN_VERSION}")
    
    # and now the version string given by qmake
    STRING(REGEX REPLACE "^([0-9]+)\\.[0-9][0-9]+\\/[0-9][0-9]+.*" "\\1" found_root_major_vers "${ROOTVERSION}")
    STRING(REGEX REPLACE "^[0-9]+\\.([0-9][0-9])+\\/[0-9][0-9]+.*" "\\1" found_root_minor_vers "${ROOTVERSION}")
    STRING(REGEX REPLACE "^[0-9]+\\.[0-9][0-9]+\\/([0-9][0-9]+).*" "\\1" found_root_patch_vers "${ROOTVERSION}")

    IF (found_root_major_vers LESS 5)
      MESSAGE( FATAL_ERROR "Invalid ROOT version \"${ROOTVERSION}\", at least major version 4 is required, e.g. \"5.00/00\"")
    ENDIF (found_root_major_vers LESS 5)

    IF (found_root_major_vers EQUAL 6)
      add_definitions(-DEUDAQ_LIB_ROOT6)
    ENDIF (found_root_major_vers EQUAL 6)
    
    # compute an overall version number which can be compared at once
    MATH(EXPR req_vers "${req_root_major_vers}*10000 + ${req_root_minor_vers}*100 + ${req_root_patch_vers}")
    MATH(EXPR found_vers "${found_root_major_vers}*10000 + ${found_root_minor_vers}*100 + ${found_root_patch_vers}")
    
    IF (found_vers LESS req_vers)
      SET(ROOT_FOUND FALSE)
      SET(ROOT_INSTALLED_VERSION_TOO_OLD TRUE)
    ELSE (found_vers LESS req_vers)
      SET(ROOT_FOUND TRUE)
    ENDIF (found_vers LESS req_vers)

  ENDIF (ROOT_CONFIG_EXECUTABLE)


  IF (ROOT_FOUND)
    
    # ask root-config for the library dir
    # Set ROOT_LIBRARY_DIR

    EXEC_PROGRAM( ${ROOT_CONFIG_EXECUTABLE}
      ARGS "--libdir"
      OUTPUT_VARIABLE ROOT_LIBRARY_DIR_TMP )

    IF(EXISTS "${ROOT_LIBRARY_DIR_TMP}")
      SET(ROOT_LIBRARY_DIR ${ROOT_LIBRARY_DIR_TMP} )
    ELSE(EXISTS "${ROOT_LIBRARY_DIR_TMP}")
      MESSAGE("Warning: ROOT_CONFIG_EXECUTABLE reported ${ROOT_LIBRARY_DIR_TMP} as library path,")
      MESSAGE("Warning: but ${ROOT_LIBRARY_DIR_TMP} does NOT exist, ROOT must NOT be installed correctly.")
    ENDIF(EXISTS "${ROOT_LIBRARY_DIR_TMP}")
    
    # ask root-config for the binary dir
    EXEC_PROGRAM(${ROOT_CONFIG_EXECUTABLE}
      ARGS "--bindir"
      OUTPUT_VARIABLE root_bins )
    SET(ROOT_BINARY_DIR ${root_bins})

    # ask root-config for the include dir
    EXEC_PROGRAM( ${ROOT_CONFIG_EXECUTABLE}
      ARGS "--incdir" 
      OUTPUT_VARIABLE root_headers )
    SET(ROOT_INCLUDE_DIR ${root_headers})
    # CACHE INTERNAL "")

    # ask root-config for the library varaibles
    EXEC_PROGRAM( ${ROOT_CONFIG_EXECUTABLE}
      ARGS "--glibs" 
      OUTPUT_VARIABLE root_flags )

    #  STRING(REGEX MATCHALL "([^ ])+"  root_libs_all ${root_flags})
    #  STRING(REGEX MATCHALL "-L([^ ])+"  root_library ${root_flags})
    #  REMOVE_FROM_LIST(root_flags "${root_libs_all}" "${root_library}")

    SET(ROOT_LIBRARIES "${root_flags} -lMinuit")

    # Make variables changeble to the advanced user
    MARK_AS_ADVANCED( ROOT_LIBRARY_DIR ROOT_INCLUDE_DIR ROOT_DEFINITIONS)

    # Set ROOT_INCLUDES
    SET( ROOT_INCLUDES ${ROOT_INCLUDE_DIR})

    SET(LD_LIBRARY_PATH ${LD_LIBRARY_PATH} ${ROOT_LIBRARY_DIR})

    #######################################
    #
    #       Check the executables of ROOT 
    #          ( rootcint ) 
    #
    #######################################

    FIND_PROGRAM(ROOT_CINT_EXECUTABLE
      NAMES rootcint
      PATHS ${ROOT_BINARY_DIR}
      NO_DEFAULT_PATH
      )

  ENDIF (ROOT_FOUND)
ENDIF(WIN32)



  ###########################################
  #
  #       Macros for building ROOT dictionary
  #
  ###########################################

MACRO (ROOT_GENERATE_DICTIONARY_OLD )
 
   set(INFILES "")    

   foreach (_current_FILE ${ARGN})

     IF (${_current_FILE} MATCHES "^.*\\.h$")
       IF (${_current_FILE} MATCHES "^.*Link.*$")
         set(LINKDEF_FILE ${_current_FILE})
       ELSE (${_current_FILE} MATCHES "^.*Link.*$")
         set(INFILES ${INFILES} ${_current_FILE})
       ENDIF (${_current_FILE} MATCHES "^.*Link.*$")
     ELSEIF (${_current_FILE} MATCHES "^.*\\.hh$")
       IF (${_current_FILE} MATCHES "^.*Link.*$")
         set(LINKDEF_FILE ${_current_FILE})
       ELSE (${_current_FILE} MATCHES "^.*Link.*$")
         set(INFILES ${INFILES} ${_current_FILE})
       ENDIF (${_current_FILE} MATCHES "^.*Link.*$")
     ELSE (${_current_FILE} MATCHES "^.*\\.h$")
       IF (${_current_FILE} MATCHES "^.*\\.cxx$")
         set(OUTFILE ${_current_FILE})
       ELSE (${_current_FILE} MATCHES "^.*\\.cxx$")
         set(INCLUDE_DIRS ${INCLUDE_DIRS} -I${_current_FILE})   
       ENDIF (${_current_FILE} MATCHES "^.*\\.cxx$")
     ENDIF (${_current_FILE} MATCHES "^.*\\.h$")
     
   endforeach (_current_FILE ${ARGN})
   
#  MESSAGE("INFILES: ${INFILES}")
#  MESSAGE("OutFILE: ${OUTFILE}")
#  MESSAGE("LINKDEF_FILE: ${LINKDEF_FILE}")
#  MESSAGE("INCLUDE_DIRS: ${INCLUDE_DIRS}")

   STRING(REGEX REPLACE "(^.*).cxx" "\\1.h" bla "${OUTFILE}")
#   MESSAGE("BLA: ${bla}")
   SET (OUTFILES ${OUTFILE} ${bla})

   ADD_CUSTOM_COMMAND(OUTPUT ${OUTFILES}
      COMMAND ${ROOT_CINT_EXECUTABLE}
      ARGS -f ${OUTFILE} -c -p -DHAVE_CONFIG_H ${INCLUDE_DIRS} ${INFILES} ${LINKDEF_FILE} )

#   MESSAGE("ROOT_CINT_EXECUTABLE has created the dictionary ${OUTFILE}")

ENDMACRO (ROOT_GENERATE_DICTIONARY_OLD)


MACRO (GENERATE_ROOT_TEST_SCRIPT SCRIPT_FULL_NAME)

  get_filename_component(path_name ${SCRIPT_FULL_NAME} PATH)
  get_filename_component(file_extension ${SCRIPT_FULL_NAME} EXT)
  get_filename_component(file_name ${SCRIPT_FULL_NAME} NAME_WE)
  set(shell_script_name "${file_name}.sh")

  #MESSAGE("PATH: ${path_name}")
  #MESSAGE("Ext: ${file_extension}")
  #MESSAGE("Name: ${file_name}")
  #MESSAGE("Shell Name: ${shell_script_name}")

  string(REPLACE ${PROJECT_SOURCE_DIR} 
         ${PROJECT_BINARY_DIR} new_path ${path_name}
        )

  #MESSAGE("New PATH: ${new_path}")

  file(MAKE_DIRECTORY ${new_path}/data)

  CONVERT_LIST_TO_STRING(${LD_LIBRARY_PATH})
  set(MY_LD_LIBRARY_PATH ${output})
  set(my_script_name ${SCRIPT_FULL_NAME})

  if(CMAKE_SYSTEM MATCHES Darwin)
    configure_file(${PROJECT_SOURCE_DIR}/cmake/scripts/root_macro_macos.sh.in
                   ${new_path}/${shell_script_name}
                  )
  else(CMAKE_SYSTEM MATCHES Darwin)
    configure_file(${PROJECT_SOURCE_DIR}/cmake/scripts/root_macro.sh.in
                   ${new_path}/${shell_script_name}
                  )
  endif(CMAKE_SYSTEM MATCHES Darwin)

  EXEC_PROGRAM(/bin/chmod ARGS "u+x  ${new_path}/${shell_script_name}")

ENDMACRO (GENERATE_ROOT_TEST_SCRIPT)


set(ROOTCINT_EXECUTABLE ${ROOT_CINT_EXECUTABLE})

#----------------------------------------------------------------------------
# function ROOT_GENERATE_DICTIONARY( dictionary
#                                    header1 header2 ...
#                                    LINKDEF linkdef1 ...
#                                    OPTIONS opt1...)
function(ROOT_GENERATE_DICTIONARY dictionary)
  CMAKE_PARSE_ARGUMENTS(ARG "" "" "LINKDEF;OPTIONS" "" ${ARGN})
  #---Get the list of include directories------------------
  get_directory_property(incdirs INCLUDE_DIRECTORIES)
  set(includedirs)
  foreach( d ${incdirs})
     set(includedirs ${includedirs} -I${d})
  endforeach()
  #---Get the list of header files-------------------------
  set(headerfiles)
  foreach(fp ${ARG_UNPARSED_ARGUMENTS})
    if(${fp} MATCHES "[*?]") # Is this header a globbing expression?
      file(GLOB files ${fp})
      foreach(f ${files})
        if(NOT f MATCHES LinkDef) # skip LinkDefs from globbing result
          set(headerfiles ${headerfiles} ${f})
        endif()
      endforeach()
    else()
      find_file(headerFile ${fp} HINTS ${incdirs})
      set(headerfiles ${headerfiles} ${headerFile})
      unset(headerFile CACHE)
    endif()
  endforeach()
  #---Get LinkDef.h file------------------------------------
  set(linkdefs)
  foreach( f ${ARG_LINKDEF})
    find_file(linkFile ${f} HINTS ${incdirs})
    set(linkdefs ${linkdefs} ${linkFile})
    unset(linkFile CACHE)
  endforeach()
  #---call rootcint------------------------------------------
  add_custom_command(OUTPUT ${dictionary}.cxx
                     COMMAND ${ROOTCINT_EXECUTABLE} -cint -f  ${dictionary}.cxx
                                          -c ${ARG_OPTIONS} ${includedirs} ${headerfiles} ${linkdefs}
                     DEPENDS ${headerfiles} ${linkdefs} VERBATIM)
endfunction()
