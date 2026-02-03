# PS1 Tile Visualization Tool

A standalone tool for visualizing PlayStation 1 level tiles and textures on a desktop computer.

## Overview

This tool allows you to inspect and debug PS1 level data directly from export files without needing to compile and burn a CD. It's designed for analyzing levels in their raw form on your computer.

## Features

- TIM (Texture Image Memory) format parsing
- Level map (MAP) file loading and visualization
- PS1 texture rendering
- Tile-based level inspection
- Real-time rendering at 60 FPS

## Platform Support

Linux (tested), macOS, and Windows

## Installation

### Dependencies

Make sure you have the following dependencies installed:

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
cd tools/tileviz/build/bin
./tileviz
```

### Controls

- **F5** - Reload level/texture
- **ESC** - Exit application

### Supported File Formats

- **.TIM** - PlayStation 1 texture files
- **.MAP** - PlayStation 1 level map files
- **.PRG** - PlayStation 1 program data files (in development)

## Directory Structure

```
tools/tileviz/
├── src/
│   ├── main.c          # Application entry point
│   ├── app.c           # Application state management
│   ├── renderer.c      # Raylib rendering functions
│   ├── tim_parser.c    # TIM format parser
│   ├── level_loader.c  # Level data loading
│   ├── tile_renderer.c # Tile rendering logic
│   └── util.c          # Utility functions
├── include/
│   ├── tim_types.h     # TIM format type definitions
│   ├── tim_constants.h # TIM format constants
│   ├── ps1_types.h     # PS1 level data types
│   ├── ps1_constants.h # PS1 constants
│   └── log.h           # Logging system
├── tests/
│   └── unit/           # Unit tests
├── CMakeLists.txt      # Build configuration
└── README.md           # This file
```

## Documentation

See [BUILD.md](BUILD.md) for detailed build instructions and [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md) for the comprehensive implementation plan.

## License

See [LICENSE.md](LICENSE.md) for license information.

## Project Status

**Phase 1 Complete** - Core infrastructure and TIM parser foundation established.

### Planned Features

- Level viewer with full navigation
- Tile property inspection
- Texture export to PNG
- VQ compression support
- Batch processing
- Advanced analysis tools

## Contributing

Pull requests and issues are welcome.

## Authors

Created as part of the PSX engine development project.