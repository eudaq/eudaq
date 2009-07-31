SHELL=/bin/bash -O extglob
TARGETS := all main gui root tlu eudrb depfet

default: main

clean:
	@echo -n " *** Removing old executable, library, object and dependency files in " && basename `pwd`
	@rm -f *.exe lib*.{a,so,dylib,dll,lib}

$(TARGETS):
	@make -C .. $@
