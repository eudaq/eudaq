EXTERNAL_DEFS += EUDAQ_FUNC=__PRETTY_FUNCTION__ EUDAQ_PLATFORM=PF_$(PLATFORM)

include ../Makefile.common

EXTERNAL_LIBS += eudaq 
EXTERNAL_LIBDIRS += ${LIB} ../bin/
EXTERNAL_INCS += ../main/include

default: exe

exe: $(EXE_FILES)

$(EXE_FILES): $(TARGET)

.PHONY: exe default
