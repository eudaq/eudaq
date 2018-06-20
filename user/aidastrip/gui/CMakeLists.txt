##### start of customized euRun GUI ######
set(KPIX_RUN_GUI_SRC
    src/kpixGui.cxx
    src/kpixGui.cc
    src/RunControlModel.cc
    src/euRun.rc
    )
qt5_wrap_ui(kpixGui_UISrcs ui/kpixGui.ui)
qt5_wrap_cpp(kpixGui_MOCSrcs include/RunControlModel.hh include/kpixGui.hh )
list(APPEND KPIX_RUN_GUI_SRC ${kpixGui_UISrcs} ${kpixGui_MOCSrcs})
add_executable(kpixGui ${KPIX_RUN_GUI_SRC})
target_link_libraries(kpixGui ${EUDAQ_CORE_LIBRARY} ${EXTERNAL_LIB_QT})
list(APPEND INSTALL_TARGETS kpixGui)
###### end of customized euRun GUI ######
