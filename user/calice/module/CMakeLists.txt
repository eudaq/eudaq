cmake_dependent_option(USER_CALICE_MODULE "build calice module" ON
  "USER_CALICE" OFF)

if(NOT WIN32)
  set(LCIO_DIR ${PROJECT_SOURCE_DIR}/extern/lcio)
  find_package(LCIO REQUIRED)
  add_definitions("-DUSE_LCIO")
  include_directories(${LCIO_INCLUDE_DIRS})
endif()

include_directories(${PROJECT_SOURCE_DIR}/main/lib/lcio/include)
include_directories(include)
aux_source_directory(src MODULE_SRC)

list(REMOVE_ITEM MODULE_SRC src/TLUConverterPlugin.cc)
list(REMOVE_ITEM MODULE_SRC src/NIConverterPlugin.cc)

add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
target_link_libraries(${EUDAQ_MODULE} ${PROJECT_NAME} ${LCIO_LIBRARIES})

install(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
