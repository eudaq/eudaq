SHELL=/bin/bash -O extglob

PRO_FILES := $(wildcard *.pro)
TEMP_APPS := $(PRO_FILES:%.pro=%)
APPS := $(TEMP_APPS:common=)

OBJ := tmp

all: $(APPS)

UNAME := $(shell uname)

# On OS X, tell qmake to use Makefiles, not XCode
ifneq ($(findstring Darwin,$(UNAME)),)
  QMAKEOPTS := -spec macx-g++
endif

clean:
	@echo -n " *** Removing old executable, library, object and dependency files in " && basename `pwd`
	@rm -rf Makefile_* $(OBJ)/*.o $(OBJ)/*.d $(OBJ)/moc_*.cpp $(OBJ)/ui_*.h

euRun: Makefile_euRun

euLog: Makefile_euLog

euProd: Makefile_euProd

$(APPS):
	make -f Makefile_$@

Makefile_%: %.pro
	qmake $(QMAKEOPTS) -o $@ $^
	[ -f $@ ] && echo '$$(TARGET): ../bin/libeudaq.*' >> $@

.PHONY: all clean $(APPS)
