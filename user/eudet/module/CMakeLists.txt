include_directories(include)
aux_source_directory(src MODULE_SRC)
list(REMOVE_ITEM MODULE_SRC src/TelProducerPS.cc)
list(REMOVE_ITEM MODULE_SRC src/TluProducerPS.cc)
# list(REMOVE_ITEM MODULE_SRC src/MinitluProducer.cc)
  
if(NOT USER_BUILD_NI)
  list(REMOVE_ITEM MODULE_SRC src/NiProducer.cc)
endif()

if(NOT USER_BUILD_TLU)
  list(REMOVE_ITEM MODULE_SRC src/TluProducer.cc)
endif()

if(NOT USER_BUILD_MINITLU)
  list(REMOVE_ITEM MODULE_SRC src/MinitluProducer.cc)
endif()

add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY} ${USER_HARDWARE_LIBRARY})

if(USER_BUILD_MINITLU)
  set_target_properties(${EUDAQ_MODULE} PROPERTIES INSTALL_RPATH
    ${EUDAQ_INSTALL_RPATH}:${CACTUS_LIBRARY_DIR})
endif()

install(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
