# 3.16 mininum for Qt6 according to  professional cmake: a practical guide
cmake_minimum_required(VERSION 3.16.0)

#include(CMakeDependentOption)

find_package(Qt6 QUIET COMPONENTS Widgets)

if (NOT Qt6_FOUND)
  find_package(Qt5 5.15 QUIET COMPONENTS Widgets)
endif()

set(Qt5_or_Qt6_FOUND (Qt6_FOUND OR Qt5_FOUND))

cmake_dependent_option(EUDAQ_BUILD_GUI "Compile GUI executables (requires QT6 or QT5)" ON
  "Qt5_or_Qt6_FOUND" OFF)

if(NOT EUDAQ_BUILD_GUI)
  message(STATUS "GUIs of executables (euRun, euLog) are NOT to be built (EUDAQ_BUILD_GUI=OFF)")
  if(NOT Qt5_or_Qt6_FOUND)
     message(STATUS "reason: neither Qt5 nor Qt6 found")
  endif()
  return()
endif()

if(Qt6_FOUND)
  message(STATUS "GUIs of executables (euRun, euLog) are to be built (EUDAQ_BUILD_GUI=ON) with Qt6")

  set(CMAKE_AUTOUIC YES)
  set(CMAKE_AUTORCC YES)
  set(CMAKE_INCLUDE_CURRENT_DIR YES)

  qt_wrap_cpp(euRun_MOCSrcs include/RunControlModel.hh include/euRun.hh)
  qt_wrap_ui(euRun_UIrcs ui/euRun.ui)

  add_executable(euRun 
    ${euRun_MOCSrcs}
    ${euRun_UIrcs}
    src/euRun.cxx
    src/euRun.cc
    src/RunControlModel.cc
    src/euRun.rc
    src/scanHelper.cc
    images/euRun.qrc
    )
  
  target_include_directories(euRun PRIVATE include )
  target_link_libraries(euRun ${EUDAQ_CORE_LIBRARY} Qt6::Widgets)

  qt_wrap_cpp(euLog_MOCSrcs include/LogCollectorModel.hh include/euLog.hh)
  qt_wrap_ui(euLog_UIrcs ui/euLog.ui ui/LogDialog.ui)

  add_executable(euLog 
    ${euLog_MOCSrcs}
    ${euLog_UIrcs}
    src/euLog.cxx
    src/euLog.cc
    src/LogCollectorModel.cc
    images/euLog.qrc
    )


  target_include_directories(euLog PRIVATE include )
  target_link_libraries(euLog ${EUDAQ_CORE_LIBRARY} Qt6::Widgets)

  install(TARGETS euRun euLog
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)  
    
elseif()
  message(STATUS "GUIs of executables (euRun, euLog) are to be built (EUDAQ_BUILD_GUI=ON) with Qt5")

  set(CMAKE_AUTOUIC YES)
  set(CMAKE_AUTORCC YES)
  set(CMAKE_INCLUDE_CURRENT_DIR YES)

  qt_wrap_cpp(euRun_MOCSrcs include/RunControlModel.hh include/euRun.hh)
  qt_wrap_ui(euRun_UIrcs ui/euRun.ui)

  add_executable(euRun 
    ${euRun_MOCSrcs}
    ${euRun_UIrcs}
    src/euRun.cxx
    src/euRun.cc
    src/RunControlModel.cc
    src/euRun.rc
    src/scanHelper.cc
    images/euRun.qrc
    )
  
  target_include_directories(euRun PRIVATE include )
  target_link_libraries(euRun ${EUDAQ_CORE_LIBRARY} Qt5::Widgets)

  qt_wrap_cpp(euLog_MOCSrcs include/LogCollectorModel.hh include/euLog.hh)
  qt_wrap_ui(euLog_UIrcs ui/euLog.ui ui/LogDialog.ui)

  add_executable(euLog 
    ${euLog_MOCSrcs}
    ${euLog_UIrcs}
    src/euLog.cxx
    src/euLog.cc
    src/LogCollectorModel.cc
    images/euLog.qrc
    )

  target_include_directories(euLog PRIVATE include )
  target_link_libraries(euLog ${EUDAQ_CORE_LIBRARY} Qt5::Widgets)

  install(TARGETS euRun euLog
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)  
    
endif()
