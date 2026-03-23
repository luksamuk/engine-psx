# TileViz Implementation Tasks

## Current Status
- ✅ Basic infrastructure (CMake, logging, Raylib window)
- ✅ VQ code removed (not used in standard PS1 TIMs)
- ✅ Endianness bugs fixed (PS1 files are LE, no bswap needed)
- ✅ TIM parser reimplemented following PS1 format spec
- ✅ CLI arguments for loading files
- ✅ Executable compiles and runs
- ✅ Tested with game assets (TILES.TIM loads correctly)

## Completed Commits

1. `f26caec` - Remove VQ compression code
2. `9fb0e30` - Fix unit tests after VQ removal
3. `fa0e2fa` - Fix endianness bugs
4. `59b1f87` - Reimplement TIM parsing following PS1 format spec
5. `64adc8c` - Add CLI arguments for loading files

## Usage

```bash
# Build
cd ~/git/engine-psx/tools/tileviz
mkdir -p build && cd build && cmake .. && make

# Run with game assets
./bin/tileviz ~/git/engine-psx/assets/levels/R0/MAP16.MAP ~/git/engine-psx/assets/levels/R0/TILES.TIM

# Controls
# F5 - Reload
# ESC - Exit
```

## Next Priority Tasks

### Phase 1: Core Improvements
- [ ] Fix MAP file parsing (currently using placeholder format)
- [ ] Test with all level assets (R0-R9)
- [ ] Add zoom/pan controls (mouse wheel, drag)
- [ ] Display tile info on hover (coordinates, index)

### Phase 2: UI Improvements
- [ ] Add tile grid overlay toggle
- [ ] Add palette viewer panel
- [ ] Add file open dialog (drag-drop or hotkey)
- [ ] Add status bar with file info

### Phase 3: Export Features
- [ ] Export TIM to PNG
- [ ] Export level map as image
- [ ] Batch export all tiles

## Technical Notes

### PS1 TIM Format
```
Offset  Size  Description
0x00    4     Magic (0x00000010)
0x04    4     Flags (bits 0-1: BPP, bit 3: has CLUT)
--- If has CLUT ---
0x08    4     CLUT block length
0x0C    2     VRAM X coordinate
0x0E    2     VRAM Y coordinate
0x10    2     Width (in 16-pixel blocks, 4-bit only)
0x12    2     Height
0x14+   N*2   Color data (RGB555)
--- Image block ---
4       4     Image block length
4       2     VRAM X coordinate
6       2     VRAM Y coordinate
8       2     Width (in words for 4/8-bit, pixels for 16-bit)
10      2     Height
12+     N     Pixel data
```

### BPP Modes
- 0 = 4-bit indexed (16 colors)
- 1 = 8-bit indexed (256 colors)
- 2 = 16-bit direct (RGB555)
- 3 = 24-bit direct (not common)

### Endianness
PS1 uses little-endian format. No byte swapping needed on x86/ARM hosts.