ILCALTRO = $(HOME)/altro_daq/eudaq/readout
ILCRORC  = $(HOME)/altro_daq/eudaq/ilcrorc

TARGETLIB := AltroProducer
EXTERNAL_DEFS += EUDAQ_FUNC=__PRETTY_FUNCTION__ EUDAQ_PLATFORM=PF_$(PLATFORM)
EXTERNAL_LIBS += eudaq 
EXTERNAL_LIBDIRS += ../bin
EXTERNAL_INCS += ../main/include $(ILCALTRO) $(ILCRORC)

include ../Makefile.common

ifeq ($(PLATFORM),WIN32)
EXTERNAL_LIBS += Ws2_32
endif

default: lib

exe: $(EXE_FILES)

$(EXE_FILES): $(TARGET)

lib: $(TARGET)

all: exe

.PHONY: exe lib all default
