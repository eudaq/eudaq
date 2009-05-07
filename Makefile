MAKE := make

DIRS := main tlu vme eudrb depfet fortis root gui

default: main

all: $(DIRS)

$(DIRS:main=): main

eudrb: vme

$(DIRS):
	$(MAKE) -C $@

clean:
	@for d in $(DIRS); do $(MAKE) -C $$d clean; done

.PHONY: default all clean $(DIRS)
