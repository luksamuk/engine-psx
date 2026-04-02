# Dependencies for Native Tools

Optional dependencies for tools that require heavy parsing.

## cookcollision (JSON heavy)

### Option 1: yyjson (Recommended)
Single-file, ultra-fast JSON parser (zero-copy).

```bash
# Download single header
mkdir -p lib/yyjson
curl -L https://raw.githubusercontent.com/ibireme/yyjson/master/src/yyjson.c \
  -o lib/yyjson/yyjson.c
curl -L https://raw.githubusercontent.com/ibireme/yyjson/master/src/yyjson.h \
  -o lib/yyjson/yyjson.h
```

### Option 2: System Library (jansson)
```bash
# Ubuntu/Debian
sudo apt install libjansson-dev

# Arch
sudo pacman -S jansson

# Fedora
sudo dnf install jansson-devel
```

## cookobj (XML + TOML)

### rapidxml (Required)
Header-only XML parser (C++).

```bash
# Download single header
mkdir -p lib/rapidxml
curl -L http://rapidxml.sourceforge.net/rapidxml.hpp \
  -o lib/rapidxml/rapidxml.hpp
```

### toml++ (Optional - for C++ TOML)
Header-only TOML parser (C++17).

```bash
# Single header amalgamated
mkdir -p lib/tomlplusplus
curl -L https://raw.githubusercontent.com/marzer/tomlplusplus/master/toml.hpp \
  -o lib/tomlplusplus/toml.hpp
```

### OR keep tomlc99 (C)
Already included - can mix C and C++.

## Quick Install (All Recommended)

```bash
cd tools/native

# yyjson for JSON
mkdir -p lib/yyjson
curl -L https://raw.githubusercontent.com/ibireme/yyjson/master/src/yyjson.c \
  -o lib/yyjson/yyjson.c
curl -L https://raw.githubusercontent.com/ibireme/yyjson/master/src/yyjson.h \
  -o lib/yyjson/yyjson.h

# rapidxml for XML
mkdir -p lib/rapidxml
curl -L http://rapidxml.sourceforge.net/rapidxml.hpp \
  -o lib/rapidxml/rapidxml.hpp 2>/dev/null || \
curl -L https://raw.githubusercontent.com/g-truc/glm/master/contrib/rapidxml/rapidxml.hpp \
  -o lib/rapidxml/rapidxml.hpp

# Rebuild
make clean && make
```

## Build Configuration

Edit `Makefile` to choose backends:

```makefile
# For cookcollision - choose one:
JSON_BACKEND = YYJSON    # Single-file, recommended
# JSON_BACKEND = JANSSON  # System lib
# JSON_BACKEND = CJSON    # Single-file (original)

# For cookobj - always C++
CXX = g++
CXXFLAGS = -std=c++17 -O2
```

## Performance Comparison

| Parser | cookcollision Time | Memory |
|--------|-------------------|--------|
| Python + shapely | ~500ms | ~30MB |
| cJSON minimal | Fails on large files | - |
| yyjson | ~20ms | ~2MB |
| jansson | ~30ms | ~3MB |

| Parser | cookobj Time |
|--------|-------------|
| Python + BeautifulSoup | ~300ms |
| rapidxml (C++) | ~15ms |

## License Notes

- yyjson: MIT License
- rapidxml: Boost Software License
- jansson: MIT License
