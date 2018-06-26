if(WIN32)
  message(STATUS "TTree Conversion is no supported in Windows, disable building of eudaq ttree library")
    return()
endif()


set(CMAKE_PREFIX_PATH $ENV{ROOTSYS})
set(ROOT_DIR $ENV{ROOTSYS}/cmake)
#set(CMAKE_PREFIX_PATH ${ROOTSYS})
#set(ROOT_DIR ${ROOTSYS}/etc/)
find_package(ROOT)

get_filename_component(EUDAQ_COMPONENT ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_TTREE_LIBRARY ${EUDAQ_COMPONENT} CACHE INTERNAL "eudaq ttree library")


cmake_dependent_option(EUDAQ_LIBRARY_BUILD_TTREE "build eduaq lib/ttree library" ON
  "ROOT_FOUND" OFF)

if(NOT EUDAQ_LIBRARY_BUILD_TTREE)
  message("disable building of eudaq lib/ttree library (EUDAQ_LIBRARY_BUILD_TTREE=OFF)")
  return()
endif()
set (TTREE_LIBRARIES ${TTREE_LIBRARIES} CACHE INTERNAL "external ttree library")
  set(EUDAQ_INCLUDE_DIRS
    ${EUDAQ_INCLUDE_DIRS}
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${ROOT_INCLUDE_DIRS}
    CACHE INTERNAL "directory of eudaq include")
include_directories(${EUDAQ_INCLUDE_DIRS})	
include_directories(${ROOT_INCLUDE_DIR})

if(ROOT_USE_FILE)
  include(${ROOT_USE_FILE})
endif()

include_directories(. include)
include_directories(${CMAKE_BINARY_DIR})
if(ROOT_VERSION_MAJOR GREATER 5)
  add_definitions(-DEUDAQ_LIB_ROOT6)
endif()

aux_source_directory(src EUDAQ_COMPONENT_SRC)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(TTREE_LIBRARIES
      ${EUDAQ_INSTALL_PREFIX}/extern/lib/libttree.dylib
      CACHE INTERNAL "external ttree library")
  else()
    set(LCIO_LIBRARIES
      ${EUDAQ_INSTALL_PREFIX}/extern/lib/liblcio.so
      CACHE INTERNAL "external ttree library")
  endif()


 if((CMAKE_COMPILER_IS_GNUCC) AND (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 6.0))
  set_property(TARGET ${EUDAQ_COMPONENT} PROPERTY CXX_STANDARD 14)
endif()

add_library(${EUDAQ_COMPONENT} SHARED ${EUDAQ_COMPONENT_SRC})

target_link_libraries(${EUDAQ_COMPONENT} ${EUDAQ_CORE_LIBRARY} ${ROOT_LIBRARIES})



install(TARGETS ${EUDAQ_COMPONENT}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

file(GLOB INC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/eudaq/*.hh")
install(FILES ${INC_FILES} DESTINATION include/eudaq)
