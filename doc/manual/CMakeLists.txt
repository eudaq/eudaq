INCLUDE(${PROJECT_SOURCE_DIR}/cmake/UseLATEX.cmake)

ADD_LATEX_DOCUMENT(
  src/EUDAQUserManual.tex
  INPUTS src/CompilingOnWindows.tex
         src/Contributing.tex
         src/Conversion.tex
         src/glossary.tex
         src/Installing.tex
         src/Introduction.tex
         src/License.tex
         src/OnlineMonConfig.tex
         src/OtherParts.tex
         src/preamble.tex
         src/Producers.tex
         src/Reporting.tex
         src/Running.tex
         src/SourceCode.tex
	 src/ClassicCite.bst
	 ../../main/exe/src/OptionExample.cxx
	 ../../conf/ExampleConfig.conf
	 ../../main/exe/src/ExampleProducer.cxx
	 ../../main/lib/plugins/ExampleConverterPlugin.cc
	 ../../main/exe/src/ExampleReader.cxx
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
