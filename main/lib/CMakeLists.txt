option(USE_ROOT "Compiling main library using ROOT" OFF)
if (USE_ROOT)
   FIND_PACKAGE( ROOT REQUIRED )
else (USE_ROOT)

   FIND_PACKAGE( ROOT )

endif (USE_ROOT)

if (ROOT_FOUND)
  INCLUDE_DIRECTORIES( ${ROOT_INCLUDE_DIR} )
  SET(ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES}  ${ROOT_LIBRARIES})
  ADD_DEFINITIONS(-DROOT_FOUND)
endif (ROOT_FOUND)

option(USE_LCIO "Compiling main library using LCIO" OFF)
IF(USE_LCIO)
  FIND_PACKAGE(LCIO REQUIRED)
  INCLUDE_DIRECTORIES(${LCIO_INCLUDE_DIRS})
  SET(ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES} ${LCIO_LIBRARIES} )
  add_definitions(${LCIO_DEFINITIONS})
ENDIF(USE_LCIO)

AUX_SOURCE_DIRECTORY( src library_sources )
AUX_SOURCE_DIRECTORY( plugins plugins_sources )

option(USE_TINYXML "Compiling main library using TinyXML" OFF)
if (USE_TINYXML OR BUILD_palpidefs)
  FIND_PACKAGE( TINYXML REQUIRED )
  if (TINYXML_FOUND)
    INCLUDE_DIRECTORIES( ${TINYXML_INCLUDE_DIRS} )
    SET(ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES} ${TINYXML_LIBRARIES} )
    ADD_DEFINITIONS(-DUSE_TINYXML)
    INCLUDE_DIRECTORIES( "include" ${CMAKE_PALPIDEFS_DRIVER_INCLUDE})
    message("Compiling main library using TinyXML")
  endif (TINYXML_FOUND)
endif (USE_TINYXML OR BUILD_palpidefs)

if (BUILD_palpidefs)
  if (NOT EXISTS "${CMAKE_PALPIDEFS_DRIVER_INCLUDE}/TTestsetup.h")
    message(FATAL_ERROR "Path to pALPIDEfs driver is wrong - Could not find ${CMAKE_PALPIDEFS_DRIVER_INCLUDE}/TTestsetup.h")
  endif (NOT EXISTS "${CMAKE_PALPIDEFS_DRIVER_INCLUDE}/TTestsetup.h")
  if (NOT EXISTS "${CMAKE_PALPIDEFS_DRIVER_INCLUDE}/libpalpidefs.so")
    message(FATAL_ERROR "Path to pALPIDEfs driver is wrong - Could not find ${CMAKE_PALPIDEFS_DRIVER_INCLUDE}/libpalpidefs.so")
  endif (NOT EXISTS "${CMAKE_PALPIDEFS_DRIVER_INCLUDE}/libpalpidefs.so")
  FIND_PACKAGE(libusb-1.0)
  if (NOT EXISTS "${LIBUSB_1_INCLUDE_DIRS}/libusb-1.0/libusb.h")
    message(FATAL_ERROR "Path to libusb-1.0/libusb.h for the pALPIDEfs driver is wrong - Could not find ${LIBUSB_1_INCLUDE_DIRS}/libusb-1.0/libusb.h")
  endif (NOT EXISTS "${LIBUSB_1_INCLUDE_DIRS}/libusb-1.0/libusb.h")
  INCLUDE_DIRECTORIES(${CMAKE_PALPIDEFS_DRIVER_INCLUDE} ${LIBUSB_1_INCLUDE_DIRS})
  SET(ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES} ${TINYXML_LIBRARIES} ${CMAKE_PALPIDEFS_DRIVER_INCLUDE}/libpalpidefs.so ${LIBUSB_1_LIBRARIES})
  ADD_DEFINITIONS(-DPALPIDEFS)
endif (BUILD_palpidefs)

list(REMOVE_ITEM plugins_sources plugins/GenericConverterPlugin.cc)

FIND_PACKAGE(PXAR)
IF(PXARCORE_FOUND)
  INCLUDE_DIRECTORIES(${PXARCORE_INCLUDE_DIRS})
  SET(ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES}  ${PXARCORE_LIBRARIES})
ELSE(PXARCORE_FOUND)
  MESSAGE(STATUS "NOT building converter plugin for CMSPixel.")
  list(REMOVE_ITEM plugins_sources plugins/CMSPixelConverterPlugin.cc)
  list(REMOVE_ITEM plugins_sources plugins/CMSPixelTRPConverterPlugin.cc)
  list(REMOVE_ITEM plugins_sources plugins/CMSPixelQUADConverterPlugin.cc)
  list(REMOVE_ITEM plugins_sources plugins/CMSPixelREFConverterPlugin.cc)
  list(REMOVE_ITEM plugins_sources plugins/CMSPixelDUTConverterPlugin.cc)
ENDIF(PXARCORE_FOUND)

if (PROTOBUF_FOUND AND GEN_proto)
    FILE(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/pb_gen )
    add_custom_target(
            GEN_PROTO ${PROTOBUF_PROTOC_EXECUTABLE} --proto_path=${PROJECT_SOURCE_DIR}/main/include/eudaq --cpp_out=${CMAKE_BINARY_DIR}/pb_gen --python_out=${PROJECT_SOURCE_DIR}/python ${PROJECT_SOURCE_DIR}/main/include/eudaq/StandardEvent.proto
            #COMMAND mv ${PROJECT_SOURCE_DIR}/main/lib/plugins/StandardEvent.pb.h ${PROJECT_SOURCE_DIR}/main/include/eudaq
            #COMMAND sed -i 's/StandardEvent.pb.h/eudaq\\/StandardEvent.pb.h/g' ${PROJECT_SOURCE_DIR}/main/lib/plugins/StandardEvent.pb.cc
        )
    list(APPEND plugins_sources plugins/GenericConverterPlugin.cc)

    list(APPEND plugins_sources ${CMAKE_BINARY_DIR}/pb_gen/StandardEvent.pb.cc)
    set_source_files_properties(${CMAKE_BINARY_DIR}/pb_gen/StandardEvent.pb.cc PROPERTIES GENERATED TRUE)
    INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/pb_gen)

endif()

ADD_LIBRARY( ${PROJECT_NAME} SHARED ${library_sources} ${plugins_sources})

if (PROTOBUF_FOUND AND GEN_proto)
    add_dependencies(${PROJECT_NAME} GEN_PROTO)
endif ()

SOURCE_GROUP(plugins .*Plugin.*cc)
SOURCE_GROUP(FileIO .*File.*cc)
SOURCE_GROUP(TCPIP .*Transport.*cc)
SOURCE_GROUP(TCPIP FILES src\\Serializable.cc src\\Producer.cc src\\CommandReceiver.cc src\\Controller.cc src\\DataCollector.cc src\\DataSender.cc  src\\LogCollector.cc)
SOURCE_GROUP(Event .*Event.*cc)
SOURCE_GROUP(Utilities FILES src\\Utils.cc src\\Time.cc src\\Mutex.cc src\\EudaqThread.cc)

target_link_libraries( ${PROJECT_NAME} ${EUDAQ_THREADS_LIB} ${ADDITIONAL_LIBRARIES})

INSTALL(TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
