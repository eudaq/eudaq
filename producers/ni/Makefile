TARGETLIB := ni_lin
EXTERNAL_DEFS += VME_TRACE=0 EUDAQ_FUNC=__PRETTY_FUNCTION__ EUDAQ_PLATFORM=PF_$(PLATFORM)
EXTERNAL_INCS += ../main/include ../vme/include include
EXTERNAL_LIBS += eudaq
EXTERNAL_LIBDIRS += $(LIB)

include ../Makefile.common

default: exe

exe: $(EXE_FILES)

$(EXE_FILES): $(TARGET)

lib: $(TARGET)

.PHONY: exe lib all default

