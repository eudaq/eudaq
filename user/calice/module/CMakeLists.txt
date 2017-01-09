include_directories(${EUDAQ_INCLUDE_DIRS})
include_directories(include)
aux_source_directory(src MODULE_SRC)

list(REMOVE_ITEM MODULE_SRC src/TLUConverterPlugin.cc)
list(REMOVE_ITEM MODULE_SRC src/NIConverterPlugin.cc)

if(NOT EUDAQ_LCIO_LIBRARY)
  list(REMOVE_ITEM MODULE_SRC src/TLURawEvent2LCEventConverter.cc)
  list(REMOVE_ITEM MODULE_SRC src/NIRawEvent2LCEventConverter.cc)
  list(REMOVE_ITEM MODULE_SRC src/AHCalRawEvent2LCEventConverter.cc)
else()
  add_definitions("-DUSE_LCIO")
endif()

add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
target_link_libraries(${EUDAQ_MODULE}
  ${EUDAQ_CORE_LIBRARY} ${EUDAQ_LCIO_LIBRARY} ${LCIO_LIBRARIES})

set_target_properties(${EUDAQ_MODULE} PROPERTIES INSTALL_RPATH
  ${CMAKE_INSTALL_PREFIX}/lib:${LCIO_LIBRARY_DIRS})

install(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
