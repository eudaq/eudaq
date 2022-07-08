option(EUDAQ_BUILD_PYTHON "Compile Python EUDAQ binding library with pybind11?" OFF)
if(NOT EUDAQ_BUILD_PYTHON)
  return()
endif()
#find_package(PythonLibsNew REQUIRED)
find_package(PythonLibs REQUIRED)

include_directories(${EUDAQ_INCLUDE_DIRS})
include_directories(${PYTHON_INCLUDE_DIRS})
aux_source_directory(src EUDAQ_COMPONENT_SRC)

set(PYBIND_NAME_VER pybind11-2.5.0)
set(PYBIND_ZIP_FILE ${PYBIND_NAME_VER}.zip)

set(PYBIND_HEADER_FILE ${CMAKE_CURRENT_BINARY_DIR}/${PYBIND_NAME_VER}/include/pybind11/pybind11.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/${PYBIND_NAME_VER}/include)

add_custom_command(
  OUTPUT ${PYBIND_HEADER_FILE}
  COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_CURRENT_LIST_DIR}/extern/${PYBIND_ZIP_FILE} 
  COMMAND ${CMAKE_COMMAND} -E touch_nocreate ${PYBIND_HEADER_FILE}
  DEPENDS extern/${PYBIND_ZIP_FILE}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Unpacking ${PYBIND_ZIP_FILE}"
  VERBATIM
  )

set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_SHARED_MODULE_PREFIX  "")
add_library(pyeudaq SHARED ${EUDAQ_COMPONENT_SRC} ${PYBIND_HEADER_FILE})
target_link_libraries(pyeudaq ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB} ${PYTHON_LIBRARIES})

if (APPLE)
    set_property(TARGET pyeudaq PROPERTY OUTPUT_NAME "pyeudaq.so")
    set_property(TARGET pyeudaq PROPERTY SUFFIX "")
endif()

install(TARGETS pyeudaq
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
