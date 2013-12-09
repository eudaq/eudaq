set(name "EUDRBProducer.exe")
set(sourcefiles src/EUDRBProducer.cxx src/EUDRBController.cc)
set(incdirs vmelib/include )

# include and build vmelib
add_subdirectory( vmelib )

ADD_DEFINITIONS("-DVME_TRACE=0")
INCLUDE_DIRECTORIES( include ${incdirs})
ADD_EXECUTABLE(${name} ${sourcefiles})

TARGET_LINK_LIBRARIES(${name}   EUDAQ ${EUDAQ_THREADS_LIB} vmelib)

INSTALL(TARGETS ${name}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
