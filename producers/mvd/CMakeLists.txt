set(name "MVDProducer.exe")
set(sourcefiles src/MVDProducer.cxx src/MVDController.cc src/ModV551.cc src/ModV550.cc)
set(ext_libraries "vme" )
set(ext_lib_paths "" )
set(ext_inc_paths "" )

ADD_DEFINITIONS(-DVME_TRACE=0)
LINK_DIRECTORIES( ${ext_lib_paths} )
INCLUDE_DIRECTORIES( include ${ext_inc_paths})
ADD_EXECUTABLE(${name} ${sourcefiles})

TARGET_LINK_LIBRARIES(${name}   EUDAQ ${EUDAQ_THREADS_LIB} ${ext_libraries})

INSTALL(TARGETS ${name}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
