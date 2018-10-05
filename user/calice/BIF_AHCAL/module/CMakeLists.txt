include_directories(include)
aux_source_directory(src MODULE_SRC)
include_directories(${CACTUS_INCLUDE_DIR})

add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})

target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY} ${CACTUS_LIBRARIES})

#set_property(TARGET ${EUDAQ_MODULE} PROPERTY CXX_STANDARD 11)
#target_compile_features(${EUDAQ_MODULE} PRIVATE cxx_range_for)
#set_target_properties(${EUDAQ_MODULE} PROPERTIES CXX_STANDARD 11)

set_target_properties(${EUDAQ_MODULE} PROPERTIES INSTALL_RPATH
  ${EUDAQ_INSTALL_RPATH}:${CACTUS_LIBRARY_DIR}
  CXX_STANDARD 11)


install(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
