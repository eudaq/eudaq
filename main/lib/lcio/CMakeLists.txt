get_filename_component(EUDAQ_COMPONENT ${CMAKE_CURRENT_LIST_DIR} NAME)
add_definitions("-DEUDAQ_CORE_EXPORTS")
include_directories(../core/include)
include_directories(../core/include/eudaq)
include_directories(include)

set(EUDAQ_LCIO_LIBRARY ${EUDAQ_COMPONENT})

set(LCIO_DIR ${PROJECT_SOURCE_DIR}/extern/lcio)
find_package(LCIO REQUIRED)
include_directories(${LCIO_INCLUDE_DIRS})

aux_source_directory(src EUDAQ_COMPONENT_SRC)
add_library(${EUDAQ_COMPONENT} SHARED ${EUDAQ_COMPONENT_SRC})
target_link_libraries(${EUDAQ_COMPONENT} ${EUDAQ_CORE_LIBRARY} ${LCIO_LIBRARIES})

set_target_properties(${EUDAQ_COMPONENT} PROPERTIES INSTALL_RPATH
  ${CMAKE_INTSTALL_PREFIX}/lib:${LCIO_LIBRARY_DIRS})


install(TARGETS ${EUDAQ_COMPONENT}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
