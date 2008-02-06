TARGETLIB := vme
EXTERNAL_INCS += ../main/include ../extern/Tsi148
EXTERNAL_DEFS += EUDAQ_FUNC=__PRETTY_FUNCTION__ EUDAQ_PLATFORM=PF_$(PLATFORM)
EXTERNAL_LIBDIRS += .  ../bin
EXTERNAL_LIBS += eudaq
EXTERNAL_DEFS += VME_TRACE=0

#ZESTSC1DIR ?= ../../ZestSC1
#EXTERNAL_LIBS += eudaq 
#EXTERNAL_LIBDIRS += ../bin
#EXTERNAL_INCS += ../main/include

include ../Makefile.common
#CFLAGS   := $(CFLAGS:-ansi=)

default: exe lib

exe: $(EXE_FILES)

$(EXE_FILES): $(TARGET)

lib: $(TARGET)

all: default

.PHONY: exe lib all

