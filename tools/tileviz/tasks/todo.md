# TileViz Implementation Tasks

## Current Status
- ✅ Basic infrastructure (CMake, logging, Raylib window)
- ✅ VQ code removed (not used in standard PS1 TIMs)
- ✅ Endianness bugs fixed (PS1 files are LE, no bswap needed)
- ✅ Executable compiles successfully
- ✅ Unit tests compiling (some need fixing)
- ⚠️ TIM parser needs proper implementation
- ⚠️ Need to test with actual game assets

## Next Priority Tasks

### Phase 1: Test with Real Assets
- [ ] Test TIM parsing with TILES.TIM from game assets
- [ ] Verify MAP file loading from game assets
- [ ] Check if parser reads CLUT and pixel data correctly
- [ ] Debug and fix any parsing issues

### Phase 2: UI Improvements
- [ ] Add CLI arguments for loading files
- [ ] Add keyboard navigation (arrows, +/- for zoom)
- [ ] Add mouse hover to highlight tiles
- [ ] Add tile info panel (coordinates, properties)

### Phase 3: Integration with Sonic XA
- [ ] Test with level TIM/MAP files
- [ ] Verify tile rendering matches game output

## Notes
- PS1 TIMs use standard little-endian format
- Format codes: CLUT_RAW_4BIT (0x00), CLUT_RAW_8BIT (0x01), CLUT_RAW_16BIT (0x02)
- [ ] Implement VQ codebook parsing
- [ ] Implement VQ decompression algorithm
- [ ] Add VQ support to TIM_LoadFile()
- [ ] Test with VQ-compressed TIM files

### Phase 3: UI Improvements
- [ ] Add mouse hover to highlight tiles
- [ ] Add tile info panel (coordinates, properties)
- [ ] Add file open dialog
- [ ] Add keyboard navigation (arrows, zoom)
- [ ] Add layer visibility toggle

### Phase 4: Integration with Sonic XA
- [ ] Test with actual game TIM/MAP files
- [ ] Verify tile rendering matches game output
- [ ] Add support for level collision visualization

## Notes
- VQ (Vector Quantization) compression uses codebooks to compress texture data
- PS1 TIM format codes 0x04-0x07 indicate VQ-compressed textures
- Codebook stores reference patterns, texture data stores indices