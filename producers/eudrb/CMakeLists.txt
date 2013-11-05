set(name "EUDRBProducer.exe")
set(sourcefiles src/EUDRBProducer.cxx src/EUDRBController.cc)
set(ext_libraries "vme" )
set(ext_lib_paths "" )
set(ext_inc_paths "" )

ADD_DEFINITIONS(VME_TRACE=0)
LINK_DIRECTORIES( ${ext_lib_paths} )
INCLUDE_DIRECTORIES( include )
ADD_EXECUTABLE(${name} ${sourcefiles})

TARGET_LINK_LIBRARIES(${name}   EUDAQ ${CMAKE_THREAD_LIBS_INIT} ${ext_libraries})

INSTALL(TARGETS ${name}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
