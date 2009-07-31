MAKE := make

DIRS := main tlu vme eudrb depfet fortis mimoroma root gui

default: main

all: $(DIRS)

$(DIRS:main=): main

eudrb: vme

$(DIRS):
	$(MAKE) -C $@

clean:
	@for d in $(DIRS) bin; do $(MAKE) -C "$$d" clean; done

.PHONY: default all clean $(DIRS)
