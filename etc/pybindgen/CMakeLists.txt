FIND_PACKAGE(PythonLibs)
IF(PYTHONLIBS_FOUND)
  MESSAGE(STATUS "Found PythonLibs xx")
  INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_DIRS})
  ADD_LIBRARY( PyIndexReader SHARED 
	  src/PyIndexReader.cpp
  )
  target_link_libraries(PyIndexReader ${PYTHON_LIBRARIES} EUDAQ ${EUDAQ_THREADS_LIB})
  ADD_LIBRARY( PyAidaFileReader SHARED 
	  src/PyAidaFileReader.cpp
  )
  target_link_libraries(PyAidaFileReader ${PYTHON_LIBRARIES} EUDAQ ${EUDAQ_THREADS_LIB})
  
  INSTALL(TARGETS PyAidaFileReader PyIndexReader
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
  
ENDIF(PYTHONLIBS_FOUND)

