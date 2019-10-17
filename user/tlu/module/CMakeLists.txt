include_directories(include)
aux_source_directory(src MODULE_SRC)

if(NOT EUDAQ_LCIO_LIBRARY)
  list(REMOVE_ITEM MODULE_SRC src/TluRawEvent2LCEventConverter.cc)
endif()

if(NOT USER_TLU_BUILD_EUDET)
  list(REMOVE_ITEM MODULE_SRC src/EudetTluProducer.cc)
endif()

if(NOT USER_TLU_BUILD_AIDA)
  list(REMOVE_ITEM MODULE_SRC src/AidaTluProducer.cc)
endif()

add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY}
  ${EUDAQ_LCIO_LIBRARY} ${LCIO_LIBRARIES} ${USER_HARDWARE_LIBRARY})

# Add to modules library:
TARGET_LINK_LIBRARIES(modules INTERFACE ${EUDAQ_MODULE})

if(USER_TLU_BUILD_AIDA)
  set_target_properties(${EUDAQ_MODULE} PROPERTIES INSTALL_RPATH
    ${EUDAQ_INSTALL_RPATH}:${CACTUS_LIBRARY_DIR})
endif()

install(TARGETS
  ${EUDAQ_MODULE}
  EXPORT eudaqTargets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
