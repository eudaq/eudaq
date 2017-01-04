cmake_dependent_option(USER_EUDET_MODULE "build telescope module" ON
  "USER_EUDET" OFF)


IF(USER_EUDET_MODULE)
  include_directories(include)
  aux_source_directory(src MODULE_SRC)

  list(REMOVE_ITEM MODULE_SRC src/TelProducerPS.cc)
  list(REMOVE_ITEM MODULE_SRC src/TluProducerPS.cc)
  
  if(NOT USER_EUDET_HARDWARE)
    list(REMOVE_ITEM MODULE_SRC src/TelProducerPS.cc)
    list(REMOVE_ITEM MODULE_SRC src/TluProducerPS.cc)
    list(REMOVE_ITEM MODULE_SRC src/NiProducer.cc)
    unset(USER_HARDWARE)
  endif()
  
  add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
  target_link_libraries(${EUDAQ_MODULE} ${USER_HARDWARE} ${EUDAQ_CORE_LIBRARY})
  
  install(TARGETS
    ${EUDAQ_MODULE}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
ENDIF()
