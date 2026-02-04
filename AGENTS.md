# AGENTS.md - Agent Configuration for engine-psx

## Introduction

This file defines configurations and guidelines for code agents that will interact with the engine-psx project, a Sonic fangame for PlayStation 1.

## Project Structure

The project follows the typical structure of a C project with Makefile and CMake:

- `/src/`: Main source code in C
- `/include/`: Header files
- `/assets/`: Game resources (sprites, sounds, levels)
- `/tools/`: Development scripts and tools
- `/build/`: Build directory
- `/cmake/`: Auxiliary CMake files

## Build and Development Commands

**Default build**
```bash
make
```

**Debug mode build**
```bash
make build-debug
```

**Run on emulator**
```bash
make run
```

**Run on specific emulator (Mednafen)**
```bash
make run-mednafen
```

**Run on specific emulator (DuckStation)**
```bash
make run-duckstation
```

**Generate CHD image (single file for recording)**
```bash
make chd
```

**Clean build**
```bash
make clean
```

## Coding Style

- Language: C (with PSn00bSDK extensions)
- Style: ANSI C, following PSn00bSDK conventions
- Variable names: lowercase with underscore (e.g., game_state)
- Function names: lowercase with underscore (e.g., update_player)
- Macro names: MAIÚSCULAS_COM_SUBLINHADO (e.g., MAX_PLAYERS)

## Architecture Patterns

- Uses PSn00bSDK as base
- Modular project structure
- Focus on performance and PS1 hardware compatibility
- Use of sprites and tilemaps optimized for PS1

## Testing Guidelines

- Tests executed on PCSX-Redux, DuckStation, and Mednafen emulators
- Tests on real hardware (PlayStation SCPH-5501)
- Verification of compatibility with different PS1 versions

## Specific Tools

- Compiler: gcc-mipsel
- Build system: CMake and GNU Make
- Sprite editor: Aseprite with custom scripts
- Level editor: Tiled with custom exporters
- Audio export: wav2vag, psxavenc, xainterleave
- ISO creation: mkpsxiso
- CHD creation: tochd

## Security Guidelines

- Open source code publicly available
- No sensitive components or restricted intellectual property
- Use of Mozilla Public License 2.0

## Specific Considerations for Agents

- The project requires knowledge about PS1 hardware
- The architecture is based on PSn00bSDK
- Code written in pure C, no use of complex external libraries
- Focus on performance optimization for constrained hardware
- The build generates ISO images that can be run on emulators or burned to CD
- The 'chd' target generates a single CHD file that can be used for physical recording
