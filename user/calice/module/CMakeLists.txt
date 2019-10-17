include_directories(${EUDAQ_INCLUDE_DIRS})
include_directories(include)
aux_source_directory(src MODULE_SRC)

if(NOT EUDAQ_LCIO_LIBRARY)
  list(REMOVE_ITEM MODULE_SRC src/AHCalRawEvent2LCEventConverter.cc)
endif()

if(NOT EUDAQ_TTREE_LIBRARY)
    list(REMOVE_ITEM MODULE_SRC src/AHCALDesyRawEvent2TTreeEventConverter.cc)
    list(REMOVE_ITEM MODULE_SRC src/AHCALRawEvent2TTreeEventConverter.cc)
endif()

add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
target_link_libraries(${EUDAQ_MODULE}
  ${EUDAQ_CORE_LIBRARY} ${EUDAQ_LCIO_LIBRARY} ${LCIO_LIBRARIES})

# Add to modules library:
tagret_link_libraries(modules INTERFACE ${EUDAQ_MODULE})

install(TARGETS
  ${EUDAQ_MODULE}
  EXPORT eudaqTargets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
