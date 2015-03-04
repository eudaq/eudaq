FIND_PACKAGE(PythonLibs)
IF(PYTHONLIBS_FOUND)
  MESSAGE(STATUS "Found PythonLibs: ${PYTHON_INCLUDE_DIRS}")
  INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_DIRS})
  ADD_LIBRARY( PyIndexReader SHARED 
	  src/PyIndexReader.cpp
  )
  target_link_libraries(PyIndexReader EUDAQ ${EUDAQ_THREADS_LIB})
  ADD_LIBRARY( PyAidaFileReader SHARED 
	  src/PyAidaFileReader.cpp
  )
  target_link_libraries(PyAidaFileReader EUDAQ ${EUDAQ_THREADS_LIB})
  
  INSTALL(TARGETS PyAidaFileReader PyIndexReader
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
  
ENDIF(PYTHONLIBS_FOUND)

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
