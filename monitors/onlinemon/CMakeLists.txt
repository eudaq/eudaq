list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
find_package(ROOT)

cmake_dependent_option(BUILD_onlinemon "build online monitor executable" ON
  "ROOT_FOUND" OFF)

IF(NOT BUILD_onlinemon)
  message("Online monitor is not to be built")
  return()
ENDIF()

if(WIN32)
  find_package(ROOT REQUIRED COMPONENTS libGui)
else()
  find_package(ROOT REQUIRED COMPONENTS Gui)
endif()
include(${ROOT_USE_FILE})
include_directories(. include)

IF(${ROOT_VERSION_MAJOR} GREATER 5)
  add_definitions(-DEUDAQ_LIB_ROOT6)
ENDIF()

set(ONLINEMON OnlineMon)

aux_source_directory(src ONLINEMON_SRC)
add_executable(${ONLINEMON} ${ONLINEMON_SRC})
target_link_libraries(${ONLINEMON} ${EUDAQ_CORE} ${ROOT_LIBRARIES})

install(TARGETS ${ONLINEMON}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
