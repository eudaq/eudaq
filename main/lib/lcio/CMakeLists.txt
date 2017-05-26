if(WIN32)
  message(STATUS "LCIO is no supported in Windows, disable building of eudaq lcio library")
  return()
endif()

get_filename_component(EUDAQ_COMPONENT ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_LCIO_LIBRARY ${EUDAQ_COMPONENT} CACHE INTERNAL "eudaq lcio library")


find_package(LCIO)
find_package(Subversion)
if(SUBVERSION_FOUND OR LCIO_FOUND)
  set(SVN_OR_FOUND TRUE)
endif()

cmake_dependent_option(EUDAQ_LIBRARY_BUILD_LCIO "build eudaq lib/lcio library" ON
  "SVN_OR_FOUND" OFF)

if(NOT EUDAQ_LIBRARY_BUILD_LCIO)
  message("disable building of eudaq lib/lcio library (EUDAQ_LIBRARY_BUILD_LCIO=OFF)")
  unset(EUDAQ_LCIO_LIBRARY CACHE)
  unset(LCIO_LIBRARIES CACHE)
  return()
endif()

add_definitions("-DEUDAQ_CORE_EXPORTS")
aux_source_directory(src EUDAQ_COMPONENT_SRC)
if(LCIO_FOUND)
  message(STATUS "Check for LCIOConfig.cmake -- ok")
  find_package(LCIO REQUIRED)
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
  set(EUDAQ_LCIO_EX_GIT_REPOSITORY https://github.com/iLCSoft/LCIO.git)
  set(EUDAQ_LCIO_EX_GIT_TAG v02-07)
  set(EUDAQ_LCIO_EX_SOURCE_DIR ${PROJECT_SOURCE_DIR}/extern/source/lcio-v02-07)
  message("Set to download source files of lcio from: ${EUDAQ_LCIO_EX_SVN_REPOSITORY}, tag "${EUDAQ_LCIO_EX_GIT_TAG})
  message("Set to download/compile/install lcio to: ${EUDAQ_LCIO_EX_SOURCE_DIR}")
  include(ExternalProject)
  if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(LCIO_LINKER_FLAGS_RPATH "-Wl,-rpath,@loader_path/../lib")
  else()
    set(LCIO_LINKER_FLAGS_RPATH "-Wl,-rpath,\$ORIGIN/../lib")
  endif()

  set(PATCH_LICO_CMD "patch -p1 "${CMAKE_CURRENT_LIST_DIR}"/lico_rpath.patch")


  externalproject_add(
    extern_lcio
    UPDATE_DISCONNECTED TRUE
    # SOURCE_DIR ${EUDAQ_LCIO_EX_SOURCE_DIR}
    GIT_REPOSITORY ${EUDAQ_LCIO_EX_GIT_REPOSITORY}
    GIT_TAG ${EUDAQ_LCIO_EX_GIT_TAG}
    # CMAKE_ARGS -DCMAKE_EXE_LINKER_FLAGS:STRING=${LCIO_LINKER_FLAGS_RPATH} -DCMAKE_SHARED_LINKER_FLAGS:STRING=${LCIO_LINKER_FLAGS_RPATH} -DCMAKE_SKIP_INSTALL_RPATH:BOOL=ON -DCMAKE_SKIP_RPATH:BOOL=ON -DCMAKE_INSTALL_PREFIX:PATH=${EUDAQ_INSTALL_PREFIX}/extern -DBUILD_TESTING:BOOL=OFF -DBUILD_LCIO_EXAMPLES:BOOL=OFF
    # UPDATE_COMMAND svn revert --recursive .
    PATCH_COMMAND patch -p1 < ${CMAKE_CURRENT_LIST_DIR}/lico_rpath.patch
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
    CACHE INTERNAL "directory of eudaq include")

  include_directories(${EUDAQ_INCLUDE_DIRS})
  add_library(${EUDAQ_COMPONENT} SHARED ${EUDAQ_COMPONENT_SRC})
  add_dependencies(${EUDAQ_COMPONENT} extern_lcio)
endif()

target_link_libraries(${EUDAQ_COMPONENT} ${EUDAQ_CORE_LIBRARY} ${LCIO_LIBRARIES})

message(xxxxxxxxxxxx..  ${EUDAQ_CORE_LIBRARY} )

install(TARGETS ${EUDAQ_COMPONENT}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
