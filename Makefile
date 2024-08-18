export PATH           := /opt/psn00bsdk/bin:$(PATH)
export PSN00BSDK_LIBS := /opt/psn00bsdk/lib/libpsn00b

.PHONY: clean ./build/engine.cue run configure chd cook

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

cook:
	./cook.sh

# TODO: Replace the following with ** instead of R1.
# Don't do this for now because we don't generate R0 the same way yet
clean:
	rm -rf ./build \
	       assets/levels/R1/*.COL \
	       assets/levels/R1/*.MAP \
	       assets/levels/R1/*.LVL \
	       assets/levels/R1/collision16.json \
	       assets/levels/R1/tilemap128.csv

