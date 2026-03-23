# PS1 Tile Visualization Tool

A standalone tool for visualizing PlayStation 1 level tiles and textures on a desktop computer.

## Overview

This tool allows you to inspect and debug PS1 level data directly from export files without needing to compile and burn a CD. It's designed for analyzing levels in their raw form on your computer.

## Features

- **TIM (Texture Image Memory) format parsing** - CLUT 4-bit, 8-bit, and 16-bit direct color
- **Level map (MAP) file loading** and visualization
- **PS1 texture rendering** with Raylib
- **Color palette support** - RGB555 to RGB888 conversion
- **Real-time rendering** at 60 FPS

## Platform Support

Linux (tested), macOS, and Windows

## Installation

### Dependencies

```bash
# Arch Linux
sudo pacman -S cmake raylib
```

For other platforms, see the [build instructions](BUILD.md).

### Building from Source

```bash
cd tools/tileviz
mkdir build && cd build
cmake ..
make
```

The executable will be in `bin/tileviz`.

## Usage

```bash
# Basic usage (opens empty window)
./build/bin/tileviz

# Load level file only
./build/bin/tileviz path/to/level.MAP

# Load level with texture
./build/bin/tileviz path/to/level.MAP path/to/texture.TIM
```

### Example with Game Assets

```bash
./build/bin/tileviz ~/git/engine-psx/assets/levels/R0/MAP16.MAP ~/git/engine-psx/assets/levels/R0/TILES.TIM
```

### Controls

- **F5** - Reload level/texture
- **ESC** - Exit application

### Supported File Formats

- **.TIM** - PlayStation 1 texture files (4-bit CLUT, 8-bit CLUT, 16-bit direct)
- **.MAP** - PlayStation 1 level map files

## TIM Format Support

| Mode | BPP | Colors | Status |
|------|-----|--------|--------|
| 4-bit CLUT | 0x00 | 16 | ✓ Supported |
| 8-bit CLUT | 0x01 | 256 | ✓ Supported |
| 16-bit Direct | 0x02 | 65536 | ✓ Supported |
| 24-bit Direct | 0x03 | - | Not implemented |

## Directory Structure

```
tools/tileviz/
├── src/
│   ├── main.c          # Application entry point + CLI args
│   ├── tim_parser.c    # TIM format parser (PS1 TIM spec)
│   ├── level_loader.c  # Level MAP file loading
│   ├── level_viewer.c  # Level visualization with Raylib
│   ├── tile_renderer.c # Tile rendering logic
│   └── util.c          # Utility functions
├── include/
│   ├── tim_types.h     # TIM format types + constants
│   ├── ps1_types.h     # PS1 level data types
│   └── log.h           # Logging system
├── tests/
│   ├── unit/           # Unit tests
│   └── integration/    # Integration tests
├── CMakeLists.txt      # Build configuration
├── README.md           # This file
├── BUILD.md            # Build instructions
└── IMPLEMENTATION_PLAN.md # Detailed implementation plan
```

## Documentation

- **[BUILD.md](BUILD.md)** - Detailed build instructions
- **[IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md)** - Comprehensive implementation plan
- **[AGENTS.md](AGENTS.md)** - Instructions for AI coding agents

## Binary Format References

- **[Pattern Language Documentation](https://docs.werwolv.net/pattern-language/)** - DSL used in `tools/layouts/` for defining PS1 file format specifications
- **`tools/layouts/*.hexpat`** - Pattern definitions for MAP, COL, LVL, MDL, and other PS1 file formats
- **[ImHex](https://github.com/WerWolv/ImHex)** - Hex editor with Pattern Language support

## Project Status

**Phase 2 Complete** - TIM parsing implemented and tested with real game assets.

### Completed

- [x] Basic infrastructure (CMake, logging, Raylib window)
- [x] TIM parser for PS1 formats (4-bit, 8-bit, 16-bit)
- [x] Level MAP file loading
- [x] CLI arguments for loading files
- [x] Endianness handling (PS1 is little-endian)
- [x] Tested with Sonic XA level assets (R0-R9)

### In Progress

- [ ] Improve MAP file parsing (placeholder format currently)
- [ ] Zoom and pan controls
- [ ] Tile info panel (coordinates, properties)

### Planned Features

- Texture export to PNG
- Collision layer visualization
- Batch processing
- Advanced analysis tools

## Contributing

Pull requests and issues are welcome.

## Authors

Created as part of the PSX engine development project.