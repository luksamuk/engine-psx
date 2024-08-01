export PATH           := /opt/psn00bsdk/bin:$(PATH)
export PSN00BSDK_LIBS := /opt/psn00bsdk/lib/libpsn00b

.PHONY: clean ./build/engine.cue run configure chd

all: ./build/engine.cue
dir: ./build
chd: engine.chd

run: ./build/engine.cue
	pcsx-redux-appimage -gdb -run -interpreter -fastboot -stdout -iso ./build/engine.cue

./build/engine.cue: ./build
	cmake --build ./build

engine.chd: ./build/engine.cue
	tochd -d . -- $<

./build: configure

configure:
	cmake --preset default .

clean:
	rm -rf ./build
