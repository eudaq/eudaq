INCLUDE(${PROJECT_SOURCE_DIR}/cmake/UseLATEX.cmake)

ADD_LATEX_DOCUMENT(
  src/EUDAQUserManual.tex
  INPUTS src/CompilingOnWindows.tex
         src/Contributing.tex
         src/Conversion.tex
         src/glossary.tex
         src/Installing.tex
         src/Introduction.tex
         src/OnlineMonConfig.tex
         src/OtherParts.tex
         src/preamble.tex
         src/Producers.tex
         src/Reporting.tex
         src/Running.tex
         src/SourceCode.tex
	 ../../main/exe/src/OptionExample.cxx
	 ../../conf/ExampleConfig.conf
	 ../../main/exe/src/ExampleProducer.cxx
	 ../../main/lib/src/ExampleConverterPlugin.cc
	 ../../main/exe/src/ExampleReader.cxx
  BIBFILES src/References.bib
  USE_GLOSSARY
  IMAGE_DIRS src/images
  DEFAULT_PDF
  )

INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/EUDAQUserManual.pdf
  DESTINATION doc/manual)