if(WIN32)
  message(STATUS "LCIO is currently not supported in Windows, disable building of eudaq lcio library")
  return()
endif()

#get_filename_component(EUDAQ_COMPONENT ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_COMPONENT "lcio_interface")
set(EUDAQ_LCIO_LIBRARY ${EUDAQ_COMPONENT} CACHE INTERNAL "eudaq lcio library")

OPTION(EUDAQ_LIBRARY_BUILD_LCIO "lib/lcio library (requires LCIO)" OFF)
if(NOT EUDAQ_LIBRARY_BUILD_LCIO)
  message(STATUS "lib/lcio library is NOT to be built (EUDAQ_LIBRARY_BUILD_LCIO=OFF)")
  unset(EUDAQ_LCIO_LIBRARY CACHE)
  unset(LCIO_LIBRARIES CACHE)
  return()
endif()
message(STATUS "lib/lcio library is to be built (EUDAQ_LIBRARY_BUILD_LCIO=ON)")

set (LCIO_GENERATE_HEADERS ON CACHE INTERNAL "Turn off java for lcio")

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
  message(STATUS "Check for existing LCIOConfig.cmake: fail, so use the build-in")

 include(FetchContent)
 FetchContent_Declare(extern_lcio
 GIT_REPOSITORY https://github.com/iLCSoft/LCIO.git
 GIT_TAG v02-20-02
 GIT_SHALLOW 1
 #FIND_PACKAGE_ARGS
 CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${EUDAQ_INSTALL_PREFIX}/extern -DBUILD_TESTING:BOOL=OFF -DBUILD_LCIO_EXAMPLES:BOOL=OFF -DCMAKE_INSTALL_RPATH:PATH=${EUDAQ_INSTALL_RPATH} -DCMAKE_INSTALL_RPATH_USE_LINK_PATH:BOOL=ON -DCMAKE_MACOSX_RPATH=ON
 )


 FetchContent_MakeAvailable(extern_lcio)
endif()

include_directories(${EUDAQ_INCLUDE_DIRS})
add_library(${EUDAQ_COMPONENT} SHARED ${EUDAQ_COMPONENT_SRC})
#add_dependencies(${EUDAQ_COMPONENT} extern_lcio)
  
target_include_directories(${EUDAQ_COMPONENT}
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  )
  
target_link_libraries(${EUDAQ_COMPONENT} PUBLIC eudaq_core LCIO::lcio SIO::sio)

install(TARGETS ${EUDAQ_COMPONENT}
  EXPORT eudaqTargets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

file(GLOB INC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/eudaq/*.hh")
install(FILES ${INC_FILES} DESTINATION include/eudaq)
