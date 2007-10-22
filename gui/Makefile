PRO_FILES := $(wildcard *.pro)
TEMP_APPS := $(PRO_FILES:%.pro=%)
APPS := $(TEMP_APPS:common=)

OBJ := tmp

OS := $(uname -s)

all: $(APPS)

clean:
	@echo -n " *** Removing old executable, library, object and dependency files in " && basename `pwd`
	@rm -rf $(OBJ)/*.o $(OBJ)/*.d Makefile_*

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
