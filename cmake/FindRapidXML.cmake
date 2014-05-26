# - Try to find RapidXML source package (XML parser)
# Once done this will define
#  RAPIDXML_FOUND - System has RapidXML
#  RAPIDXML_INCLUDE_DIRS - The RapidXML include directories

macro(find_rapidxml_in_extern)
# disable a warning about changed behaviour when traversing directories recursively (wrt symlinks)
IF(COMMAND cmake_policy)
  CMAKE_POLICY(SET CMP0009 NEW)
  CMAKE_POLICY(SET CMP0011 NEW) # disabling a warning about policy changing in this scope
ENDIF(COMMAND cmake_policy)
# determine path to rapidxml package in ./extern folder
file(GLOB_RECURSE extern_file ${PROJECT_SOURCE_DIR}/extern/rapidxml*/rapidxml.hpp)
if (extern_file)
  GET_FILENAME_COMPONENT(RAPIDXML_INCLUDE_DIRS ${extern_file} PATH)
  MESSAGE(STATUS "Found RapidXML package in 'extern' subfolder: ${extern_lib_path}")
endif(extern_file)
endmacro()

find_rapidxml_in_extern()


include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set RapidXML_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(RAPIDXML  DEFAULT_MSG RAPIDXML_INCLUDE_DIRS)
