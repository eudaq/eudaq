set(name "TakiProducer.exe")
set(sourcefiles src/TakiProducer.cxx)
set(ext_libraries "" )
set(ext_lib_paths "" )
set(ext_inc_paths "" )

LINK_DIRECTORIES( ${ext_lib_paths} )
INCLUDE_DIRECTORIES( include ${ext_inc_paths})
ADD_EXECUTABLE(${name} ${sourcefiles})
ADD_EXECUTABLE(TakiProducerFAKEDATA.exe src/TakiProducerFAKEDATA.cxx)

TARGET_LINK_LIBRARIES(${name}   EUDAQ ${EUDAQ_THREADS_LIB} ${ext_libraries})
TARGET_LINK_LIBRARIES(TakiProducerFAKEDATA.exe   EUDAQ ${EUDAQ_THREADS_LIB} ${ext_libraries})

INSTALL(TARGETS ${name} TakiProducerFAKEDATA.exe
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
