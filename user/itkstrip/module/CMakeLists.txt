include_directories(include)
aux_source_directory(src MODULE_SRC)

list(REMOVE_ITEM MODULE_SRC src/SCTConverterPlugin.cc)

if(NOT EUDAQ_LCIO_LIBRARY)
  list(REMOVE_ITEM MODULE_SRC src/ItsAbcRawEvent2LCEventConverter.cc)
  list(REMOVE_ITEM MODULE_SRC src/ItsTtcRawEvent2LCEventConverter.cc)
endif()

add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY}
  ${EUDAQ_LCIO_LIBRARY} ${LCIO_LIBRARIES})

install(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
