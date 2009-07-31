#the altro header files, relative to this dir they are in their 
# specific subdirs altro, fec, u2f, tools
ALTRO_INCLUDE_DIR=/usr/local/include/altro

#the alto shared libraries
ALTRO_LIB_DIR=/usr/local/lib/altro

TARGETLIB := AltroUSBProducer
EXTERNAL_DEFS += EUDAQ_FUNC=__PRETTY_FUNCTION__ EUDAQ_PLATFORM=PF_$(PLATFORM)
EXTERNAL_LIBS += eudaq ilcdaq_usb
EXTERNAL_LIBDIRS += ../bin ilcdaq_usb $(ALTRO_LIB_DIR)
EXTERNAL_INCS += ../main/include ilcdaq_usb $(ALTRO_INCLUDE_DIR)

include ../Makefile.common

ifeq ($(PLATFORM),WIN32)
EXTERNAL_LIBS += Ws2_32
endif

default: exe

exe: $(EXE_FILES)

$(EXE_FILES): $(TARGET)

lib: $(TARGET)

all: exe

.PHONY: exe lib all default
