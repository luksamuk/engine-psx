export PATH           := /opt/psn00bsdk/bin:$(PATH)
export PSN00BSDK_LIBS := /opt/psn00bsdk/lib/libpsn00b

MAP16SRC  := $(shell ls ./assets/levels/**/map16.json)
COL16SRC  := $(shell ls ./assets/levels/**/tiles16.tsx)
MAP128SRC := $(shell ls ./assets/levels/**/tilemap128.tmx)
LVLSRC    := $(shell ls ./assets/levels/**/Z*.tmx)

MAP16OUT  := $(addsuffix MAP16.MAP,$(dir $(MAP16SRC)))
COL16OUT  := $(addsuffix MAP16.COL,$(dir $(COL16SRC)))
MAP128OUT := $(addsuffix MAP128.MAP,$(dir $(COL16SRC)))
LVLOUT    := $(addsuffix .LVL,$(basename $(LVLSRC)))

.PHONY: clean ./build/engine.cue run configure chd cook cooktest purge

all: ./build/engine.cue
dir: ./build
chd: engine.chd

run: ./build/engine.cue
	pcsx-redux-appimage -gdb -run -interpreter -fastboot -stdout -iso ./build/engine.cue

./build/engine.cue: cook ./build
	cmake --build ./build

engine.chd: ./build/engine.cue
	tochd -d . -- $<

./build: configure

configure:
	cmake --preset default .

clean:
	rm -rf ./build

purge: clean cleancook

# =======================================
#         ASSET COOKING TARGETS
# =======================================

map16:  $(MAP16OUT) $(COL16OUT)
map128: $(MAP128OUT)
lvl:    $(LVLOUT)

cook: map16 map128 lvl

cleancook:
	rm -rf assets/levels/**/*.COL \
	       assets/levels/**/*.MAP \
	       assets/levels/**/*.LVL \
	       assets/levels/**/collision16.json \
	       assets/levels/**/tilemap128.csv

# 16x16 tile mapping
# (Depends on mapping generated on Aseprite)
%/MAP16.MAP: %/map16.json
	./tools/framepacker.py --tilemap $< $@

# 16x16 collision
# (Depends on tiles16.tsx tile map with collision data, generated on Tiled).
%/MAP16.COL: %/tiles16.tsx
	tiled --export-tileset $< "$(dir $<)collision16.json"
	./tools/cookcollision.py "$(dir $<)collision16.json" $@
	rm "$(dir $@)collision16.json"

# 128x128 tile mapping
# Also generates 128.png to create a 128x128 tileset (should be done manually)
# (Depends on tilemap128.tmx map generated on Tiled)
%/MAP128.MAP: %/tilemap128.tmx
	tiled --export-map $< "$(basename $<).csv"
	tmxrasterizer $< "$(dir $<)128.png"
	./tools/chunkgen.py "$(basename $<).csv" $@
	rm "$(basename $<).csv"

# Level maps
# These maps should use a tileset generated from "128.png".
# (Depends on files such as Z1.tmx, Z2.tmx, etc., generated on Tiled)
%.LVL: %.tmx
	tiled --export-map $< "$(basename $@).psxlvl"
	./tools/cooklvl.py "$(basename $@).psxlvl" $@
	rm "$(basename $@).psxlvl"


