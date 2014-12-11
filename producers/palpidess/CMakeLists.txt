set(name "PalpidessProducer.exe")
set(sourcefiles src/DevicePalpidess.cc src/PalpidessProducer.cxx)
set(ext_libraries "" )
set(ext_lib_paths "" )
set(ext_inc_paths "include" )
# config files to be installed
#set(ConfigFiles ni_autotrig_explorer_pedestalmeas.conf ni_coins_explorer_wpedestal.conf)

ADD_DEFINITIONS()
LINK_DIRECTORIES( ${ext_lib_paths} )
INCLUDE_DIRECTORIES( ${ext_inc_paths})
ADD_EXECUTABLE(${name} ${sourcefiles})

TARGET_LINK_LIBRARIES(${name}   EUDAQ ${CMAKE_THREAD_LIBS_INIT} ${ext_libraries})

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
