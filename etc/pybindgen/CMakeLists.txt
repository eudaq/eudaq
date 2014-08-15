FIND_PACKAGE(PythonLibs)
IF(PYTHONLIBS_FOUND)

  set(GEN_FILES PyPyPacket PyAidaFileReader)
  
  foreach(_file ${GEN_FILES}) 
    add_custom_command (
      OUTPUT ${PROJECT_SOURCE_DIR}/etc/pybindgen/src/${_file}.cpp
      COMMAND python 
      ARGS ${PROJECT_SOURCE_DIR}/etc/pybindgen/py-gen/${_file}-gen.py > ${PROJECT_SOURCE_DIR}/etc/pybindgen/src/${_file}.cpp
      DEPENDS py-gen/${_file}-gen.py
    )
  
    INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_DIRS})
    ADD_LIBRARY( ${_file} SHARED 
	  src/${_file}.cpp
    )
    target_link_libraries(${_file} ${PYTHON_LIBRARIES} EUDAQ ${EUDAQ_THREADS_LIB})
  endforeach()  
  
  INSTALL(TARGETS PyAidaFileReader PyPyPacket
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
  
ENDIF(PYTHONLIBS_FOUND)

