INCLUDE_DIRECTORIES(include)

LIST(APPEND MODULE_SRC src/Timepix3Event2StdEventConverter.cc)

IF(SPIDR_FOUND AND XercesC_FOUND)
  LIST(APPEND MODULE_SRC src/Timepix3Config.cxx src/Timepix3Producer.cxx)
  SET(ext_libraries "libSpidrTpx3Lib.so" "libxerxes-c.so")
  MESSAGE(STATUS "Building Timepix3 Producer")

  LINK_DIRECTORIES(${SPIDR_LIB_PATH})
  INCLUDE_DIRECTORIES(${SPIDR_INCLUDE_DIR})
ELSE()
  MESSAGE(WARNING "Timepix3 Producer NOT built: SPIDR and/or Xerces-C are missing")
ENDIF()
MESSAGE(STATUS "Timepix3 StandardEvent converter will be built")

ADD_LIBRARY(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
TARGET_LINK_LIBRARIES(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_LCIO_LIBRARY} ${ext_libraries})

INSTALL(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
