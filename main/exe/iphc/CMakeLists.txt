
include_directories(include)

add_executable(IPHCConverter    src/IPHCConverter.cxx)

set_property(TARGET IPHCConverter  PROPERTY PROJECT_LABEL "bin_IPHCConverter") 
target_link_libraries(IPHCConverter  ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB} ${ADDITIONAL_LIBRARIES})

INSTALL(TARGETS IPHCConverter
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
