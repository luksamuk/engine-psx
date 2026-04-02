# Makefile for Native Build Tools
# Tools for engine-psx asset conversion

CC = gcc
CFLAGS = -Wall -Wextra -O2 -I./include -I./lib
LDFLAGS = -lm

# Output directory
BUILD_DIR = build
BIN_DIR = bin

# Common library files
COMMON_SRC = lib/minilib.c
CJSON_SRC = lib/cJSON.c
TOML_SRC = lib/tomlc99.c
YYJSON_SRC = lib/yyjson/yyjson.c

# Tools to build
TOOLS = cooklvl buildprl framepacker chunkgen chunkmapper cookcollision

.PHONY: all clean directories

all: directories $(patsubst %,$(BIN_DIR)/%,$(TOOLS))

directories:
	@mkdir -p $(BIN_DIR)

$(BIN_DIR)/cooklvl: src/cooklvl.c $(COMMON_SRC) $(CJSON_SRC) | directories
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BIN_DIR)/buildprl: src/buildprl.c $(COMMON_SRC) $(TOML_SRC) | directories
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BIN_DIR)/framepacker: src/framepacker.c $(COMMON_SRC) $(CJSON_SRC) | directories
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BIN_DIR)/chunkgen: src/chunkgen.c $(COMMON_SRC) | directories
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BIN_DIR)/chunkmapper: src/chunkmapper.c $(COMMON_SRC) $(CJSON_SRC) | directories
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# cookcollision uses yyjson for fast JSON parsing
$(BIN_DIR)/cookcollision: src/cookcollision.c $(YYJSON_SRC) $(COMMON_SRC) | directories
	$(CC) $(CFLAGS) -I./lib/yyjson $^ -o $@ $(LDFLAGS)

clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean directories
