path=C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\bin;C:\Program Files (x86)\Microsoft Visual Studio 11.0\Common7\IDE;%path%
del *ROOTProducer_Dict.*

%ROOTSYS%\bin\rootcint.exe ROOTProducer_Dict.cxx -c -p ..\inc\ROOTProducer.h
pause
