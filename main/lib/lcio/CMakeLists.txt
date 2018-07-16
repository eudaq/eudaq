if(WIN32)
  message(STATUS "LCIO is no supported in Windows, disable building of eudaq lcio library")
  return()
endif()

get_filename_component(EUDAQ_COMPONENT ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_LCIO_LIBRARY ${EUDAQ_COMPONENT} CACHE INTERNAL "eudaq lcio library")


find_package(LCIO CONFIG)
find_package(Git QUIET)
if(GIT_FOUND OR LCIO_FOUND)
  set(GIT_OR_FOUND TRUE)
endif()

cmake_dependent_option(EUDAQ_LIBRARY_BUILD_LCIO "build eudaq lib/lcio library" ON
  "GIT_OR_FOUND" OFF)

if(NOT EUDAQ_LIBRARY_BUILD_LCIO)
    message(STATUS "disable building of eudaq lib/lcio library (EUDAQ_LIBRARY_BUILD_LCIO=OFF)")
  unset(EUDAQ_LCIO_LIBRARY CACHE)
  unset(LCIO_LIBRARIES CACHE)
  return()
endif()

add_definitions("-DEUDAQ_CORE_EXPORTS")
aux_source_directory(src EUDAQ_COMPONENT_SRC)
if(LCIO_FOUND)
  message(STATUS "Check for LCIOConfig.cmake -- ok")
  set(LCIO_LIBRARIES ${LCIO_LIBRARIES} CACHE INTERNAL "external lcio library")
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
  message(STATUS "Set to compile the build-in LCIO (${CMAKE_CURRENT_LIST_DIR}/extern/LCIO-02-07.zip)")
  include(ExternalProject)
  externalproject_add(
    extern_lcio
    URL ${CMAKE_CURRENT_LIST_DIR}/extern/LCIO-02-07.zip
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/extern/LCIO-02-07
    PATCH_COMMAND patch -p1 < ${CMAKE_CURRENT_LIST_DIR}/extern/LCIO-02-07-rpath.patch
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${EUDAQ_INSTALL_PREFIX}/extern -DBUILD_TESTING:BOOL=OFF -DBUILD_LCIO_EXAMPLES:BOOL=OFF -DCMAKE_INSTALL_RPATH:PATH=${EUDAQ_INSTALL_RPATH} -DCMAKE_INSTALL_RPATH_USE_LINK_PATH:BOOL=ON -DCMAKE_MACOSX_RPATH=ON
    )
  
  if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(LCIO_LIBRARIES
      ${EUDAQ_INSTALL_PREFIX}/extern/lib/liblcio.dylib
      ${EUDAQ_INSTALL_PREFIX}/extern/lib/libsio.dylib
      CACHE INTERNAL "external lcio library")
  else()
    set(LCIO_LIBRARIES
      ${EUDAQ_INSTALL_PREFIX}/extern/lib/liblcio.so
      ${EUDAQ_INSTALL_PREFIX}/extern/lib/libsio.so
      CACHE INTERNAL "external lcio library")
  endif()
  
  set(EUDAQ_INCLUDE_DIRS
    ${EUDAQ_INCLUDE_DIRS}
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${EUDAQ_INSTALL_PREFIX}/extern/include
    CACHE INTERNAL "directory of eudaq include")

  include_directories(${EUDAQ_INCLUDE_DIRS})
  add_library(${EUDAQ_COMPONENT} SHARED ${EUDAQ_COMPONENT_SRC})
  add_dependencies(${EUDAQ_COMPONENT} extern_lcio)
endif()

target_link_libraries(${EUDAQ_COMPONENT} ${EUDAQ_CORE_LIBRARY} ${LCIO_LIBRARIES})

install(TARGETS ${EUDAQ_COMPONENT}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

file(GLOB INC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/eudaq/*.hh")
install(FILES ${INC_FILES} DESTINATION include/eudaq)
