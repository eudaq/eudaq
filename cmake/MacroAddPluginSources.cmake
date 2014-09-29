##############################################################################
# macro to globally add source files for converter plugins
##############################################################################

# function to collect all the sources from sub-directories
# into a single list
function(add_plugin_sources)
  get_property(is_defined GLOBAL PROPERTY PLUGIN_SRCS_LIST DEFINED)
  if(NOT is_defined)
    define_property(GLOBAL PROPERTY PLUGIN_SRCS_LIST
      BRIEF_DOCS "List of source files"
      FULL_DOCS "List of source files to be compiled in one library")
  endif()
  # make absolute paths
  set(PLUGIN_SRCS)
  foreach(s IN LISTS ARGN)
    if(NOT IS_ABSOLUTE "${s}")
      get_filename_component(s "${s}" ABSOLUTE)
    endif()
    list(APPEND PLUGIN_SRCS "${s}")
  endforeach()
  # append to global list
  set_property(GLOBAL APPEND PROPERTY PLUGIN_SRCS_LIST "${PLUGIN_SRCS}")
endfunction(add_plugin_sources)