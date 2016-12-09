cmake_dependent_option(USER_TELESCOPE_MODULE "build telescope module" ON
  "USER_TELESCOPE" OFF)

# option(USER_TELESCOPE_MODULE "build telescope module" ON)

IF(USER_TELESCOPE_MODULE)
  include_directories(include)
  aux_source_directory(src MODULE_SRC)

  if(NOT USER_TELESCOPE_HARDWARE)
    list(REMOVE_ITEM MODULE_SRC src/TelProducerPS.cc)
    list(REMOVE_ITEM MODULE_SRC src/TluProducerPS.cc)
    list(REMOVE_ITEM MODULE_SRC src/NiProducer.cc)
  endif()
  
  add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
  target_link_libraries(${EUDAQ_MODULE} ${USER_HARDWARE} ${PROJECT_NAME})

  install(TARGETS
    ${EUDAQ_MODULE}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
ENDIF()
