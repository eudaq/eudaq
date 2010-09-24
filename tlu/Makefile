TARGETLIB := tlu
EXTERNAL_DEFS += EUDAQ_FUNC=__PRETTY_FUNCTION__ EUDAQ_PLATFORM=PF_$(PLATFORM)
EXTERNAL_DEFS += USB_TRACE=0 TLUDEBUG=0


include ../Makefile.common

ifeq ($(PLATFORM),WIN32)
  ZESTSC1DEFAULT := ../extern/ZestSC1/windows
  EXTERNAL_LIBS += SetupAPI Ws2_32
else
  EXTERNAL_LIBS += usb  
  ifeq ($(PLATFORM),CYGWIN)
    ZESTSC1DEFAULT := ../extern/ZestSC1/windows
    EXTERNAL_LIBS += ZestSC1 SetupAPI Ws2_32
  else
    ifeq ($(PLATFORM),LINUX)
      ZESTSC1DEFAULT := ../extern/ZestSC1/linux
    else
      ZESTSC1DEFAULT := ../extern/ZestSC1/macosx
      EXTERNAL_INCS += /sw/include
    endif
  endif
endif

ZESTSC1DIR ?= $(ZESTSC1DEFAULT)
EXTERNAL_LIBS += eudaq ZestSC1 pthread
EXTERNAL_LIBDIRS += . ${ZESTSC1DIR}/Lib /sw/lib /opt/local/lib $(LIB) /usr/local/lib
EXTERNAL_INCS += ${ZESTSC1DIR}/Inc /sw/include /opt/local/include ../main/include

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

.PHONY: exe lib doc pdf all default

