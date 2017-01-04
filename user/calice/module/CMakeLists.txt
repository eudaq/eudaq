cmake_dependent_option(USER_CALICE_MODULE "build calice module" ON
  "USER_CALICE" OFF)

include_directories(include)
aux_source_directory(src MODULE_SRC)

list(REMOVE_ITEM MODULE_SRC src/TLUConverterPlugin.cc)
list(REMOVE_ITEM MODULE_SRC src/NIConverterPlugin.cc)

if(WIN32)
  list(REMOVE_ITEM MODULE_SRC src/TLURawEvent2LCEventConverter.cc)
  list(REMOVE_ITEM MODULE_SRC src/NIRawEvent2LCEventConverter.cc)
  list(REMOVE_ITEM MODULE_SRC src/AHCalRawEvent2LCEventConverter.cc)
else()
  set(LCIO_DIR ${PROJECT_SOURCE_DIR}/extern/lcio)
  find_package(LCIO REQUIRED)
  add_definitions("-DUSE_LCIO")
  include_directories(${LCIO_INCLUDE_DIRS})
  include_directories(${PROJECT_SOURCE_DIR}/main/lib/lcio/include)  
endif()

add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
target_link_libraries(${EUDAQ_MODULE}
  ${EUDAQ_CORE_LIBRARY} ${EUDAQ_LICO_LIBRARY} ${LCIO_LIBRARIES})

set_target_properties(${EUDAQ_MODULE} PROPERTIES INSTALL_RPATH
  ${CMAKE_INTSTALL_PREFIX}/lib:${LCIO_LIBRARY_DIRS})

install(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
