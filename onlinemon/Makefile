EXTERNAL_DEFS += EUDAQ_FUNC=__PRETTY_FUNCTION__ EUDAQ_PLATFORM=PF_$(PLATFORM)


PROFILE=0
include ../Makefile.common


EXTERNAL_LIBS += eudaq
EXTERNAL_LIBDIRS += $(LIB)
EXTERNAL_INCS += ../main/include ./include .
EXTERNAL_CFLAGS += $(shell root-config --cflags) -Wno-non-virtual-dtor
EXTERNAL_LDLIBS += $(shell root-config --glibs)


default: exe

exe: $(EXE_FILES)

$(EXE_FILES): $(TARGET)

lib: $(TARGET)

tmp/ex2aDictOnlineMon.cc: include/OnlineMonWindow.hh include/CheckEOF.hh
	rootcint  -f $@ -c -I../main/include -p  $+ 

tmp/ex2aDictOnlineMon.o: tmp/ex2aDictOnlineMon.cc
	@echo "  ** Compiling  `basename $@`"
	$V$(CXX) -o $@ $< -c $(CXXFLAGS)
	
#tmp/ex2aDictEnvMon.cc: include/GraphWindow.hh include/TGraphSet.hh 
#	rootcint  -f $@ -c -I../main/include -p  $+ 

#tmp/ex2aDictEnvMon.o: tmp/ex2aDictEnvMon.cc
#	@echo "  ** Compiling  `basename $@`"
#	$V$(CXX) -o $@ $< -c $(CXXFLAGS)

$(BIN)/OnlineMon.exe: tmp/OnlineMonWindow.o  tmp/SimpleStandardEvent.o tmp/SimpleStandardPlane.o tmp/ex2aDictOnlineMon.o tmp/OnlineMon_exe.o tmp/OnlineHistograms.o tmp/CheckEOF.o tmp/EventSanityChecker.o tmp/HitmapHistos.o tmp/CorrelationHistos.o  tmp/BaseCollection.o tmp/HitmapCollection.o tmp/CorrelationCollection.o tmp/MonitorPerformanceHistos.o tmp/MonitorPerformanceCollection.o tmp/OnlineMonConfiguration.o tmp/EUDAQMonitorHistos.o tmp/EUDAQMonitorCollection.o

# $(BIN)/EnvMon.exe: tmp/ex2aDictEnvMon.o tmp/GraphWindow.o tmp/TGraphSet.o


.PHONY: exe default
superclean:
	@echo -n " *** Removing old executable, library, object and dependency files in " && basename `pwd`
	$(V) rm -f  tmp/*.{o,d,cc}


