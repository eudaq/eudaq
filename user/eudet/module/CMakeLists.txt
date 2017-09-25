include_directories(include)
aux_source_directory(src MODULE_SRC)

if(NOT USER_BUILD_NI)
  list(REMOVE_ITEM MODULE_SRC src/NiProducer.cc)
endif()

if(NOT USER_BUILD_TLU)
  list(REMOVE_ITEM MODULE_SRC src/TluProducer.cc)
endif()

if(NOT USER_BUILD_FMCTLU)
  list(REMOVE_ITEM MODULE_SRC src/MinitluProducer.cc)
  list(REMOVE_ITEM MODULE_SRC src/FmctluProducer.cc)
endif()

if(NOT EUDAQ_LCIO_LIBRARY)
  list(REMOVE_ITEM MODULE_SRC src/TluRawEvent2LCEventCuseonverter.cc)
  list(REMOVE_ITEM MODULE_SRC src/NiRawEvent2LCEventConverter.cc)
  list(REMOVE_ITEM MODULE_SRC src/UsbpixrefRawEvent2LCEventConverter.cc)
endif()

if(WIN32)
  list(REMOVE_ITEM MODULE_SRC src/UsbpixrefRawEvent2StdEventConverter.cc)
  list(REMOVE_ITEM MODULE_SRC src/UsbpixrefRawEvent2LCEventConverter.cc)
endif()

add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY}
  ${EUDAQ_LCIO_LIBRARY} ${LCIO_LIBRARIES} ${USER_HARDWARE_LIBRARY})

if(USER_BUILD_FMCTLU)
  set_target_properties(${EUDAQ_MODULE} PROPERTIES INSTALL_RPATH
    ${EUDAQ_INSTALL_RPATH}:${CACTUS_LIBRARY_DIR})
endif()

install(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
