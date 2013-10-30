TARGETLIB := eudrb
EXTERNAL_DEFS += EUDAQ_FUNC=__PRETTY_FUNCTION__ EUDAQ_PLATFORM=PF_$(PLATFORM)
EXTERNAL_LIBS += vme pthread
EXTERNAL_INCS += ../vme/include
EXTERNAL_DEFS += VME_TRACE=0

#ZESTSC1DIR ?= ../../ZestSC1
#TLUDIR ?= ../../tlu
EXTERNAL_LIBS += eudaq 
EXTERNAL_LIBDIRS += ../bin
EXTERNAL_INCS += ../main/include


include ../Makefile.common

ifeq ($(PLATFORM),WIN32)
EXTERNAL_LIBS += Ws2_32
endif

default: exe

exe: $(EXE_FILES)

$(EXE_FILES): $(TARGET)

lib: $(TARGET)

doc:
	doxygen doc/Doxyfile

pdf: doc
	make -C doc/latex

all: default pdf

.PHONY: exe lib doc pdf all default

