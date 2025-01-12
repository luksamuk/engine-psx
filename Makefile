export PATH           := /opt/psn00bsdk/bin:$(PATH)
export PSN00BSDK_LIBS := /opt/psn00bsdk/lib/libpsn00b

CHD       := ./SONICXA.chd
CUESHEET  := ./build/SONICXA.cue

MAP16SRC  := $(shell ls ./assets/levels/**/map16.json)
COL16SRC  := $(shell ls ./assets/levels/**/tiles16.tsx)
MAP128SRC := $(shell ls ./assets/levels/**/tilemap128.tmx)
LVLSRC    := $(shell ls ./assets/levels/**/Z*.tmx)
MDLSRC    := $(shell ls ./assets/models/**/*.rsd)
PRLSRC    := $(shell ls ./assets/levels/**/parallax.toml)
XASRC     := $(shell ls ./assets/bgm/*.txt)
VAGSRC    := $(shell ls ./assets/sfx/*.ogg)

MAP16OUT  := $(addsuffix MAP16.MAP,$(dir $(MAP16SRC)))
COL16OUT  := $(addsuffix MAP16.COL,$(dir $(COL16SRC)))
MAP128OUT := $(addsuffix MAP128.MAP,$(dir $(COL16SRC)))
LVLOUT    := $(addsuffix .LVL,$(basename $(LVLSRC)))
OMPOUT    := $(addsuffix .OMP,$(basename $(LVLSRC)))
MDLOUT    := $(addsuffix .mdl,$(basename $(MDLSRC)))
PRLOUT    := $(addsuffix PRL.PRL,$(dir $(PRLSRC)))
XAOUT     := $(addsuffix .XA,$(basename $(XASRC)))
VAGOUT    := $(addsuffix .VAG,$(basename $(VAGSRC)))

.PHONY: clean ${CUESHEET} run configure chd cook iso elf debug cooktest purge rebuild repack packrun

# Final product is CUE+BIN files
all: iso

# Targets for producing ELF, CUE+BIN and CHD files
elf: ./build/sonic.elf
iso: ${CUESHEET}
chd: ${CHD}

# Target for running the image
run: ${CUESHEET}
	pcsx-redux-appimage \
		-run -interpreter -fastboot -stdout \
		-iso "$<"

# Target for running the image on Mednafen
run-mednafen: ${CUESHEET}
	mednafen -force_module psx "$<"

# Target for running the image on PCSX-ReARMed
run-rearmed: ${CUESHEET}
	pcsx -cdfile "$<"

run-duckstation: ${CUESHEET}
	duckstation "$<"

# Run debugger
debug:
	gdb-multiarch

# =======================================
#  Targets for executable building
# =======================================

# Target directory
./build: configure

# ELF PSX executable
./build/sonic.elf: ./build
	cd build && make sonic

# .CUE + .BIN (needs ELF and cooked assets)
${CUESHEET}: cook ./build/sonic.elf
	cd build && make iso

# .CHD file (single-file CD image)
${CHD}: ${CUESHEET}
	tochd -d . -- "$<"


# =======================================
#   Utilitary targets
# =======================================

# Create build directory and generate CMake config from preset
configure:
#	cmake --preset default .
	cmake --preset release .

# Clean build directory
clean:
	rm -rf ./build

# Clean build directory and purge cooked assets
purge: clean cleancook
	rm -rf *.chd

# Clean everything and recreate iso
rebuild: purge cook elf iso

# Clean binaries and recreate iso
repack: clean cook elf iso

# Repack and run
packrun: repack run

# =======================================
#         ASSET COOKING TARGETS
# =======================================

mdls:   $(MDLOUT)
map16:  $(MAP16OUT) $(COL16OUT)
map128: $(MAP128OUT)
lvl:    $(LVLOUT)
prl:    $(PRLOUT)
objs:   $(OMPOUT)
xa:     $(XAOUT)
vag:    $(VAGOUT)

cook: mdls map16 map128 lvl objs prl vag xa

cleancook:
	rm -rf assets/models/**/*.mdl \
	       assets/levels/**/*.COL \
	       assets/levels/**/*.MAP \
	       assets/levels/**/*.LVL \
	       assets/levels/**/*.OMP \
	       assets/levels/**/*.OTD \
	       assets/levels/**/*.PRL \
	       assets/levels/**/collision16.json \
	       assets/levels/**/tilemap128.csv \
	       assets/levels/**/tilemap128_solid.csv \
	       assets/levels/**/tilemap128_oneway.csv \
	       assets/levels/**/tilemap128_front.csv \
	       assets/sfx/*.WAV \
	       assets/sfx/*.VAG \
	       assets/bgm/*.XA \
	       assets/bgm/*.xa

# =========== Object models ===========
%.mdl: %.rsd %.ply %.mat
	./tools/convrsd/convrsd.py $<

# =========== 16x16 tile mapping ===========
# (Depends on mapping generated on Aseprite)
%/MAP16.MAP: %/map16.json
	./tools/framepacker.py --tilemap $< $@

# =========== 16x16 collision ===========
# (Depends on tiles16.tsx tile map with collision data, generated on Tiled).
%/MAP16.COL: %/tiles16.tsx
	tiled --export-tileset $< "$(dir $<)collision16.json"
	./tools/cookcollision.py "$(dir $<)collision16.json" $@
	rm "$(dir $@)collision16.json"

# =========== 128x128 tile mapping ===========
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
	rm -f "$(basename $<)_front.psxcsv"

# =========== Level maps ===========
# These maps should use a tileset generated from "128.png".
# (Depends on files such as Z1.tmx, Z2.tmx, etc., generated on Tiled)
%.LVL: %.tmx
	tiled --export-map $< "$(basename $@).psxlvl"
	./tools/cooklvl.py "$(basename $@).psxlvl" $@
	rm "$(basename $@).psxlvl"


# =========== Object level placement ===========
# (Depends on files such as Z1.tmx, Z2.tmx, etc., generated on Tiled)
%.OMP: %.tmx
	./tools/cookobj/cookobj.py $<

# =========== Level parallax data ===========
# (Depends on a specific file named parallax.toml within level directory)
%/PRL.PRL: %/parallax.toml
	./tools/buildprl/buildprl.py $<

# =========== VAG audio encoding ===========
%.VAG: %.ogg
	ffmpeg -loglevel quiet -y -i "$<" -acodec pcm_s16le -ac 1 -ar 22050 "$(basename $<).WAV"
	wav2vag "$(basename $<).WAV" "$@"
	@rm "$(basename $<).WAV"

# =========== XA audio encoding ===========
# Individual file XA songs
%.xa: %.flac
	psxavenc -f 37800 -t xa -b 4 -c 2 -F 1 -C 0 $< $@

# Dynamic rule for generating dependencies for a given .XA file.
# These dependencies are then .xa files that are generated for individual .flac songs.
define XA_RULE
$T: $(shell cat "$(T:.XA=.txt)" | awk '{print $$3}' | awk 'NF {print "./assets/bgm/"$$1}' | paste -s -d ' ')
endef
$(foreach T,$(XAOUT),$(eval $(XA_RULE)))

# Interleaved XA songs
%.XA: %.txt 
	cd $(dir $<) && xainterleave 1 $(notdir $<) $(notdir $@)
