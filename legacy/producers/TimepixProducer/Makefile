EXTERNAL_DEFS += EUDAQ_FUNC=__PRETTY_FUNCTION__ EUDAQ_PLATFORM=PF_$(PLATFORM)
EXTERNAL_LIBS += eudaq dl Keithley gpib
EXTERNAL_LIBDIRS += ../bin $(KEITHLEYLIB)/lib /usr/local/lib
EXTERNAL_INCS += ../main/include ./include $(KEITHLEYLIB)/include
EXTERNAL_INCS += $(PIXELMAN_INSTALL)/_other_/headers

include ../Makefile.common

PIXELMAN_INSTALL:=$(TPPROD)/Pixelman_2013_09_25_x64
PIXELMAN_HWLIBS:=$(PIXELMAN_INSTALL)/hwlibs


BIN = $(TPPROD)

CFLAGS += -c -m32 -I$(PIXELMAN_INSTALL)/_other_/headers
CPPFLAGS += -c -m32
LDFLAGS +=  -m32  -L/usr/lib/i386-linux-gnu -L/usr/lib32 -L$(PIXELMAN_INSTALL) -lMpxManager -lMpxCtrl -L$(PIXELMAN_HWLIBS) -lusb -lstdc++ -ldl -lc 

ifeq ($(PLATFORM),WIN32)
EXTERNAL_LIBS += Ws2_32
endif

default: exe

exe: $(EXE_FILES)

$(EXE_FILES): $(OBJ_FILES)

lib: $(TARGET)

install : exe
	cp ../bin/TimepixProducer.exe $(PIXELMAN_INSTALL)

all: exe

.PHONY: exe lib all default
