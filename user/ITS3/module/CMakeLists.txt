include_directories(include)
aux_source_directory(src MODULE_SRC)

find_package(ROOT)

if(NOT USER_ITS3_BUILD)
  list(REMOVE_ITEM MODULE_SRC src/*.cc)
endif()

if(NOT ROOT_FOUND)
  list(REMOVE_ITEM MODULE_SRC src/CE65RawEvent2StdEventConverter.cc)
  list(REMOVE_ITEM MODULE_SRC src/OPAMPRawEvent2StdEventConverter.cc)
endif()

add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
target_link_libraries(${EUDAQ_MODULE} PUBLIC ${EUDAQ_CORE_LIBRARY})

IF(ROOT_FOUND)
  target_link_libraries(${EUDAQ_MODULE} PUBLIC ${ROOT_LIBRARIES})
ENDIF()

install(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
