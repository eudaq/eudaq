USE_LCIO := 1
USE_EUTELESCOPE := 0

TARGETLIB := eudaq
EXTERNAL_DEFS += USE_LCIO=$(USE_LCIO) USE_EUTELESCOPE=$(USE_EUTELESCOPE)
EXTERNAL_DEFS += EUDAQ_FUNC=__PRETTY_FUNCTION__ EUDAQ_PLATFORM=PF_$(PLATFORM)
EXTERNAL_LIBS += pthread
EXTERNAL_CFLAGS += $(shell root-config --cflags)
EXTERNAL_LDLIBS += $(shell root-config --glibs)

ifneq (${USE_LCIO},0)
  EXTERNAL_INCS += ${LCIO}/include
  EXTERNAL_LIBS += lcio
  EXTERNAL_LIBDIRS += ${LCIO}/lib
endif

ifneq (${USE_EUTELESCOPE},0) 
  EXTERNAL_INCS += ${EUTELESCOPE}/include
  EXTERNAL_LIBS += Eutelescope
  EXTERNAL_LIBDIRS += ${EUTELESCOPE}/lib
endif

include ../Makefile.common

ifeq ($(PLATFORM),WIN32)
EXTERNAL_LIBS += Ws2_32
endif

ifeq ($(PLATFORM),MINGW)
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

