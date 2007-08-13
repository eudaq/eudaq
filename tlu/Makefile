TARGETLIB := tlu
EXTERNAL_DEFS += EUDAQ_FUNC=__PRETTY_FUNCTION__ EUDAQ_PLATFORM=PF_$(PLATFORM)

include ../main/Makefile.common

ifeq ($(PLATFORM),WIN32)
  ZESTSC1DEFAULT := C:\Progra~1\Orange~1\ZestSC1
else
  ifeq ($(PLATFORM),CYGWIN)
    ZESTSC1DEFAULT := /cygdrive/c/Progra~1/Orange~1/ZestSC1
  else
    ZESTSC1DEFAULT := ../../ZestSC1
  endif
endif
ZESTSC1DIR ?= $(ZESTSC1DEFAULT)
EXTERNAL_LIBS += eudaq ZestSC1 pthread
EXTERNAL_LIBDIRS += . ${ZESTSC1DIR}/Lib /sw/lib ../main /usr/local/lib
EXTERNAL_INCS += ${ZESTSC1DIR}/Inc ../main/include
#EXTERNAL_DEFS += TLUDEBUG

ifeq ($(PLATFORM),WIN32)
  EXTERNAL_LIBS += SetupAPI Ws2_32
else
  ifeq ($(PLATFORM),CYGWIN)
    EXTERNAL_LIBS += SetupAPI Ws2_32
  else
    EXTERNAL_LIBS += usb
  endif 
endif

default: exe

exe: $(EXE_FILES)

$(EXE_FILES): $(TARGET)

lib: $(TARGET)

test:
	@echo $(PLATFORM)
	@echo $(ZESTSC1DEFAULT)
	@echo $(ZESTSC1DIR)
	@echo $(EXTERNAL_LIBS)

doc:
	doxygen doc/Doxyfile

pdf: doc
	make -C doc/latex

all: default pdf

.PHONY: exe doc pdf all

