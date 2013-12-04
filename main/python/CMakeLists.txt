FIND_PACKAGE(Threads REQUIRED) # pthreads

ADD_LIBRARY( PyEUDAQ SHARED 
  src/PyRunControl.cpp 
  src/PyProducer.cpp
  src/PyDataCollector.cpp
)

target_link_libraries(PyEUDAQ      EUDAQ ${CMAKE_THREAD_LIBS_INIT})

INSTALL(TARGETS PyEUDAQ 
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
