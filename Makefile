MAKE := make

DIRS := main root gui tlu vme eudrb

default: main

all: $(DIRS)

$(DIRS:main=): main

eudrb: vme

$(DIRS):
	$(MAKE) -C $@

clean:
	@for d in $(DIRS); do $(MAKE) -C $$d clean; done

.PHONY: default all clean $(DIRS)
