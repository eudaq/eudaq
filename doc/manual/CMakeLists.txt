###############################################
# LaTeX target to compile the PDF user manual #
###############################################
# Information and the manual on the UseLATEX.cmake package can be found here
# http://www.cmake.org/Wiki/CMakeUserUseLATEX
# The code resides here:
# https://github.com/kmorel/UseLATEX
option(BUILD_manual "Generate user's manual as PDF? (requires LaTeX)?" OFF)
IF(BUILD_manual)

  INCLUDE(${PROJECT_SOURCE_DIR}/cmake/UseLATEX.cmake)

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
    src/NewData.tex
    src/OtherParts.tex
    src/Contributing.tex
    ../../conf/ExampleConfig.conf
    ../../user/example/module/src/Ex0Producer.cc
    ../../user/example/module/src/Ex0DataCollector.cc
    ../../user/example/module/src/Ex0RawEvent2StdEventConverter.cc
    ../../producers/example/src/ExampleConverterPlugin.cc
    ../../main/exe/src/ExampleReader.cxx
    src/CompilingOnWindows.tex
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

ENDIF(BUILD_manual)
