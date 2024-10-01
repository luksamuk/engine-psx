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
OMPOUT    := $(addsuffix .OMP,$(basename $(LVLSRC)))

.PHONY: clean ./build/engine.cue run configure chd cook iso elf debug cooktest purge

# Final product is CUE+BIN files
all: iso

# Targets for producing ELF, CUE+BIN and CHD files
elf: ./build/engine.elf
iso: ./build/engine.cue
chd: engine.chd

# Target for running the image
run: ./build/engine.cue
	pcsx-redux-appimage \
		-run -interpreter -fastboot -stdout \
		-iso ./build/engine.cue

# Run PCSX-Redux emulator
emu:
	2>/dev/null 1>&2 pcsx-redux -gdb -gdb-port 3333 -run -interpreter -fastboot &

# Run debugger
debug:
	gdb-multiarch


# =======================================
#  Targets for executable building
# =======================================

# Target directory
./build: configure

# ELF PSX executable
./build/engine.elf: ./build
	cd build && make engine

# .CUE + .BIN (needs ELF and cooked assets)
./build/engine.cue: cook ./build/engine.elf
	cd build && make iso

# .CHD file (single-file CD image)
engine.chd: ./build/engine.cue
	tochd -d . -- $<


# =======================================
#   Utilitary targets
# =======================================

# Create build directory and generate CMake config from preset
configure:
	cmake --preset default .

# Clean build directory
clean:
	rm -rf ./build

# Clean build directory and purge cooked assets
purge: clean cleancook
	rm -rf *.chd

# =======================================
#         ASSET COOKING TARGETS
# =======================================

map16:  $(MAP16OUT) $(COL16OUT)
map128: $(MAP128OUT)
lvl:    $(LVLOUT)
objs:   $(OMPOUT)

cook: map16 map128 lvl objs

cleancook:
	rm -rf assets/levels/**/*.COL \
	       assets/levels/**/*.MAP \
	       assets/levels/**/*.LVL \
	       assets/levels/**/*.OMP \
	       assets/levels/**/*.OTD \
	       assets/levels/**/collision16.json \
	       assets/levels/**/tilemap128.csv \
	       assets/levels/**/tilemap128_solid.csv \
	       assets/levels/**/tilemap128_oneway.csv

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
	tiled --export-map $< "$(basename $<).psxcsv"
	tmxrasterizer $< "$(dir $<)128.png"
	./tools/chunkgen.py "$(basename $<).psxcsv" $@
	rm -f "$(basename $<).psxcsv"
	rm -f "$(basename $<)_solid.psxcsv"
	rm -f "$(basename $<)_oneway.psxcsv"
	rm -f "$(basename $<)_none.psxcsv"

# Level maps
# These maps should use a tileset generated from "128.png".
# (Depends on files such as Z1.tmx, Z2.tmx, etc., generated on Tiled)
%.LVL: %.tmx
	tiled --export-map $< "$(basename $@).psxlvl"
	./tools/cooklvl.py "$(basename $@).psxlvl" $@
	rm "$(basename $@).psxlvl"


# Object level placement
# (Depends on files such as Z1.tmx, Z2.tmx, etc., generated on Tiled)
%.OMP: %.tmx
	./tools/cookobj/cookobj.py $<
