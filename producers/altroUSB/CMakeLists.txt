set(name "AltroUSBProducer.exe")
set(sourcefiles src/AltroUSBProducer.cxx src/AltroUSBProducer.cc)
set(ext_libraries ilcdaq_usb )
set(ext_lib_paths /usr/local/lib/altro)

LINK_DIRECTORIES( ${ext_lib_paths} )
INCLUDE_DIRECTORIES( include )
ADD_EXECUTABLE(${name} ${sourcefiles})

TARGET_LINK_LIBRARIES(${name}   EUDAQ ${EUDAQ_THREADS_LIB} ${ext_libraries})

INSTALL(TARGETS ${name}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
