# CMake script to build GUI applications
option(EUDAQ_BUILD_GUI "Compile GUI executables (requires QT5/4)" ON)
if(NOT EUDAQ_BUILD_GUI)
  return()
endif()

include(CMakeDependentOption)

cmake_minimum_required(VERSION 3.1.0)
find_package(Qt5Widgets QUIET)
if(Qt5Widgets_FOUND)
  message(STATUS "Found Qt5Widgets: ${Qt5Widgets_DIR}")
  macro(qt_wrap_cpp)
    qt5_wrap_cpp(${ARGN})
  endmacro()
  macro(qt_wrap_ui)
    qt5_wrap_ui(${ARGN})
  endmacro()
  set(EXTERNAL_LIB_QT Qt5::Widgets)
  include_directories(${Qt5Widgets_INCLUDE_DIRS})
  add_definitions(${Qt5Widgets_DEFINITIONS})
else()
  message(STATUS "NOT found Qt5Widgets.")
  message(STATUS "Please setup Qt5Widgets_DIR, if Qt5 is intended to be built with.")
  message(STATUS "Try to find Qt4...")
  find_package(Qt4 REQUIRED COMPONENTS QtCore QtGui)
  macro(qt_wrap_cpp)
    qt4_wrap_cpp(${ARGN})
  endmacro()
  macro(qt_wrap_ui)
    qt4_wrap_ui(${ARGN})
  endmacro()
  set(EXTERNAL_LIB_QT Qt4::QtCore Qt4::QtGui)
  include_directories(${QT_INCLUDES})
  add_definitions(${QT_DEFINITIONS})
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(./include)


set(RUN_GUI_SRC
  src/euRun.cxx
  src/euRun.cc
  src/RunControlModel.cc
  # src/euRun.rc
  )
qt_wrap_ui(euRun_UISrcs ui/euRun.ui)
qt_wrap_cpp(euRun_MOCSrcs include/RunControlModel.hh include/euRun.hh )
list(APPEND RUN_GUI_SRC ${euRun_UISrcs} ${euRun_MOCSrcs})
add_executable(euRun ${RUN_GUI_SRC})
target_link_libraries(euRun ${EUDAQ_CORE_LIBRARY} ${EXTERNAL_LIB_QT})
list(APPEND INSTALL_TARGETS euRun)

set(LOG_GUI_SRC
  src/euLog.cxx
  src/euLog.cc
  src/LogCollectorModel.cc
  # src/euLog.rc
  )

set(PRD_GUI_SRC
  src/euProd.cxx
  src/euProd.cc
  # src/euProd.rc
  )


set(MODULE_SRC
  src/euLog.cc
  src/LogCollectorModel.cc
  src/euProd.cc
  # src/euLog.rc
  # src/euProd.rc
  )


qt_wrap_cpp(euLog_MOCSrcs include/LogCollectorModel.hh include/euLog.hh )
qt_wrap_ui(euLog_UISrcs ui/LogDialog.ui ui/euLog.ui)
list(APPEND MODULE_SRC ${euLog_UISrcs} ${euLog_MOCSrcs})

qt_wrap_cpp(euProd_MOCSrcs include/euProd.hh )
qt_wrap_ui(euProd_UISrcs ui/euProd.ui)
list(APPEND MODULE_SRC ${euProd_UISrcs} ${euProd_MOCSrcs})

get_filename_component(MODULE_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE module_${MODULE_NAME})
add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY} ${EXTERNAL_LIB_QT})
list(APPEND INSTALL_TARGETS ${EUDAQ_MODULE})

add_executable(euLog src/euLog.cxx)
set_property(TARGET euLog PROPERTY PROJECT_LABEL "BinEuLog") 
target_link_libraries(euLog ${EUDAQ_CORE_LIBRARY} ${EXTERNAL_LIB_QT})
list(APPEND INSTALL_TARGETS euLog)

add_executable(euProd src/euProd.cxx)
set_property(TARGET euProd PROPERTY PROJECT_LABEL "BinEuProd")
target_link_libraries(euProd ${EUDAQ_CORE_LIBRARY} ${EXTERNAL_LIB_QT})
list(APPEND INSTALL_TARGETS euProd)

install(TARGETS ${INSTALL_TARGETS}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
