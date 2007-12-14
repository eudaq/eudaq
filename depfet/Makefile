EXTERNAL_DEFS += EUDAQ_FUNC=__PRETTY_FUNCTION__ EUDAQ_PLATFORM=PF_$(PLATFORM)
EXTERNAL_LIBS += eudaq
EXTERNAL_LIBDIRS += ../bin
EXTERNAL_INCS += ../main/include

include ../Makefile.common

ifeq ($(PLATFORM),WIN32)
EXTERNAL_LIBS += Ws2_32
endif

default: exe

exe: $(EXE_FILES)

$(EXE_FILES): $(OBJ_FILES)

lib: $(TARGET)

all: exe

.PHONY: exe lib all default
