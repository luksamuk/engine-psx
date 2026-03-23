// TIM type structure tests

#include "tim_types.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void test_TIMFileHeader_PackedSize() {
    printf("Testing TIMFileHeader packed size...\n");
    TIMFileHeader header;
    size_t size = sizeof(TIMFileHeader);
    printf("TIMFileHeader size: %zu bytes\n", size);

    // TIMFileHeader is now 8 bytes: magic (4) + flags (4)
    if (size == 8) {
        printf("✓ TIMFileHeader has expected size (8 bytes)\n");
    } else {
        printf("✗ TIMFileHeader size incorrect: expected 8, got %zu\n", size);
    }
}

void test_PS1Color_Structure() {
    printf("\nTesting PS1Color structure...\n");
    PS1Color color;
    memset(&color, 0, sizeof(PS1Color));
    printf("PS1Color size: %zu bytes\n", sizeof(PS1Color));

    if (sizeof(PS1Color) == 3) {
        printf("✓ PS1Color has expected size (3 bytes for RGB)\n");
    } else {
        printf("✗ PS1Color size incorrect: expected 3, got %zu\n", sizeof(PS1Color));
    }
}

void test_TIMBPPCodes() {
    printf("\nTesting TIM BPP codes...\n");

    printf("TIM_BPP_4BIT  = %d (4-bit indexed, 16 colors)\n", TIM_BPP_4BIT);
    printf("TIM_BPP_8BIT  = %d (8-bit indexed, 256 colors)\n", TIM_BPP_8BIT);
    printf("TIM_BPP_16BIT = %d (16-bit direct color)\n", TIM_BPP_16BIT);
    printf("TIM_BPP_24BIT = %d (24-bit direct color)\n", TIM_BPP_24BIT);

    if (TIM_BPP_4BIT == 0 && TIM_BPP_8BIT == 1 && TIM_BPP_16BIT == 2 && TIM_BPP_24BIT == 3) {
        printf("✓ TIM BPP codes have expected values\n");
    } else {
        printf("✗ TIM BPP codes have unexpected values\n");
    }
}

void test_TIMFlags() {
    printf("\nTesting TIM flags...\n");

    printf("TIM_FLAG_HAS_CLUT = 0x%02X\n", TIM_FLAG_HAS_CLUT);

    if (TIM_FLAG_HAS_CLUT == 0x08) {
        printf("✓ TIM_FLAG_HAS_CLUT has expected value (0x08)\n");
    } else {
        printf("✗ TIM_FLAG_HAS_CLUT has unexpected value\n");
    }

    // Test helper macros
    uint32_t flags_4bit_clut = 0x08;  // 4-bit with CLUT
    uint32_t flags_8bit_clut = 0x09;  // 8-bit with CLUT
    uint32_t flags_16bit = 0x02;      // 16-bit direct (no CLUT)

    printf("Testing TIM_GET_BPP macro:\n");
    printf("  4-bit CLUT flags = 0x%02X, BPP = %d\n", flags_4bit_clut, TIM_GET_BPP(flags_4bit_clut));
    printf("  8-bit CLUT flags = 0x%02X, BPP = %d\n", flags_8bit_clut, TIM_GET_BPP(flags_8bit_clut));
    printf("  16-bit flags = 0x%02X, BPP = %d\n", flags_16bit, TIM_GET_BPP(flags_16bit));

    printf("Testing TIM_HAS_CLUT macro:\n");
    printf("  4-bit CLUT flags = 0x%02X, has_CLUT = %d\n", flags_4bit_clut, TIM_HAS_CLUT(flags_4bit_clut));
    printf("  8-bit CLUT flags = 0x%02X, has_CLUT = %d\n", flags_8bit_clut, TIM_HAS_CLUT(flags_8bit_clut));
    printf("  16-bit flags = 0x%02X, has_CLUT = %d\n", flags_16bit, TIM_HAS_CLUT(flags_16bit));
}

void test_TIMMagic() {
    printf("\nTesting TIM magic constant...\n");

    printf("TIM_MAGIC = 0x%08X\n", TIM_MAGIC);

    if (TIM_MAGIC == 0x00000010) {
        printf("✓ TIM_MAGIC has expected value (0x00000010)\n");
    } else {
        printf("✗ TIM_MAGIC has unexpected value\n");
    }
}

void test_PS1PixelFormat_Enum() {
    printf("\nTesting PS1PixelFormat enum...\n");

    PS1PixelFormat format_4bit = PS1_PIXEL_FORMAT_4BIT;
    PS1PixelFormat format_8bit = PS1_PIXEL_FORMAT_8BIT;
    PS1PixelFormat format_16bit = PS1_PIXEL_FORMAT_16BIT;

    printf("PS1_PIXEL_FORMAT_4BIT  = %d\n", format_4bit);
    printf("PS1_PIXEL_FORMAT_8BIT  = %d\n", format_8bit);
    printf("PS1_PIXEL_FORMAT_16BIT = %d\n", format_16bit);

    if (format_16bit > format_8bit && format_8bit > format_4bit) {
        printf("✓ PS1PixelFormat enum values are in ascending order\n");
    } else {
        printf("✗ PS1PixelFormat enum values not in expected order\n");
    }
}

void test_PS1CLUT_Structure() {
    printf("\nTesting PS1CLUT structure...\n");

    PS1CLUT clut;
    memset(&clut, 0, sizeof(PS1CLUT));

    printf("PS1CLUT size: %zu bytes\n", sizeof(PS1CLUT));

    if (clut.colors == NULL) {
        printf("✓ PS1CLUT.colors is NULL-initialized\n");
    } else {
        printf("✗ PS1CLUT.colors should be NULL-initialized\n");
    }

    printf("✓ PS1CLUT structure defined\n");
}

void test_TIMFile_Structure() {
    printf("\nTesting TIMFile structure...\n");

    TIMFile tim;
    memset(&tim, 0, sizeof(TIMFile));

    printf("TIMFile size: %zu bytes\n", sizeof(TIMFile));

    if (tim.palette == NULL) {
        printf("✓ TIMFile.palette is NULL-initialized\n");
    } else {
        printf("✗ TIMFile.palette should be NULL-initialized\n");
    }

    if (tim.image_data == NULL) {
        printf("✓ TIMFile.image_data is NULL-initialized\n");
    } else {
        printf("✗ TIMFile.image_data should be NULL-initialized\n");
    }

    printf("✓ TIMFile structure defined\n");
}

void test_TIMBlockHeader() {
    printf("\nTesting TIMBlockHeader structure...\n");

    TIMBlockHeader block;
    memset(&block, 0, sizeof(TIMBlockHeader));

    printf("TIMBlockHeader size: %zu bytes\n", sizeof(TIMBlockHeader));

    // TIMBlockHeader should be 8 bytes: vram_x (2) + vram_y (2) + width (2) + height (2)
    if (sizeof(TIMBlockHeader) == 8) {
        printf("✓ TIMBlockHeader has expected size (8 bytes)\n");
    } else {
        printf("✗ TIMBlockHeader size incorrect: expected 8, got %zu\n", sizeof(TIMBlockHeader));
    }
}

int main(int argc, char* argv[]) {
    log_set_level(LOG_LEVEL_DEBUG);

    printf("========================================\n");
    printf("TIM Type Structure Tests\n");
    printf("========================================\n\n");

    test_TIMFileHeader_PackedSize();
    test_PS1Color_Structure();
    test_TIMBPPCodes();
    test_TIMFlags();
    test_TIMMagic();
    test_PS1PixelFormat_Enum();
    test_PS1CLUT_Structure();
    test_TIMFile_Structure();
    test_TIMBlockHeader();

    printf("\n========================================\n");
    printf("All structure tests completed\n");
    printf("========================================\n");

    return 0;
}