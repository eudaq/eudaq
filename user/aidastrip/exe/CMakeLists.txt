
## OPT: further choose if you want specific kpix executable or not?
if(NOT USER_AIDASTRIP_EXE)
  message(STATUS "Disable building of aux. AIDASTRIP executables user/aidastrip/exe (USER_AIDASTRIP_EXE=OFF)")
  return()
endif()


##-- add include directories:
include_directories( ${LIBXML2_INCLUDE_DIR} )
#include_directories(${KPIX_INCLUDE_DIR})
include_directories(. include)

#include_directories(${ROOT_INCLUDE_DIR})
if(ROOT_USE_FILE)
  include(${ROOT_USE_FILE})
  message("[exe] root include dir is ${ROOT_USE_FILE}")
endif()

##-- Setup global variables:
#set(ROOT_DIR $ENV{ROOTSYS}/cmake)
set(EXE_CLI_LYCORIS_DATACON lycorisCliConverter)
set(EXE_CLI_SYNC_DATATOOL lycorisSyncTool)


##-- Adding individual executables:
## 1) lycoris converter
add_executable(${EXE_CLI_LYCORIS_DATACON} src/lycorisCliConverter.cxx)
target_link_libraries(${EXE_CLI_LYCORIS_DATACON}
  ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB}
  ${ROOT_LIBRARIES}
  ${KPIX_LIBRARY} # this handles the IMPORTED targets auto and lets embed compiler flags, include paths, library dependencies.
  ${LIBXML2_LIBRARIES}
  )
list(APPEND INSTALL_TARGETS ${EXE_CLI_LYCORIS_DATACON})


## 2) optional: mu3e root converter, direct save mimosa with trigger id out to root file
## -- this code needs 'Boost' lib
# FIND_PACKAGE( Boost 1.58 COMPONENTS program_options QUIET )
# if(Boost_FOUND)
#   message(STATUS "Boost 1.58 found! Compile ${EXE_CLI_SYNC_DATATOOL}.")
#   INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )
#   add_executable(${EXE_CLI_SYNC_DATATOOL} src/eudaqtoRoot.cpp)
#   target_link_libraries(${EXE_CLI_SYNC_DATATOOL} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB} ${ROOT_LIBRARIES} ${Boost_LIBRARIES})
# list(APPEND INSTALL_TARGETS ${EXE_CLI_SYNC_DATATOOL})
# endif()


##--- set run path of the dynamic kpix lib
set_target_properties(${INSTALL_TARGETS}
  PROPERTIES INSTALL_RPATH
  #${EUDAQ_INSTALL_RPATH}:${KPIX_LIB_DIR}
  ${EUDAQ_INSTALL_RPATH}
  )

# Get all other source files to be compiled as executables:
FILE(GLOB TARGET_FILES "src/eu*.cxx")

FOREACH(TFILE ${TARGET_FILES})
  GET_FILENAME_COMPONENT(TNAME ${TFILE} NAME_WE)
  MESSAGE(STATUS "Building device ${TNAME}")
  MESSAGE(STATUS "Building file ${TFILE}")
  ADD_EXECUTABLE(${TNAME} ${TFILE})
  TARGET_LINK_LIBRARIES(${TNAME} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB} ${ROOT_LIBRARIES})
  LIST(APPEND INSTALL_TARGETS ${TNAME})
ENDFOREACH()

install(TARGETS ${INSTALL_TARGETS}
  DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
