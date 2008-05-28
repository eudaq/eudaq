TARGETLIB := eudaq
EXTERNAL_DEFS += EUDAQ_FUNC=__PRETTY_FUNCTION__ EUDAQ_PLATFORM=PF_$(PLATFORM)
EXTERNAL_INCS += /Applications/ilcsoft/lcio/v01-09/include
EXTERNAL_LIBS += pthread #lcio
#EXTERNAL_LIBDIRS += /Applications/ilcsoft/lcio/v01-09/lib

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

