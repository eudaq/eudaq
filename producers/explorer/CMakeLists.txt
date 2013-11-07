set(name "ExplorerProducer.exe")
set(sourcefiles src/DeviceExplorer.cc src/ExplorerProducer.cxx)
set(ext_libraries "" )
set(ext_lib_paths "" )
set(ext_inc_paths "include" )

ADD_DEFINITIONS()
LINK_DIRECTORIES( ${ext_lib_paths} )
INCLUDE_DIRECTORIES( ${ext_inc_paths})
ADD_EXECUTABLE(${name} ${sourcefiles})

TARGET_LINK_LIBRARIES(${name}   EUDAQ ${CMAKE_THREAD_LIBS_INIT} ${ext_libraries})

INSTALL(TARGETS ${name}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
