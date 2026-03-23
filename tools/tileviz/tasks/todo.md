# TileViz Implementation Tasks

## Current Status
- ✅ Basic infrastructure (CMake, logging, Raylib window)
- ✅ TIM parser for standard PS1 formats (CLUT 4-bit, 8-bit, 16-bit direct)
- ✅ Level viewer core (tile rendering)
- ✅ VQ code removed (not used in standard PS1 TIMs)
- ✅ Executable compiles successfully
- ⚠️ Tests need cleanup (but not blocking main functionality)

## Next Priority Tasks

### Phase 1: Core Functionality Improvements
- [ ] Test with actual TIM files from Sonic XA assets
- [ ] Add proper dimension calculation from TIM headers
- [ ] Implement image_data parsing from TIM files

### Phase 2: UI Improvements
- [ ] Add mouse hover to highlight tiles
- [ ] Add tile info panel (coordinates, pixel values)
- [ ] Add file open dialog via CLI args
- [ ] Add keyboard navigation (arrows, +/- for zoom)

### Phase 3: Integration with Sonic XA
- [ ] Test with level TIM/MAP files
- [ ] Add support for level collision visualization
- [ ] Verify tile rendering matches game output

## Notes
- VQ compression removed - not used in standard PS1 TIM format
- Format codes standardized: CLUT_4BIT (0x00), CLUT_8BIT (0x01), 16BIT (0x02)
- Next: test with real game assets to verify parsing works correctly
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