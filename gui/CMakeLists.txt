# CMake script to build GUI applications
# it will search for either Qt4 or Qt5, starting with Qt5

include(CMakeDependentOption)

IF(POLICY CMP0020)
  CMAKE_POLICY(SET CMP0011 NEW) # disabling a warning about policy changing in this scope
  CMAKE_POLICY(SET CMP0020 OLD) # explicitly disabling automatic linking of Qt executables to qtmain target on Windows (changed in CMake 2.8.10)
ENDIF(POLICY CMP0020)


option(BUILD_WITH_QT4 "Build with Qt4 no matter whether Qt5 is found or not" OFF)

if( NOT BUILD_WITH_QT4 )
    # search for Qt5 package 'Widgets' (implies Core and Gui)
    find_package(Qt5Widgets QUIET) # QUIET means it won't complain if not found
    if( Qt5Widgets_DIR)
        message(STATUS "Found Qt5Widgets: ${Qt5Gui_DIR}")
        macro(qt_wrap_cpp)
            qt5_wrap_cpp(${ARGN})
        endmacro()
        macro(qt_wrap_ui)
            qt5_wrap_ui(${ARGN})
        endmacro()
	# setup Qt5 libaries and include paths
	set (my_qt_libraries ${Qt5Widgets_LIBRARIES})
	include_directories(${Qt5Widgets_INCLUDES})
	ADD_DEFINITIONS(${Qt5Widgets_DEFINITIONS})
    endif()
endif()

if( NOT Qt5Gui_DIR )
    message(STATUS "Could not find Qt5, searching for Qt4 instead...")
    find_package( Qt4 )
    macro(qt5_use_modules)
    endmacro()
    macro(qt_wrap_cpp)
      qt4_wrap_cpp(${ARGN})
    endmacro()
    macro(qt_wrap_ui)
      qt4_wrap_ui(${ARGN})
    endmacro()
    # setup Qt4 libaries and include paths
    include( ${QT_USE_FILE} )
    set (my_qt_libraries ${QT_LIBRARIES})
    ADD_DEFINITIONS(${QT_DEFINITIONS})
endif()


IF (NOT Qt5Gui_DIR AND NOT QT4_FOUND)
  MESSAGE(WARNING "Neither Qt4 nor Qt5 were not found, will not build gui applications!")
ELSE()
  message(STATUS "Found Qt, will build GUI executables")
  # this will run uic on .ui files:
  QT_WRAP_UI(euRun_UISrcs ui/euRun.ui)
  QT_WRAP_UI(euProd_UISrcs ui/euProd.ui)
  QT_WRAP_UI(euLog_UISrcs ui/LogDialog.ui ui/euLog.ui)

  # and finally this will run moc:
  QT_WRAP_CPP(euRun_MOCSrcs include/RunControlModel.hh include/euRun.hh )
  QT_WRAP_CPP(euProd_MOCSrcs include/euProd.hh )
  QT_WRAP_CPP(euLog_MOCSrcs include/LogCollectorModel.hh include/euLog.hh )

  # we need this to be able to include headers produced by uic in our
  # code (CMAKE_BINARY_DIR holds a path to the build directory, while
  # INCLUDE_DIRECTORIES() works just like INCLUDEPATH from qmake)
  include_directories(./include ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} )

  # here we instruct CMake to build "sample" executable from all of the source files
  ADD_EXECUTABLE(euRun.exe src/euRun.cc src/euRun.rc src/RunControlModel.cc ${euRun_MOCSrcs} ${euRun_UISrcs} )
  ADD_EXECUTABLE(euProd.exe src/euProd.cc src/euProd.rc ${euProd_MOCSrcs} ${euProd_UISrcs} )
  ADD_EXECUTABLE(euLog.exe src/euLog.cc src/euLog.rc src/LogCollectorModel.cc ${euLog_MOCSrcs} ${euLog_UISrcs} )

  # last thing we have to do is to tell CMake what libraries our executable needs
  TARGET_LINK_LIBRARIES(euRun.exe  EUDAQ ${EUDAQ_THREADS_LIB} ${my_qt_libraries})
  TARGET_LINK_LIBRARIES(euProd.exe EUDAQ ${EUDAQ_THREADS_LIB} ${my_qt_libraries})
  TARGET_LINK_LIBRARIES(euLog.exe  EUDAQ ${EUDAQ_THREADS_LIB} ${my_qt_libraries})

  # to disable the opening of a command window on Windows platforms, add LINK_FLAGS properties
  IF(WIN32) # Check if we are on Windows
    if(MSVC) # Check if we are using the Visual Studio compiler
# this causes linker errors on Qt5:
#      set_target_properties(euRun.exe PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
#      set_target_properties(euProd.exe PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
#      set_target_properties(euLog.exe PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
    elseif(CMAKE_COMPILER_IS_GNUCXX)
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows") # Not tested
    else()
      message(SEND_ERROR "You are using an unsupported Windows compiler! (Not MSVC or GCC)")
    endif()
  elseif(UNIX)
    # Nothing special required
  else()
    message(SEND_ERROR "You are on an unsupported platform! (Not Win32 or Unix)")
  ENDIF()

INSTALL(TARGETS euRun.exe euProd.exe euLog.exe
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

ENDIF() # check if any Qt was found
