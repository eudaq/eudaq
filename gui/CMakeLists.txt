# 3.16 mininum for Qt6 according to  professional cmake: a practical guide
cmake_minimum_required(VERSION 3.16.0)

include(CMakeDependentOption)

option(EUDAQ_BUILD_GUI "Compile GUI executables (requires QT6 or QT5)" ON)
cmake_dependent_option(EUDAQ_FORCE_QT5 "Force usage QT5 even if Qt6 is available" OFF "EUDAQ_BUILD_GUI" OFF)

if(NOT EUDAQ_FORCE_QT5)
  message(STATUS "checking for Qt6")
  find_package(Qt6 COMPONENTS Widgets)
endif()

if((NOT Qt6_FOUND) OR (NOT DEFINED Qt6_FOUND))
  message(STATUS "checking for Qt5")
  find_package(Qt5 5.15 COMPONENTS Widgets)
endif()

set(Qt5_or_Qt6_FOUND FALSE)
set(Qt5_or_Qt6_FOUND (Qt6_FOUND OR Qt5_FOUND))


if(EUDAQ_BUILD_GUI)
  if((NOT Qt5_or_Qt6_FOUND) AND (NOT EUDAQ_FORCE_QT5))
     message(STATUS "GUIs of executables (euRun, euLog) were meant to be built (EUDAQ_BUILD_GUI=ON) but finally it is not possible as neither Qt5 nor Qt6 were found")
     return()     
  elseif(NOT Qt5_or_Qt6_FOUND)
     message(STATUS "GUIs of executables (euRun, euLog) were meant to be built (EUDAQ_BUILD_GUI=ON) but finally it is not possible as Qt5 was forced but not found")
     return()     
  endif()
else()
  message(STATUS "GUIs of executables (euRun, euLog) are NOT to be built (EUDAQ_BUILD_GUI=OFF)")
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

   return()    
endif()

if(Qt5_FOUND)
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
   
   return() 
endif()
