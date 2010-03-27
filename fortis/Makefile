EXTERNAL_DEFS += EUDAQ_FUNC=__PRETTY_FUNCTION__ EUDAQ_PLATFORM=PF_$(PLATFORM)
EXTERNAL_LIBS += eudaq
EXTERNAL_LIBDIRS += ../bin
EXTERNAL_INCS += ../main/include

include ../Makefile.common

ifneq (${USE_ROOT},0) 
  EXTERNAL_INCS += ${ROOTSYS}/include
  EXTERNAL_LDLIBS +=  $(shell root-config --glibs)
  EXTERNAL_DEFS += USE_ROOT=1
endif

ifeq ($(PLATFORM),WIN32)
EXTERNAL_LIBS += Ws2_32
endif

ifeq ($(PLATFORM),MINGW)
EXTERNAL_LIBS += Ws2_32 pthread
endif

default: exe

exe: $(EXE_FILES)

$(EXE_FILES): $(OBJ_FILES)

lib: $(TARGET)

all: exe

.PHONY: exe lib all default
