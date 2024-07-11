export PATH           := /opt/psn00bsdk/bin:$(PATH)
export PSN00BSDK_LIBS := /opt/psn00bsdk/lib/libpsn00b

.PHONY: clean ./build/sonicengine.cue run

all: ./build/sonicengine.cue
dir: ./build

run: ./build/sonicengine.cue
	pcsx-redux-appimage -gdb -run -interpreter \
		-loadiso ./build/sonicengine.cue

./build/sonicengine.cue: ./build
	cmake --build ./build

./build:
	cmake --preset default .

clean:
	rm -rf ./build
