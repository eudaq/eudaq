cmake_minimum_required(VERSION 3.1.0)

set( PROJECT_GUI_SRC ${PROJECT_SOURCE_DIR}/gui/src )
set( PROJECT_GUI_INC ${PROJECT_SOURCE_DIR}/gui/include )

find_package(Qt5Widgets REQUIRED)
set(EXTERNAL_LIB_QT Qt5::Widgets)
include_directories(${Qt5Widgets_INCLUDE_DIRS})
add_definitions(${Qt5Widgets_DEFINITIONS})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(include)
include_directories(${PROJECT_GUI_INC})

##### start of customized euRun GUI ######
set(KPIX_RUN_GUI_SRC
    src/kpixGui.cxx
    src/kpixGui.cc
    ${PROJECT_GUI_SRC}/RunControlModel.cc
    ${PROJECT_GUI_SRC}/euRun.rc
    )

qt5_wrap_ui(kpixGui_ui_moc ui/kpixGui.ui)
qt5_wrap_cpp(kpixGui_hdr_moc ${PROJECT_GUI_INC}/RunControlModel.hh include/kpixGui.hh )

list(APPEND KPIX_RUN_GUI_SRC ${kpixGui_hdr_moc} ${kpixGui_ui_moc})

add_executable(kpixGui ${KPIX_RUN_GUI_SRC})
target_link_libraries(kpixGui ${EUDAQ_CORE_LIBRARY} ${EXTERNAL_LIB_QT})
list(APPEND INSTALL_TARGETS kpixGui)


#get_filename_component(MODULE_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
#set(EUDAQ_MODULE module_${MODULE_NAME})
#add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
#target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY} ${EXTERNAL_LIB_QT})
#list(APPEND INSTALL_TARGETS ${EUDAQ_MODULE})



###### end of customized euRun GUI ######

install(TARGETS ${INSTALL_TARGETS}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
