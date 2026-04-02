# Makefile for Native Build Tools
# Tools for engine-psx asset conversion

CC = gcc
CXX = g++
CFLAGS = -Wall -Wextra -O2 -I./include -I./lib
CXXFLAGS = -Wall -Wextra -O2 -I./include -I./lib -I./lib/rapidxml -std=c++17
LDFLAGS = -lm

BIN_DIR = bin
OBJ_DIR = obj

COMMON_SRC = lib/minilib.c
CJSON_SRC = lib/cJSON.c
TOML_SRC = lib/toml.c
YYJSON_SRC = lib/yyjson/yyjson.c

TOOLS = cooklvl buildprl framepacker chunkgen chunkmapper cookcollision cookobj

.PHONY: all clean directories

all: directories $(patsubst %,$(BIN_DIR)/%,$(TOOLS))

directories:
	@mkdir -p $(BIN_DIR) $(OBJ_DIR)

# C tools
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

$(BIN_DIR)/cookcollision: src/cookcollision.c $(YYJSON_SRC) $(COMMON_SRC) | directories
	$(CC) $(CFLAGS) -I./lib/yyjson $^ -o $@ $(LDFLAGS)

# C++ tool - compile C parts separately
$(OBJ_DIR)/toml.o: lib/toml.c | directories
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/minilib.o: lib/minilib.c | directories
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/cookobj: src/cookobj.cpp $(OBJ_DIR)/toml.o $(OBJ_DIR)/minilib.o | directories
	$(CXX) $(CXXFLAGS) $< $(OBJ_DIR)/toml.o $(OBJ_DIR)/minilib.o -o $@ $(LDFLAGS)

clean:
	rm -rf $(BIN_DIR) $(OBJ_DIR)

.PHONY: all clean directories
