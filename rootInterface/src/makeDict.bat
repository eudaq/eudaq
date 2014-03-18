path=C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\bin;C:\Program Files (x86)\Microsoft Visual Studio 11.0\Common7\IDE;%path%
del *SCTDict.*

%ROOTSYS%\bin\rootcint.exe SCTDict.cxx -c -p ..\inc\SCTProducer.h
pause
