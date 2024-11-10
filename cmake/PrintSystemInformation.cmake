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

# uses COMPILER_VERSION set in CMakeCompatibility.cmake file
MESSAGE( STATUS "Using ${CMAKE_CXX_COMPILER_ID} (${CMAKE_CXX_COMPILER}) version ${COMPILER_VERSION}")

message(STATUS "Compiler Flags: ${CMAKE_CXX_FLAGS} ${ALL_CXX_FLAGS_${CMAKE_BUILD_TYPE}}")
get_directory_property( DirDefs DIRECTORY ${CMAKE_SOURCE_DIR} COMPILE_DEFINITIONS )
message( STATUS "With COMPILE_DEFINITIONS = ${DirDefs}" )