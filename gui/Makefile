SHELL=/bin/bash -O extglob

PRO_FILES := $(wildcard *.pro)
TEMP_APPS := $(PRO_FILES:%.pro=%)
APPS := $(TEMP_APPS:common=)

OBJ := tmp

OS := $(uname -s)

all: $(APPS)

clean:
	@echo -n " *** Removing old executable, library, object and dependency files in " && basename `pwd`
	@rm -rf Makefile_* $(OBJ)/*.o $(OBJ)/*.d $(OBJ)/moc_*.cpp $(OBJ)/ui_*.h

euRun: Makefile_euRun

euLog: Makefile_euLog

euProd: Makefile_euProd

test: Makefile_test

$(APPS):
	make -f Makefile_$@

Makefile_%: %.pro
	qmake -o $@ $^
	echo '$$(TARGET): ../bin/libeudaq.*' >> $@

.PHONY: all $(APPS)
