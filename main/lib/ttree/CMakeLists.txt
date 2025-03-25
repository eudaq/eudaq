cmake_dependent_option(EUDAQ_LIBRARY_BUILD_TTREE "lib/ttree library (requires ROOT)" OFF
  "ROOT_FOUND" OFF)

if(NOT EUDAQ_LIBRARY_BUILD_TTREE)
    message(STATUS "lib/ttree is NOT to be built (EUDAQ_LIBRARY_BUILD_TTREE=OFF)")
  unset(EUDAQ_TTREE_LIBRARY CACHE)
  unset(TTREE_LIBRARIES CACHE)
  return()
endif()

get_filename_component(EUDAQ_COMPONENT ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_TTREE_LIBRARY ${EUDAQ_COMPONENT} CACHE INTERNAL "eudaq ttree library")

set (TTREE_LIBRARIES ${TTREE_LIBRARIES} CACHE INTERNAL "external ttree library")
  set(EUDAQ_INCLUDE_DIRS
    ${EUDAQ_INCLUDE_DIRS}
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${ROOT_INCLUDE_DIRS}
    CACHE INTERNAL "directory of eudaq include")
include_directories(${EUDAQ_INCLUDE_DIRS})	
include_directories(${ROOT_INCLUDE_DIR})

include_directories(. include)
include_directories(${CMAKE_BINARY_DIR})

aux_source_directory(src EUDAQ_COMPONENT_SRC)

add_library(${EUDAQ_COMPONENT} SHARED ${EUDAQ_COMPONENT_SRC})

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(TTREE_LIBRARIES
      ${EUDAQ_INSTALL_PREFIX}/extern/lib/libttree.dylib
      CACHE INTERNAL "external ttree library")
  else()
    set(TTREE_LIBRARIES
      ${EUDAQ_INSTALL_PREFIX}/extern/lib/libttree.so
      CACHE INTERNAL "external ttree library")
  endif()

target_link_libraries(${EUDAQ_COMPONENT} ${EUDAQ_CORE_LIBRARY} ${ROOT_LIBRARIES})

install(TARGETS ${EUDAQ_COMPONENT}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

file(GLOB INC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/eudaq/*.hh")
install(FILES ${INC_FILES} DESTINATION include/eudaq)
