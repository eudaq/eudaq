cmake_dependent_option(USER_ITKSTRIP_MODULE "build ITK strip module" ON
  "USER_ITKSTRIP" OFF)

IF(NOT USER_ITKSTRIP_MODULE)
  return()
ENDIF()

include_directories(include)
aux_source_directory(src MODULE_SRC)
add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
target_link_libraries(${EUDAQ_MODULE} EUDAQ)

install(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)


