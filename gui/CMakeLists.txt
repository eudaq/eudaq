# 3.16 mininum for Qt6 according to  professional cmake: a practical guide
cmake_minimum_required(VERSION 3.16.0)

find_package(Qt5 5.15 REQUIRED
COMPONENTS Widgets Core
)

set(CMAKE_AUTOUIC YES)
set(CMAKE_AUTORCC YES)
set(CMAKE_INCLUDE_CURRENT_DIR YES)

cmake_dependent_option(EUDAQ_BUILD_GUI "Compile GUI executables (requires QT5)" ON
  "Qt5Widgets_FOUND" OFF)

#include(CMakeDependentOption)
if(NOT EUDAQ_BUILD_GUI)
    message(STATUS "GUIs of executables (euRun, euLog) are NOT to be built (EUDAQ_BUILD_GUI=OFF)")
  return()
endif()

message(STATUS "GUIs of executables (euRun, euLog) are to be built (EUDAQ_BUILD_GUI=ON)")

qt5_wrap_cpp(euRun_MOCSrcs include/RunControlModel.hh include/euRun.hh)

add_executable(euRun 
  ${euRun_MOCSrcs}
  src/euRun.cxx
  src/euRun.cc
  src/RunControlModel.cc
  src/euRun.rc
  src/scanHelper.cc
  ui/euRun.ui
  images/euRun.qrc
  )
  
target_include_directories(euRun PRIVATE include )
target_link_libraries(euRun ${EUDAQ_CORE_LIBRARY} Qt5::Widgets)

qt5_wrap_cpp(euLog_MOCSrcs include/LogCollectorModel.hh include/euLog.hh)

add_executable(euLog 
  ${euLog_MOCSrcs}
  src/euLog.cxx
  src/euLog.cc
  src/LogCollectorModel.cc
  ui/LogDialog.ui 
  ui/euLog.ui
  images/euLog.qrc
  )


target_include_directories(euLog PRIVATE include )
target_link_libraries(euLog ${EUDAQ_CORE_LIBRARY} Qt5::Widgets)


install(TARGETS euRun euLog
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
