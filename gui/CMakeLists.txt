# CMake script to build GUI applications
option(BUILD_gui "Compile GUI executables (requires QT5)?" ON)
if(NOT BUILD_gui)
  return()
endif()

include(CMakeDependentOption)
# Check if environment variable QTDIR is set.
# Extra security for windows environment as well.
if (DEFINED ENV{QTDIR})
  set(CMAKE_PREFIX_PATH $ENV{QTDIR} ${CMAKE_PREFIX_PATH})
endif()

# search for Qt5 package 'Widgets' (implies Core and Gui)
cmake_minimum_required(VERSION 3.1.0)
find_package(Qt5Widgets REQUIRED) # QUIET means it won't complain if not found
message(STATUS "Found Qt5Widgets: ${Qt5Widgets_DIR}")
message(STATUS "Found Qt ${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}.${QT_VERSION_PATCH}, will build GUI executables")

macro(qt_wrap_cpp)
  qt5_wrap_cpp(${ARGN})
endmacro()
macro(qt_wrap_ui)
  qt5_wrap_ui(${ARGN})
endmacro()

set(my_qt_libraries ${Qt5Widgets_LIBRARIES})
include_directories(${Qt5Widgets_INCLUDE_DIRS})
include_directories(./include)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
add_definitions(${Qt5Widgets_DEFINITIONS})

# this will run uic on .ui files:
QT_WRAP_UI(euRun_UISrcs ui/euRun.ui)
QT_WRAP_UI(euProd_UISrcs ui/euProd.ui)
QT_WRAP_UI(euLog_UISrcs ui/LogDialog.ui ui/euLog.ui)

# and finally this will run moc:
QT_WRAP_CPP(euRun_MOCSrcs include/RunControlModel.hh include/euRun.hh )
QT_WRAP_CPP(euProd_MOCSrcs include/euProd.hh )
QT_WRAP_CPP(euLog_MOCSrcs include/LogCollectorModel.hh include/euLog.hh )

# here we instruct CMake to build "sample" executable from all of the source files
add_executable(euRun  src/euRun.cc src/euRun.rc src/RunControlModel.cc ${euRun_MOCSrcs} ${euRun_UISrcs} )
add_executable(euProd  src/euProd.cc src/euProd.rc ${euProd_MOCSrcs} ${euProd_UISrcs} )
add_executable(euLog  src/euLog.cc src/euLog.rc src/LogCollectorModel.cc ${euLog_MOCSrcs} ${euLog_UISrcs} )

set_property(TARGET euRun   PROPERTY PROJECT_LABEL "BinEuRun") 
set_property(TARGET euProd   PROPERTY PROJECT_LABEL "BinEuProd") 
set_property(TARGET euLog   PROPERTY PROJECT_LABEL "BinEuLog") 

target_link_libraries(euRun   EUDAQ ${EUDAQ_THREADS_LIB} Qt5::Widgets)
target_link_libraries(euProd  EUDAQ ${EUDAQ_THREADS_LIB} Qt5::Widgets)
target_link_libraries(euLog   EUDAQ ${EUDAQ_THREADS_LIB} Qt5::Widgets)

install(TARGETS euRun  euProd  euLog 
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
