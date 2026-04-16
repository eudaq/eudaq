include_directories(include /usr/include)
include_directories(include /usr/include/ni-visa)
link_directories(/usr/lib)
link_directories(/usr/lib/x86_64-linux-gnu)
aux_source_directory(src MODULE_SRC)

if(NOT EUDAQ_TTREE_LIBRARY)
  list(REMOVE_ITEM MODULE_SRC src/QTPProducerEvent2TTreeEventConverter.cc)
  list(REMOVE_ITEM MODULE_SRC src/DRSProducerEvent2TTreeEventConverter.cc)
  list(REMOVE_ITEM MODULE_SRC src/FERSProducerEvent2TTreeEventConverter.cc)
endif()


add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_TTREE_LIBRARY} ${EUDAQ_ROOT_MONITOR_LIBRARY} ${USER_HARDWARE_LIBRARY} ${CAEN_DIGI_LIBRARY} ${CAEN_COMM_LIBRARY} ${CAEN_VME_LIBRARY} visa)

install(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
