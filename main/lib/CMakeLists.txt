option(USE_ROOT "Compiling main library using ROOT" OFF)
if (USE_ROOT)
   FIND_PACKAGE( ROOT REQUIRED )
   if (ROOT_FOUND)
     INCLUDE_DIRECTORIES( ${ROOT_INCLUDE_DIR} )
     SET(ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES}  ${ROOT_LIBRARIES})
     ADD_DEFINITIONS(-DROOT_FOUND)
   endif (ROOT_FOUND)
endif (USE_ROOT)

AUX_SOURCE_DIRECTORY( src library_sources )
AUX_SOURCE_DIRECTORY( plugins local_plugin_sources )

# Preprocess source files for converter plugins from subdirectories:
set(PREP_PLUGIN_SRCS)
get_property(PLUGIN_SRCS GLOBAL PROPERTY PLUGIN_SRCS_LIST)
foreach(s IN LISTS PLUGIN_SRCS)
  file(RELATIVE_PATH rs "${CMAKE_CURRENT_SOURCE_DIR}" "${s}")
  string(REGEX REPLACE "r$" "" o "${CMAKE_CURRENT_BINARY_DIR}/${rs}")
  add_custom_command(
    OUTPUT "${o}"
    COMMAND ${CMAKE_COMMAND} -E copy "${s}" "${o}"
    DEPENDS "${s}"
    COMMENT "Creating ${o}"
    VERBATIM
    )
  list(APPEND PREP_PLUGIN_SRCS "${o}")
endforeach()

MESSAGE(STATUS "Converter plugin files: ${PREP_PLUGIN_SRCS}")
MESSAGE(STATUS "Converter plugins in main: ${local_plugin_sources}")

ADD_LIBRARY( ${PROJECT_NAME} SHARED ${library_sources} ${local_plugin_sources} ${PREP_PLUGIN_SRCS} ${plugins_sources})
SOURCE_GROUP(plugins .*Plugin.*cc)
SOURCE_GROUP(FileIO .*File.*cc)
SOURCE_GROUP(TCPIP .*Transport.*cc)
SOURCE_GROUP(TCPIP FILES src\\Serializable.cc src\\Producer.cc src\\CommandReceiver.cc src\\DataCollector.cc src\\DataSender.cc  src\\LogCollector.cc)
SOURCE_GROUP(Event .*Event.*cc)
SOURCE_GROUP(Utilities FILES src\\Utils.cc src\\Time.cc src\\Mutex.cc src\\EudaqThread.cc)

target_link_libraries( ${PROJECT_NAME} ${EUDAQ_THREADS_LIB} ${ADDITIONAL_LIBRARIES})


INSTALL(TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
