set(name "PALPIDEFSProducer.exe")
set(sourcefiles src/PALPIDEFSProducer.cxx)
set(ext_libraries "palpidefs.so" "tinyxml.so")
set(ext_lib_paths "${CMAKE_PALPIDEFS_DRIVER_INCLUDE}")
set(ext_inc_paths "include" "${CMAKE_PALPIDEFS_DRIVER_INCLUDE}")
# config files to be installed
#set(ConfigFiles ni_autotrig_explorer_pedestalmeas.conf ni_coins_explorer_wpedestal.conf)

if (NOT CMAKE_PALPIDEFS_DRIVER_INCLUDE)
  message(FATAL_ERROR "Path to pALPIDEfs driver not set - you need to set CMAKE_PALPIDEFS_DRIVER_INCLUDE")
endif(NOT CMAKE_PALPIDEFS_DRIVER_INCLUDE)

FIND_PACKAGE( TINYXML REQUIRED )
if (TINYXML_FOUND)
  INCLUDE_DIRECTORIES( ${TINYXML_INCLUDE_DIRS} )
  SET(ADDITIONAL_LIBRARIES ${TINYXML_LIBRARIES} )
  ADD_DEFINITIONS(-DUSE_TINYXML)
endif (TINYXML_FOUND)

find_package(libusb-1.0 REQUIRED)

if (NOT EXISTS "${CMAKE_PALPIDEFS_DRIVER_INCLUDE}/TTestsetup.h")
  message(FATAL_ERROR "Path to pALPIDEfs driver is wrong - Could not find ${CMAKE_PALPIDEFS_DRIVER_INCLUDE}/TTestsetup.h")
endif (NOT EXISTS "${CMAKE_PALPIDEFS_DRIVER_INCLUDE}/TTestsetup.h")
set(driver_library "${CMAKE_PALPIDEFS_DRIVER_INCLUDE}/libpalpidefs.so" )

ADD_DEFINITIONS()
INCLUDE_DIRECTORIES( "include" ${CMAKE_PALPIDEFS_DRIVER_INCLUDE})
ADD_EXECUTABLE(${name} ${sourcefiles})

TARGET_LINK_LIBRARIES(${name} EUDAQ ${CMAKE_THREAD_LIBS_INIT} ${LIBUSB_1_LIBRARIES} ${TINYXML_LIBRARIES} ${driver_library})

INSTALL(TARGETS ${name}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)


INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/conf/palpidefs.conf DESTINATION ${CMAKE_INSTALL_PREFIX}/conf)
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/conf/palpidefs DESTINATION ${CMAKE_INSTALL_PREFIX}/conf)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/conf/palpidefs/full.xml DESTINATION ${CMAKE_INSTALL_PREFIX}/conf/palpidefs)
