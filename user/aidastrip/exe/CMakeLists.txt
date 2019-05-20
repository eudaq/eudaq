if(NOT USER_AIDASTRIP_EXE)
  message(STATUS "Disable building of aux. AIDASTRIP executables user/aidastrip/exe (USER_AIDASTRIP_EXE=OFF)")
  return()
endif()

set(CMAKE_PREFIX_PATH $ENV{ROOTSYS})
set(ROOT_DIR $ENV{ROOTSYS}/cmake)

set(EXE_CLI_LYCORIS_DATACON lycorisCliConverter)
set(EXE_CLI_SYNC_DATATOOL lycorisSyncTool)

## -- Boost for mu3e converter
FIND_PACKAGE( Boost 1.58 COMPONENTS program_options REQUIRED )
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )
if(Boost_FOUND)
  message(STATUS "Boost 1.58 found! Compile ${EXE_CLI_SYNC_DATATOOL}.")
endif()

## -- lycoris converter
add_executable(${EXE_CLI_LYCORIS_DATACON} src/lycorisCliConverter.cxx)
target_link_libraries(${EXE_CLI_LYCORIS_DATACON} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB} ${ROOT_LIBRARIES}  -L/usr/local/lib/kpix/ -lkpix )
list(APPEND INSTALL_TARGETS ${EXE_CLI_LYCORIS_DATACON})

## -- mu3e root converter, direct save mimosa with trigger id out to root file
add_executable(${EXE_CLI_SYNC_DATATOOL} src/eudaqtoRoot.cpp)
target_link_libraries(${EXE_CLI_SYNC_DATATOOL} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB} ${ROOT_LIBRARIES} ${Boost_LIBRARIES})
list(APPEND INSTALL_TARGETS ${EXE_CLI_SYNC_DATATOOL})


include_directories(. include)
include_directories(${ROOT_INCLUDE_DIR})
include_directories(/usr/local/include/kpix)

##--- set run path of the dynamic kpix lib
set_target_properties(${INSTALL_TARGETS} PROPERTIES INSTALL_RPATH
  ${EUDAQ_INSTALL_RPATH}:/usr/local/lib/kpix/)

# Get all other source files to be compiled as executables: 
FILE(GLOB TARGET_FILES "src/eu*.cxx")

FOREACH(TFILE ${TARGET_FILES})
  GET_FILENAME_COMPONENT(TNAME ${TFILE} NAME_WE)
  MESSAGE(STATUS "Building device ${TNAME}")
  MESSAGE(STATUS "Building file ${TFILE}")
  ADD_EXECUTABLE(${TNAME} ${TFILE})
  TARGET_LINK_LIBRARIES(${TNAME} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB})
  LIST(APPEND INSTALL_TARGETS ${TNAME})
ENDFOREACH()

install(TARGETS ${INSTALL_TARGETS}
  DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
