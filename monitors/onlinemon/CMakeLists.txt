list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
find_package(ROOT)

cmake_dependent_option(BUILD_onlinemon "build online monitor executable" ON
  "ROOT_FOUND" OFF)

if(NOT BUILD_onlinemon)
  message("Online monitor is not to be built")
  return()
endif()

if(WIN32)
  find_package(ROOT REQUIRED COMPONENTS libGui libCore libHist libRIO)
else()
  find_package(ROOT REQUIRED COMPONENTS Gui)
endif()

include_directories(${ROOT_INCLUDE_DIR})
if(ROOT_USE_FILE)
  include(${ROOT_USE_FILE})
endif()

include_directories(. include)
include_directories(${CMAKE_BINARY_DIR})

message("${ROOT_LIBRARIES}")

if(ROOT_VERSION_MAJOR GREATER 5)
  add_definitions(-DEUDAQ_LIB_ROOT6)
endif()

set(ONLINEMON OnlineMon)

aux_source_directory(src ONLINEMON_SRC)
add_executable(${ONLINEMON} ${ONLINEMON_SRC})
if(CMAKE_COMPILER_IS_GNUCC AND (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 6.0))
  set_property(TARGET ${ONLINEMON} PROPERTY CXX_STANDARD 14)
endif()
target_link_libraries(${ONLINEMON} ${EUDAQ_CORE} ${ROOT_LIBRARIES})

install(TARGETS ${ONLINEMON}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
