if(WIN32)
  message(STATUS "LCIO is no supported in Windows, disable building of eudaq lcio library")
  return()
endif()

get_filename_component(EUDAQ_COMPONENT ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_LCIO_LIBRARY ${EUDAQ_COMPONENT} CACHE INTERNAL "eudaq lcio library")

option(EUDAQ_LIBRARY_BUILD_LCIO "build eudaq lib/lcio library" ON)

if(NOT EUDAQ_LIBRARY_BUILD_LCIO)
  message("disable building of eudaq lib/lcio library (EUDAQ_LIBRARY_BUILD_LCIO=OFF)")
  unset(EUDAQ_LCIO_LIBRARY CACHE)
  unset(LCIO_LIBRARIES CACHE)
  unset(LCIO_LIBRARY_DIRS CACHE)
  return()
endif()

file(GLOB_RECURSE LCIO_CMAKE_CONFIG_FILE_FOUND
  ${PROJECT_SOURCE_DIR}/extern/*/LCIOConfig.cmake)

add_definitions("-DEUDAQ_CORE_EXPORTS")
aux_source_directory(src EUDAQ_COMPONENT_SRC)
if(LCIO_CMAKE_CONFIG_FILE_FOUND)
  message(STATUS "Check for LCIOConfig.cmake : ${LCIO_CMAKE_CONFIG_FILE_FOUND} -- ok")
  message(STATUS "Check if compiled binary external lcio exists ...")
  list(GET LCIO_CMAKE_CONFIG_FILE_FOUND 0 LCIO_CMAKE_CONFIG_FILE)
  get_filename_component(LCIO_CMAKE_CONFIG_FILE ${LCIO_CMAKE_CONFIG_FILE} ABSOLUTE)
  get_filename_component(LCIO_CMAKE_CONFIG_DIR ${LCIO_CMAKE_CONFIG_FILE} DIRECTORY)
  set(LCIO_DIR ${LCIO_CMAKE_CONFIG_DIR} CACHE INTERNAL "directory of LCIOConfig.cmake")
  find_package(LCIO REQUIRED)
  set(LCIO_LIBRARIES ${LCIO_LIBRARIES} CACHE INTERNAL "external lcio library")
  set(LCIO_LIBRARY_DIRS ${LCIO_LIBRARY_DIRS} CACHE INTERNAL "directory of external lcio library")

  set(EUDAQ_INCLUDE_DIRS
    ${EUDAQ_INCLUDE_DIRS}
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${LCIO_INCLUDE_DIRS}
    CACHE INTERNAL "directory of eudaq include")

  include_directories(${EUDAQ_INCLUDE_DIRS})
  add_library(${EUDAQ_COMPONENT} SHARED ${EUDAQ_COMPONENT_SRC})
  unset(LCIO_INCLUDE_DIRS CACHE)
  unset(LCIO_LCIO_LIBRARY CACHE)
  unset(LCIO_SIO_LIBRARY CACHE)
else()
  message(STATUS "Check for LCIOConfig.cmake : -- fail")
  set(EUDAQ_LCIO_EX_SVN_REPOSITORY svn://svn.freehep.org/lcio/tags/v02-07)
  set(EUDAQ_LCIO_EX_SOURCE_DIR ${PROJECT_SOURCE_DIR}/extern/lcio-v02-07)
  message("Set to download source files of lcio from: ${EUDAQ_LCIO_EX_SVN_REPOSITORY}")
  message("Set to download/compile/install lcio to: ${EUDAQ_LCIO_EX_SOURCE_DIR}")
  include(ExternalProject)
  ExternalProject_Add(
    extern_lcio
    UPDATE_DISCONNECTED TRUE
    SOURCE_DIR  ${EUDAQ_LCIO_EX_SOURCE_DIR}
    SVN_REPOSITORY ${EUDAQ_LCIO_EX_SVN_REPOSITORY}
    )

  if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(LCIO_LIBRARIES
      ${EUDAQ_LCIO_EX_SOURCE_DIR}/lib/liblcio.dylib
      ${EUDAQ_LCIO_EX_SOURCE_DIR}/lib/libsio.dylib
      CACHE INTERNAL "external lcio library")
  else()
    set(LCIO_LIBRARIES
      ${EUDAQ_LCIO_EX_SOURCE_DIR}/lib/liblcio.so
      ${EUDAQ_LCIO_EX_SOURCE_DIR}/lib/libsio.so
      CACHE INTERNAL "external lcio library")
  endif()
  
  set(LCIO_LIBRARY_DIRS ${EUDAQ_LCIO_EX_SOURCE_DIR}/lib
    CACHE INTERNAL "directory of external lcio library")

  set(EUDAQ_INCLUDE_DIRS
    ${EUDAQ_INCLUDE_DIRS}
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${EUDAQ_LCIO_EX_SOURCE_DIR}/include
    CACHE INTERNAL "directory of eudaq include")

  include_directories(${EUDAQ_INCLUDE_DIRS})
  add_library(${EUDAQ_COMPONENT} SHARED ${EUDAQ_COMPONENT_SRC})
  add_dependencies(${EUDAQ_COMPONENT} extern_lcio)
endif()

target_link_libraries(${EUDAQ_COMPONENT} ${EUDAQ_CORE_LIBRARY} ${LCIO_LIBRARIES})
set_target_properties(${EUDAQ_COMPONENT} PROPERTIES INSTALL_RPATH
  ${EUDAQ_INSTALL_RPATH}:${LCIO_LIBRARY_DIRS})

install(TARGETS ${EUDAQ_COMPONENT}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
