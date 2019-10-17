include_directories(include)
aux_source_directory(src MODULE_SRC)
find_package(ROOT REQUIRED)
if(NOT EUDAQ_LCIO_LIBRARY)
  list(REMOVE_ITEM MODULE_SRC src/NiRawEvent2LCEventConverter.cc)
endif()

if(NOT USER_MuPix8_BUILD)
  list(REMOVE_ITEM MODULE_SRC src/*.cc)
endif()


FIND_PACKAGE(MUPIX8DAQ)
IF(NOT MUPIX8DAQ_FOUND)
    MESSAGE(FATAL_ERROR "Could not find MuPix-telescope. You need to do: \n export MUPIX8DAQ=/path/to/repo")
ENDIF()
INCLUDE_DIRECTORIES(SYSTEM ${MUPIX8DAQ_INCLUDE_DIR})

MESSAGE(STATUS "-----${ROOT_LIBRARIES}")
add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
target_link_libraries(${EUDAQ_MODULE} ${ROOT_LIBRARIES} ${MUPIX8DAQ_LIBRARY} ${EUDAQ_CORE_LIBRARY}
  ${EUDAQ_LCIO_LIBRARY} ${LCIO_LIBRARIES} )

# Add to modules library:
target_link_libraries(modules INTERFACE ${EUDAQ_MODULE})

install(TARGETS
  ${EUDAQ_MODULE}
  EXPORT eudaqTargets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
