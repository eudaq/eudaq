# print system information
#
# if you are building in-source, this is the same as CMAKE_SOURCE_DIR, otherwise 
# this is the top level directory of your build tree 
MESSAGE( STATUS "CMAKE_BINARY_DIR: " ${CMAKE_BINARY_DIR} )

# contains the full path to the root of your project source directory,
# i.e. to the nearest directory where CMakeLists.txt contains the PROJECT() command 
MESSAGE( STATUS "PROJECT_SOURCE_DIR: " ${PROJECT_SOURCE_DIR} )

if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  exec_program(lsb_release ARGS "--description -s" OUTPUT_VARIABLE _linux_dist)
endif()
 
MESSAGE( STATUS "This is CMake " ${CMAKE_VERSION} " (" ${CMAKE_COMMAND} ") running on " ${CMAKE_SYSTEM_NAME} " (" ${CMAKE_SYSTEM_VERSION} " " ${_linux_dist} ")")

# determine compiler type (32/64bit)
if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
    MESSAGE(STATUS "64-bit compiler detected" )
else( CMAKE_SIZEOF_VOID_P EQUAL 8 ) 
    MESSAGE(STATUS "32-bit compiler detected" )
endif( CMAKE_SIZEOF_VOID_P EQUAL 8 )

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
    set(COMPILER_VERSION "${GCC_MAJOR}${GCC_MINOR}${GCC_PATCH}")
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

MESSAGE( STATUS "Using ${CMAKE_CXX_COMPILER_ID} (${CMAKE_CXX_COMPILER}) version ${COMPILER_VERSION}")

message(STATUS "Compiler Flags: ${CMAKE_CXX_FLAGS} ${ALL_CXX_FLAGS_${CMAKE_BUILD_TYPE}}")
get_directory_property( DirDefs DIRECTORY ${CMAKE_SOURCE_DIR} COMPILE_DEFINITIONS )
message( STATUS "With COMPILE_DEFINITIONS = ${DirDefs}" )