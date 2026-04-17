ITEM MODULE_SRC src/FERSProducerEvent2TTreeEventConverter.cc)
# endif()

add_library(${EUDAQ_MODULE}_rc SHARED ${MODULE_SRC})
target_link_libraries(${EUDAQ_MODULE}_rc ${EUDAQ_CORE_LIBRARY} ${EUDAQ_TTREE_LIBRARY} ${EUDAQ_ROOT_MONITOR_LIBRARY})
