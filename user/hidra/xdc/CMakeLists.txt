include_directories(include /usr/include)
link_directories(/usr/lib)
aux_source_directory(src XDC_SRC)

set(CAEN_COMM_LIBRARY CAENComm)
set(CAEN_VME_LIBRARY CAENVME)

# if(NOT EUDAQ_TTREE_LIBRARY)
#   list(REMOVE_ITEM MODULE_SRC src/QTPProducerEvent2TTreeEventConverter.cc)
#   list(REMOVE_ITEM MODULE_SRC src/DRSProducerEvent2TTreeEventConverter.cc)
#   list(REMOVE_ITEM MODULE_SRC src/FERSProducerEvent2TTreeEventConverter.cc)
# endif()

add_library(${EUDAQ_MODULE}_xdc SHARED ${XDC_SRC})
target_link_libraries(${EUDAQ_MODULE}_xdc ${EUDAQ_CORE_LIBRARY} ${EUDAQ_TTREE_LIBRARY} ${EUDAQ_ROOT_MONITOR_LIBRARY} ${CAEN_COMM_LIBRARY} ${CAEN_VME_LIBRARY})

install(TARGETS
  ${EUDAQ_MODULE}_xdc
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

