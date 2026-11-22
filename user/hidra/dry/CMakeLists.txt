include_directories(include)
aux_source_directory(src DRY_SRC)

set(CAEN_COMM_LIBRARY CAENComm)
set(CAEN_VME_LIBRARY CAENVME)

# if(NOT EUDAQ_TTREE_LIBRARY)
#   list(REMOVE_ITEM MODULE_SRC src/QTPProducerEvent2TTreeEventConverter.cc)
#   list(REMOVE_ITEM MODULE_SRC src/DRSProducerEvent2TTreeEventConverter.cc)
#   list(REMOVE_ITEM MODULE_SRC src/FERSProducerEvent2TTreeEventConverter.cc)
# endif()

add_library(${EUDAQ_MODULE}_dry SHARED ${DRY_SRC})
target_link_libraries(${EUDAQ_MODULE}_dry ${EUDAQ_CORE_LIBRARY} ${EUDAQ_TTREE_LIBRARY} ${EUDAQ_ROOT_MONITOR_LIBRARY} ${EUDAQ_MODULE}_common)

install(TARGETS
  ${EUDAQ_MODULE}_dry
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

