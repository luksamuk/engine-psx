// TIM parser comprehensive tests

#include "tim_parser.h"
#include "log.h"
#include "tim_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper: create a minimal valid PS1 TIM file with CLUT
void create_test_tim_4bit(const char* filename) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        printf("Test skipped: cannot create test file\n");
        return;
    }

    // PS1 TIM format:
    // uint32 magic (0x10)
    // uint32 flags (bpp + has_clut)
    // If has_clut:
    //   uint32 clut_len
    //   uint16 vram_x, vram_y, width, height
    //   uint16[] colors
    // uint32 img_len
    // uint16 vram_x, vram_y, width, height
    // uint16[] pixel_data

    uint8_t data[512];
    memset(data, 0, 512);

    uint32_t offset = 0;

    // Magic = 0x10
    uint32_t magic = 0x00000010;
    memcpy(data + offset, &magic, 4); offset += 4;

    // Flags: 4-bit (bpp=0) + has_clut (bit 3)
    uint32_t flags = 0x08; // TIM_BPP_4BIT | TIM_FLAG_HAS_CLUT
    memcpy(data + offset, &flags, 4); offset += 4;

    // CLUT block
    uint32_t clut_len = 12 + 16 * 2; // header (12) + 16 colors * 2 bytes
    memcpy(data + offset, &clut_len, 4); offset += 4;

    // CLUT rect: vram_x, vram_y, width (colors per row), height (rows)
    uint16_t clut_x = 0, clut_y = 0, clut_w = 16, clut_h = 1;
    memcpy(data + offset, &clut_x, 2); offset += 2;
    memcpy(data + offset, &clut_y, 2); offset += 2;
    memcpy(data + offset, &clut_w, 2); offset += 2;
    memcpy(data + offset, &clut_h, 2); offset += 2;

    // CLUT colors (16 RGB555 colors)
    for (int i = 0; i < 16; i++) {
        // RGB555: bits 0-4=R, 5-9=G, 10-14=B
        uint16_t color = ((i * 2) << 10) | ((i * 4) << 5) | (i * 8);
        memcpy(data + offset, &color, 2); offset += 2;
    }

    // Image block
    uint32_t img_len = 12 + 64; // header + pixel data (4x4 pixels in 4 words)
    memcpy(data + offset, &img_len, 4); offset += 4;

    // Image rect
    uint16_t img_x = 0, img_y = 0, img_w = 1, img_h = 1; // 1 word = 4 pixels (4-bit)
    memcpy(data + offset, &img_x, 2); offset += 2;
    memcpy(data + offset, &img_y, 2); offset += 2;
    memcpy(data + offset, &img_w, 2); offset += 2;
    memcpy(data + offset, &img_h, 2); offset += 2;

    // Pixel data (dummy)
    for (int i = 0; i < 32; i++) {
        uint16_t pixel = 0x1234;
        memcpy(data + offset, &pixel, 2); offset += 2;
    }

    fwrite(data, 1, offset, file);
    fclose(file);
}

// Create 8-bit TIM with CLUT
void create_test_tim_8bit(const char* filename) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        printf("Test skipped: cannot create test file\n");
        return;
    }

    uint8_t data[1024];
    memset(data, 0, 1024);

    uint32_t offset = 0;

    // Magic
    uint32_t magic = 0x00000010;
    memcpy(data + offset, &magic, 4); offset += 4;

    // Flags: 8-bit (bpp=1) + has_clut
    uint32_t flags = 0x09; // TIM_BPP_8BIT | TIM_FLAG_HAS_CLUT
    memcpy(data + offset, &flags, 4); offset += 4;

    // CLUT block
    uint32_t clut_len = 12 + 256 * 2;
    memcpy(data + offset, &clut_len, 4); offset += 4;

    uint16_t clut_x = 0, clut_y = 0, clut_w = 256, clut_h = 1;
    memcpy(data + offset, &clut_x, 2); offset += 2;
    memcpy(data + offset, &clut_y, 2); offset += 2;
    memcpy(data + offset, &clut_w, 2); offset += 2;
    memcpy(data + offset, &clut_h, 2); offset += 2;

    // 256 colors
    for (int i = 0; i < 256; i++) {
        uint16_t color = (i << 10) | (i << 5) | i;
        memcpy(data + offset, &color, 2); offset += 2;
    }

    // Image block
    uint32_t img_len = 12 + 128;
    memcpy(data + offset, &img_len, 4); offset += 4;

    uint16_t img_x = 0, img_y = 0, img_w = 8, img_h = 8; // 8 words per row, 8 rows
    memcpy(data + offset, &img_x, 2); offset += 2;
    memcpy(data + offset, &img_y, 2); offset += 2;
    memcpy(data + offset, &img_w, 2); offset += 2;
    memcpy(data + offset, &img_h, 2); offset += 2;

    // Pixel data
    for (int i = 0; i < 64; i++) {
        uint16_t pixel = 0xABCD;
        memcpy(data + offset, &pixel, 2); offset += 2;
    }

    fwrite(data, 1, offset, file);
    fclose(file);
}

// Create 16-bit TIM (no CLUT)
void create_test_tim_16bit(const char* filename) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        printf("Test skipped: cannot create test file\n");
        return;
    }

    uint8_t data[256];
    memset(data, 0, 256);

    uint32_t offset = 0;

    // Magic
    uint32_t magic = 0x00000010;
    memcpy(data + offset, &magic, 4); offset += 4;

    // Flags: 16-bit (bpp=2), no CLUT
    uint32_t flags = 0x02;
    memcpy(data + offset, &flags, 4); offset += 4;

    // Image block directly (no CLUT)
    uint32_t img_len = 12 + 64;
    memcpy(data + offset, &img_len, 4); offset += 4;

    uint16_t img_x = 0, img_y = 0, img_w = 8, img_h = 8;
    memcpy(data + offset, &img_x, 2); offset += 2;
    memcpy(data + offset, &img_y, 2); offset += 2;
    memcpy(data + offset, &img_w, 2); offset += 2;
    memcpy(data + offset, &img_h, 2); offset += 2;

    // RGB555 pixel data
    for (int i = 0; i < 32; i++) {
        uint16_t pixel = 0x7FFF; // White
        memcpy(data + offset, &pixel, 2); offset += 2;
    }

    fwrite(data, 1, offset, file);
    fclose(file);
}

void test_TIM_ParseHeader_4bit() {
    printf("Testing 4-bit TIM with CLUT...\n");

    create_test_tim_4bit("test_tim_4bit.bin");

    TIMFile* tim = TIM_LoadFile("test_tim_4bit.bin");

    if (tim) {
        if (tim->header.magic == TIM_MAGIC) {
            printf("✓ test_TIM_4bit: valid magic (0x%08X)\n", tim->header.magic);
        } else {
            printf("✗ test_TIM_4bit: invalid magic (expected 0x%08X, got 0x%08X)\n", TIM_MAGIC, tim->header.magic);
        }

        if (tim->bpp == TIM_BPP_4BIT) {
            printf("✓ test_TIM_4bit: correct BPP mode (%d)\n", tim->bpp);
        } else {
            printf("✗ test_TIM_4bit: wrong BPP (expected %d, got %d)\n", TIM_BPP_4BIT, tim->bpp);
        }

        if (tim->has_clut) {
            printf("✓ test_TIM_4bit: CLUT flag set\n");
        } else {
            printf("✗ test_TIM_4bit: CLUT flag not set\n");
        }

        if (tim->palette && tim->palette->entry_count == 16) {
            printf("✓ test_TIM_4bit: CLUT has %d colors\n", tim->palette->entry_count);
        } else {
            printf("✗ test_TIM_4bit: CLUT missing or wrong count\n");
        }

        TIM_FreeFile(tim);
    } else {
        printf("✗ test_TIM_4bit: failed to load\n");
    }

    remove("test_tim_4bit.bin");
}

void test_TIM_ParseHeader_8bit() {
    printf("\nTesting 8-bit TIM with CLUT...\n");

    create_test_tim_8bit("test_tim_8bit.bin");

    TIMFile* tim = TIM_LoadFile("test_tim_8bit.bin");

    if (tim) {
        if (tim->bpp == TIM_BPP_8BIT) {
            printf("✓ test_TIM_8bit: correct BPP mode (%d)\n", tim->bpp);
        } else {
            printf("✗ test_TIM_8bit: wrong BPP\n");
        }

        if (tim->has_clut) {
            printf("✓ test_TIM_8bit: CLUT flag set\n");
        } else {
            printf("✗ test_TIM_8bit: CLUT flag not set\n");
        }

        if (tim->palette && tim->palette->entry_count == 256) {
            printf("✓ test_TIM_8bit: CLUT has %d colors\n", tim->palette->entry_count);
        } else {
            printf("✗ test_TIM_8bit: CLUT missing or wrong count\n");
        }

        TIM_FreeFile(tim);
    } else {
        printf("✗ test_TIM_8bit: failed to load\n");
    }

    remove("test_tim_8bit.bin");
}

void test_TIM_ParseHeader_16bit() {
    printf("\nTesting 16-bit TIM (no CLUT)...\n");

    create_test_tim_16bit("test_tim_16bit.bin");

    TIMFile* tim = TIM_LoadFile("test_tim_16bit.bin");

    if (tim) {
        if (tim->bpp == TIM_BPP_16BIT) {
            printf("✓ test_TIM_16bit: correct BPP mode (%d)\n", tim->bpp);
        } else {
            printf("✗ test_TIM_16bit: wrong BPP\n");
        }

        if (!tim->has_clut) {
            printf("✓ test_TIM_16bit: No CLUT flag (correct)\n");
        } else {
            printf("✗ test_TIM_16bit: CLUT flag incorrectly set\n");
        }

        if (tim->width == 8 && tim->height == 8) {
            printf("✓ test_TIM_16bit: dimensions %dx%d\n", tim->width, tim->height);
        } else {
            printf("✗ test_TIM_16bit: wrong dimensions %dx%d\n", tim->width, tim->height);
        }

        TIM_FreeFile(tim);
    } else {
        printf("✗ test_TIM_16bit: failed to load\n");
    }

    remove("test_tim_16bit.bin");
}

void test_TIM_InvalidFile() {
    printf("\nTesting invalid file handling...\n");

    TIMFile* tim = TIM_LoadFile("nonexistent_file.bin");
    if (!tim) {
        printf("✓ test_TIM_InvalidFile: correctly returned NULL for missing file\n");
    } else {
        printf("✗ test_TIM_InvalidFile: should return NULL\n");
        TIM_FreeFile(tim);
    }
}

void test_TIM_InvalidMagic() {
    printf("\nTesting invalid magic handling...\n");

    FILE* file = fopen("test_bad_magic.bin", "wb");
    if (!file) {
        printf("Test skipped: cannot create test file\n");
        return;
    }

    // Bad magic
    uint32_t bad_magic = 0xDEADBEEF;
    uint32_t flags = 0x02;
    fwrite(&bad_magic, 4, 1, file);
    fwrite(&flags, 4, 1, file);
    fclose(file);

    TIMFile* tim = TIM_LoadFile("test_bad_magic.bin");
    if (!tim) {
        printf("✓ test_TIM_InvalidMagic: correctly rejected bad magic\n");
    } else {
        printf("✗ test_TIM_InvalidMagic: should reject bad magic\n");
        TIM_FreeFile(tim);
    }

    remove("test_bad_magic.bin");
}

void test_TIM_RealFile() {
    printf("\nTesting real TIM file...\n");

    // Try the actual asset file
    TIMFile* tim = TIM_LoadFile("~/git/engine-psx/assets/levels/R0/TILES.TIM");
    if (tim) {
        printf("✓ test_TIM_RealFile: Loaded TILES.TIM\n");
        printf("  Magic: 0x%08X\n", tim->header.magic);
        printf("  Flags: 0x%08X\n", tim->header.flags);
        printf("  BPP: %d\n", tim->bpp);
        printf("  Has CLUT: %s\n", tim->has_clut ? "yes" : "no");
        printf("  Dimensions: %dx%d\n", tim->width, tim->height);

        if (tim->palette) {
            printf("  CLUT entries: %d\n", tim->palette->entry_count);
            if (tim->palette->entry_count > 0) {
                printf("  First color: RGB(%d,%d,%d)\n",
                       tim->palette->colors[0].r,
                       tim->palette->colors[0].g,
                       tim->palette->colors[0].b);
            }
        }

        printf("  Image rect: vram(%d,%d) %dx%d words\n",
               tim->image_rect.vram_x, tim->image_rect.vram_y,
               tim->image_rect.width, tim->image_rect.height);

        TIM_FreeFile(tim);
    } else {
        printf("Note: TILES.TIM not found (this is OK if file doesn't exist)\n");
    }
}

int main(int argc, char* argv[]) {
    log_set_level(LOG_LEVEL_DEBUG);

    printf("==============================================\n");
    printf("TIM Parser Tests (PS1 TIM Format)\n");
    printf("==============================================\n\n");

    test_TIM_ParseHeader_4bit();
    test_TIM_ParseHeader_8bit();
    test_TIM_ParseHeader_16bit();
    test_TIM_InvalidFile();
    test_TIM_InvalidMagic();
    test_TIM_RealFile();

    printf("\n==============================================\n");
    printf("All TIM parser tests completed\n");
    printf("==============================================\n");

    return 0;
}