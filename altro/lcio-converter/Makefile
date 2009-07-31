EXTERNAL_DEFS += EUDAQ_FUNC=__PRETTY_FUNCTION__ EUDAQ_PLATFORM=PF_$(PLATFORM)
EXTERNAL_LIBS += eudaq lcio AltroConverterPlugin
EXTERNAL_LIBDIRS += ../../bin ${LCIO}/lib
EXTERNAL_INCS += ../../main/include ${LCIO}/include

include ../../Makefile.common

ifeq ($(PLATFORM),WIN32)
EXTERNAL_LIBS += Ws2_32
endif

default: exe

exe: $(EXE_FILES)

$(EXE_FILES): $(TARGET)

lib: $(TARGET)

all: exe

.PHONY: exe lib all default
