EXTERNAL_DEFS += EUDAQ_FUNC=__PRETTY_FUNCTION__ EUDAQ_PLATFORM=PF_$(PLATFORM)

include ../Makefile.common

EXTERNAL_LIBS += eudaq Thread pthread
EXTERNAL_LIBDIRS += $(LIB)
EXTERNAL_INCS += ../main/include
EXTERNAL_CFLAGS += $(shell root-config --cflags)
EXTERNAL_LDLIBS += $(shell root-config --glibs)

default: exe

exe: $(EXE_FILES)

$(EXE_FILES): $(TARGET)

$(BIN)/Correlator.exe: tmp/radon.o

.PHONY: exe default
