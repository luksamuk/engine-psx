# PS1 Tile Visualization Tool - Build Instructions

## Prerequisites

### Arch Linux

```bash
sudo pacman -S cmake raylib
```

### Other Platforms

See raylib installation for your specific OS:
- Linux: Install via package manager or build from source
- macOS: `brew install raylib cmake`
- Windows: Use vcpkg: `vcpkg install raylib:x64-windows`

## Building

```bash
cd tools/tileviz
mkdir build
cd build

# Generate build files
cmake ..

# Compile
make

# Run
./bin/tileviz
```

## CMake Options

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake -DBUILD_TESTS=ON ..
```

## Troubleshooting

### Missing raylib

If you get "raylib not found" error:

```bash
# Arch Linux
sudo pacman -S raylib

# Build from source
git clone https://github.com/raysan5/raylib.git
cd raylib
make
sudo make install
```

### Compilation errors

```bash
# Clean build
make clean
cmake ..
make
```

## Development

```bash
# Install development dependencies
sudo pacman -S cmake raylib

# Run build with verbose output
make VERBOSE=1
```

## Binary Format Analysis

- **Pattern Language Files**: `tools/layouts/*.hexpat` - Provide precise format definitions for PS1 binary files
- **[Pattern Language Documentation](https://docs.werwolv.net/pattern-language/)** - Guide to understanding the .hexpat format specifications
- **[ImHex](https://github.com/WerWolv/ImHex)** - Recommended hex editor for verifying binary structure and parsing results

Use these resources when analyzing custom PS1 file formats or debugging parsing issues.