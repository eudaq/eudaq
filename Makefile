MAKE := make

DIRS := main ni tlu vme eudrb mvd depfet fortis mimoroma taki root gui doc onlinemon

default: main

all: $(DIRS)

$(DIRS:main=): main

eudrb mvd: vme

$(DIRS):
	$(MAKE) -C $@

clean:
	@for d in $(DIRS) bin; do $(MAKE) -C "$$d" clean; done

.PHONY: default all clean $(DIRS)
