set(name "Timepix3Producer.exe")
set(sourcefiles src/Timepix3Producer.cxx)
set(ext_libraries "libSpidrTpx3Lib.so" )
set(ext_lib_paths "/afs/cern.ch/user/s/sarfaoui/work/public/SPIDR/software/trunk/Release" )
set(ext_inc_paths "./include" "/afs/cern.ch/user/s/sarfaoui/work/public/SPIDR/software/trunk/SpidrTpx3Lib")

ADD_DEFINITIONS()
LINK_DIRECTORIES( ${ext_lib_paths} )
INCLUDE_DIRECTORIES( ${ext_inc_paths})
ADD_EXECUTABLE(${name} ${sourcefiles})

TARGET_LINK_LIBRARIES(${name}   EUDAQ ${EUDAQ_THREADS_LIB} ${ext_libraries})

INSTALL(TARGETS ${name}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
