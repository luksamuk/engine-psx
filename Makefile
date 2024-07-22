export PATH           := /opt/psn00bsdk/bin:$(PATH)
export PSN00BSDK_LIBS := /opt/psn00bsdk/lib/libpsn00b

.PHONY: clean ./build/sonicengine.cue run configure chd

all: ./build/sonicengine.cue
dir: ./build
chd: sonicengine.chd

run: ./build/sonicengine.cue
	pcsx-redux-appimage -gdb -run -interpreter -fastboot -stdout -iso ./build/sonicengine.cue

./build/sonicengine.cue: ./build
	cmake --build ./build

sonicengine.chd: ./build/sonicengine.cue
	tochd -d . -- $<

./build: configure

configure:
	cmake --preset default .

clean:
	rm -rf ./build
