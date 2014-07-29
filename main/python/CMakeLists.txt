

ADD_LIBRARY( PyEUDAQ SHARED 
  src/PyRunControl.cpp 
  src/PyProducer.cpp
  src/PyDataCollector.cpp
  src/PyLogCollector.cpp
)

target_link_libraries(PyEUDAQ      EUDAQ ${EUDAQ_THREADS_LIB})

INSTALL(TARGETS PyEUDAQ 
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
