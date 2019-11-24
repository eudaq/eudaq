include_directories(include)
aux_source_directory(src MODULE_SRC)

if(NOT EUDAQ_LCIO_LIBRARY)
  list(REMOVE_ITEM MODULE_SRC src/NiRawEvent2LCEventConverter.cc)
endif()

if(NOT USER_EUDET_BUILD)
  list(REMOVE_ITEM MODULE_SRC src/NiProducer.cc)
endif()

add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})

target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY}
  ${EUDAQ_LCIO_LIBRARY} ${LCIO_LIBRARIES} ${USER_HARDWARE_LIBRARY})

install(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
