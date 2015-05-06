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

if (NOT EXISTS "${ext_lib_paths}/TTestsetup.h")
  message(FATAL_ERROR "Path to pALPIDEfs driver is wrong - Could not find ${ext_lib_paths}/TTestsetup.h")
endif (NOT EXISTS "${ext_lib_paths}/TTestsetup.h")

ADD_DEFINITIONS()
LINK_DIRECTORIES( ${ext_lib_paths} )
INCLUDE_DIRECTORIES( ${ext_inc_paths})
ADD_EXECUTABLE(${name} ${sourcefiles})

TARGET_LINK_LIBRARIES(${name} EUDAQ ${CMAKE_THREAD_LIBS_INIT} ${ext_libraries} ${LIBUSB_1_LIBRARIES} ${TINYXML_LIBRARIES})

INSTALL(TARGETS ${name}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

foreach(ConfigFile ${ConfigFiles})
  if (UNIX)
    INSTALL(CODE "EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/conf/${ConfigFile} ${CMAKE_INSTALL_PREFIX}/conf/${ConfigFile})")
  else()
    INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/conf/${ConfigFile} DESTINATION ${CMAKE_INSTALL_PREFIX}/conf)
   endif()
endforeach()
