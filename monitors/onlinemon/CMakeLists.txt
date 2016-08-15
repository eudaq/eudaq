list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
find_package(ROOT)

cmake_dependent_option(BUILD_onlinemon "build online monitor executable" ON
  "ROOT_FOUND" OFF)

IF(NOT BUILD_onlinemon)
  message("Online monitor is not to be built")
  return()
ENDIF()

find_package(ROOT REQUIRED COMPONENTS Gui)
include(${ROOT_USE_FILE})
include_directories(. include)

IF(${ROOT_VERSION_MAJOR} GREATER 5)
  add_definitions(-DEUDAQ_LIB_ROOT6)
ENDIF()

set(DICT_INC
  include/OnlineMonWindow.hh
  include/SimpleStandardHit.hh
  include/SimpleStandardCluster.hh
  include/SimpleStandardPlane.hh
  include/OnlineMonConfiguration.hh
  include/CheckEOF.hh)

set(ONLINEMON OnlineMon)
set(ONLINEMON_DICT_CXX ${CMAKE_CURRENT_BINARY_DIR}/OnlineMon_ROOT.cxx)
set(ONLINEMON_DICT_PCM ${CMAKE_CURRENT_BINARY_DIR}/libOnlineMon_ROOT_rdict.pcm)
set(ONLINEMON_DICT_MAP ${CMAKE_CURRENT_BINARY_DIR}/libOnlineMon_ROOT.rootmap)
root_generate_dictionary(OnlineMon_ROOT ${DICT_INC})

aux_source_directory(src ONLINEMON_SRC)
add_executable(${ONLINEMON} ${ONLINEMON_SRC} ${ONLINEMON_DICT_CXX})
target_link_libraries(${ONLINEMON} ${EUDAQ_CORE} ${ROOT_LIBRARIES})

install(TARGETS ${ONLINEMON}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

IF(${ROOT_VERSION_MAJOR} GREATER 5)
  install(FILES
    ${ONLINEMON_DICT_PCM}
    ${ONLINEMON_DICT_MAP}
    DESTINATION bin)
ENDIF()
