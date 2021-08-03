###############################################
# LaTeX target to compile the PDF user manual #
###############################################
# Information and the manual on the UseLATEX.cmake package can be found here
# https://gitlab.kitware.com/kmorel/UseLATEX
# The code resides here:
# https://gitlab.kitware.com/kmorel/UseLATEX

OPTION(EUDAQ_BUILD_MANUAL "doc/manual code documentation (requires Latex)" OFF)
if(NOT EUDAQ_BUILD_MANUAL)
  message(STATUS "doc/manual is NOT to be built (EUDAQ_BUILD_MANUAL=OFF)")
  return()
endif()
message(STATUS "doc/manual is to be built (EUDAQ_BUILD_MANUAL=ON), if Latex is found.")

FIND_PACKAGE(LATEX)

IF(LATEX_FOUND)
  INCLUDE(${PROJECT_SOURCE_DIR}/cmake/UseLATEX.cmake)

  #message(ERROR ${CMAKE_SOURCE_DIR})
  
  ADD_LATEX_DOCUMENT(
    src/EUDAQUserManual.tex
    INPUTS
    src/preamble.tex
    src/ClassicCite.bst
    src/glossary.tex
    src/License.tex
    src/Introduction.tex
    src/Installing.tex
    src/Running.tex
    src/Integration.tex
    src/Producers.tex
    src/DataCollector.tex
    src/DataConverter.tex
    src/Monitor.tex
    src/RunControl.tex
    src/NewData.tex
    src/OtherParts.tex
    src/Contributing.tex
    src/PlatformIssue.tex
    src/TTreeConverter.tex
    ../../main/lib/core/include/eudaq/Producer.hh  
    ../../main/lib/core/include/eudaq/DataCollector.hh
    ../../main/lib/core/include/eudaq/Monitor.hh
    ../../main/lib/core/include/eudaq/RunControl.hh
    ../../main/lib/core/include/eudaq/FileWriter.hh
    ../../main/lib/core/include/eudaq/FileReader.hh
    ../../main/lib/core/src/NativeFileWriter.cc
    ../../main/lib/core/src/NativeFileReader.cc
    ../../user/example/module/src/Ex0Producer.cc
    ../../user/example/module/src/Ex0RunControl.cc
    ../../user/example/module/src/Ex0Monitor.cc
    ../../user/example/module/src/Ex0TgDataCollector.cc
    ../../user/example/module/src/Ex0TsDataCollector.cc
    ../../user/example/module/src/Ex0TgTsDataCollector.cc
    ../../user/example/module/src/Ex0RawEvent2StdEventConverter.cc
    ../../user/example/module/src/Ex0RawEvent2TTreeEventConverter.cc
    ../../user/example/misc/Ex0.ini
    ../../user/example/misc/Ex0.conf
    ../../user/eudet/module/src/DirectSaveDataCollector.cc
    
    BIBFILES src/References.bib
    USE_GLOSSARY
    IMAGE_DIRS src/images
    FORCE_PDF
    )

  # overwrite the version info tex file with information from CMake/git
  CONFIGURE_FILE("${PROJECT_SOURCE_DIR}/cmake/version.cmake.tex" 
    "${CMAKE_CURRENT_BINARY_DIR}/src/version.tex" @ONLY)

  INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/EUDAQUserManual.pdf
    DESTINATION doc/manual)

ENDIF()
