include_directories(include)
aux_source_directory(src MODULE_SRC)

if(NOT EUDAQ_LCIO_LIBRARY)
  list(REMOVE_ITEM MODULE_SRC src/TluRawEvent2LCEventConverter.cc)
  list(REMOVE_ITEM MODULE_SRC src/NiRawEvent2LCEventConverter.cc)
  list(REMOVE_ITEM MODULE_SRC src/UsbpixI4RawEvent2LCEventConverter.cc)
  list(REMOVE_ITEM MODULE_SRC src/UsbpixI4BRawEvent2LCEventConverter.cc)
endif()

if(WIN32)
  list(REMOVE_ITEM MODULE_SRC src/UsbpixI4RawEvent2StdEventConverter.cc)
  list(REMOVE_ITEM MODULE_SRC src/UsbpixI4RawEvent2LCEventConverter.cc)
  list(REMOVE_ITEM MODULE_SRC src/UsbpixI4BRawEvent2LCEventConverter.cc)
endif()

add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY}
  ${EUDAQ_LCIO_LIBRARY} ${LCIO_LIBRARIES})

# Add to modules library:
TARGET_LINK_LIBRARIES(modules INTERFACE ${EUDAQ_MODULE})

install(TARGETS
  ${EUDAQ_MODULE}
  EXPORT eudaqTargets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
