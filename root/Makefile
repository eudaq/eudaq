EXTERNAL_DEFS += EUDAQ_FUNC=__PRETTY_FUNCTION__ EUDAQ_PLATFORM=PF_$(PLATFORM)

include ../main/Makefile.common

EXTERNAL_LIBS += eudaq Thread pthread
EXTERNAL_LIBDIRS += ../main
EXTERNAL_INCS += ../main/include
EXTERNAL_CFLAGS += $(shell root-config --cflags)
EXTERNAL_LDLIBS += $(shell root-config --glibs)

default: exe

exe: $(EXE_FILES)

$(EXE_FILES): $(TARGET)

.PHONY: exe
