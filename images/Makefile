ICONS_PNG := $(wildcard Icon_*.png)
ICONS := $(ICONS_PNG:Icon_%.png=%)
ICONS_ICO := $(ICONS:%=%.ico)
ICONS_ICNS := $(ICONS:%=%.icns)

all: $(ICONS_ICO) $(ICONS_ICNS)

V := @
ifeq ($(VERBOSE),1)
 V :=
endif

%.ico: tmp/%16.png tmp/%32.png tmp/%48.png
	@echo " ** Creating $@"
	$Vpng2ico $@ $^

%.icns: tmp/%128.png # + smaller images, + 1-bit masks (made from alpha channel?)
	@echo " ** Creating $@ (not yet implemented)"
	#$Vsips

tmp/%16.png: %.png
	$Vconvert -resize 16x16 $^ $@

tmp/%32.png: %.png
	$Vconvert -resize 32x32 $^ $@

tmp/%48.png: %.png
	$Vconvert -resize 48x48 $^ $@

tmp/%128.png: %.png
	$Vconvert -resize 128x128 $^ $@

