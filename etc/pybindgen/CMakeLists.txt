FIND_PACKAGE(PythonLibs)
IF(PYTHONLIBS_FOUND)

# add the command to generate the source code
  add_custom_command (
    OUTPUT ${PROJECT_SOURCE_DIR}/etc/pybindgen/src/PyPyPacket.cpp
    COMMAND python 
    ARGS ${PROJECT_SOURCE_DIR}/etc/pybindgen/py-gen/PyPyPacket-gen.py > ${PROJECT_SOURCE_DIR}/etc/pybindgen/src/PyPyPacket.cpp
    DEPENDS py-gen/PyPyPacket-gen.py
  )
  
  INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_DIRS})
  ADD_LIBRARY( PyIndexReader SHARED 
	  src/PyIndexReader.cpp
  )
  target_link_libraries(PyIndexReader ${PYTHON_LIBRARIES} EUDAQ ${EUDAQ_THREADS_LIB})
  
  ADD_LIBRARY( PyAidaFileReader SHARED 
	  src/PyAidaFileReader.cpp
  )
  target_link_libraries(PyAidaFileReader ${PYTHON_LIBRARIES} EUDAQ ${EUDAQ_THREADS_LIB})
  
  ADD_LIBRARY( PyPyPacket SHARED 
	  ${PROJECT_SOURCE_DIR}/etc/pybindgen/src/PyPyPacket.cpp
  )
  target_link_libraries(PyPyPacket ${PYTHON_LIBRARIES} EUDAQ ${EUDAQ_THREADS_LIB})
  
  INSTALL(TARGETS PyAidaFileReader PyIndexReader PyPyPacket
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
  
ENDIF(PYTHONLIBS_FOUND)

