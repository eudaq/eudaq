@echo off
REM do not use backslashes here -- they tend to cause trouble
SET EUDAQSRC=C:/Users/hperrey/Desktop/eudaq
ctest -S %EUDAQSRC%\etc\tests\nightly\nightly.cmake -DReferenceDataDir=%EUDAQSRC%/data -DCTEST_CMAKE_GENERATOR="Visual Studio 12" -DCTEST_SOURCE_DIRECTORY=%EUDAQSRC% -DCTEST_BINARY_DIRECTORY=%EUDAQSRC%/build
