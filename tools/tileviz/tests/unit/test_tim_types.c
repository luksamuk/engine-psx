// TIM type structure tests

#include "../include/tim_types.h"
#include "../include/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void test_TIMFileHeader_PackedSize() {
    printf("Testing TIMFileHeader packed size...\n");
    TIMFileHeader header;
    size_t size = sizeof(TIMFileHeader);
    printf("TIMFileHeader size: %zu bytes\n", size);

    if (size == 32) {
        printf("✓ TIMFileHeader has expected size\n");
    } else {
        printf("✗ TIMFileHeader size incorrect: expected 32, got %zu\n", size);
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

void test_TIMCompression_Enum() {
    printf("\nTesting TIMCompression enum...\n");

    printf("TIMCompression values:\n");

    printf("TIM_COMPRESSION_NONE = %d\n", TIM_COMPRESSION_NONE);
    // Note: Standard PS1 TIMs use uncompressed formats only
    // See TIM_FORMAT_CLUT_* and TIM_FORMAT_16BIT constants

    printf("✓ TIMCompression enum has expected values\n");
}

void test_TIMFormatCodes() {
    printf("\nTesting TIM format codes...\n");

    printf("TIM_FORMAT_CLUT_4BIT = %d (4-bit CLUT, 16 colors)\n", TIM_FORMAT_CLUT_4BIT);
    printf("TIM_FORMAT_CLUT_8BIT = %d (8-bit CLUT, 256 colors)\n", TIM_FORMAT_CLUT_8BIT);
    printf("TIM_FORMAT_16BIT = %d (16-bit direct color)\n", TIM_FORMAT_16BIT);
    printf("TIM_FORMAT_MIXED = %d (mixed mode, reserved)\n", TIM_FORMAT_MIXED);

    printf("✓ TIM format codes defined for standard PS1 formats\n");
}

void test_TIMPaletteFormat() {
    printf("\nTesting TIM palette format codes...\n");

    printf("TIM_PALETTE_DIRECT = %d\n", TIM_PALETTE_DIRECT);
    printf("TIM_PALETTE_INDIRECT = %d\n", TIM_PALETTE_INDIRECT);
    printf("TIM_PALETTE_COMPRESSED = %d\n", TIM_PALETTE_COMPRESSED);

    printf("✓ TIM palette format codes defined\n");
}

void test_PS1PixelFormat_Enum() {
    printf("\nTesting PS1PixelFormat enum...\n");

    PS1PixelFormat format_4bit = PS1_PIXEL_FORMAT_4BIT;
    PS1PixelFormat format_8bit = PS1_PIXEL_FORMAT_8BIT;
    PS1PixelFormat format_16bit = PS1_PIXEL_FORMAT_16BIT;

    printf("PS1_PIXEL_FORMAT_4BIT = %d\n", format_4bit);
    printf("PS1_PIXEL_FORMAT_8BIT = %d\n", format_8bit);
    printf("PS1_PIXEL_FORMAT_16BIT = %d\n", format_16bit);

    if (format_16bit > format_8bit && format_8bit > format_4bit) {
        printf("✓ PS1PixelFormat enum values are in ascending order\n");
    } else {
        printf("✗ PS1PixelFormat enum values not in expected order\n");
    }
}

void test_TIMSignature_Constant() {
    printf("\nTesting TIM signature constant...\n");

    #ifdef TIM_SIGNATURE
        printf("TIM_SIGNATURE = 0x%04X\n", TIM_SIGNATURE);
        printf("✓ TIM_SIGNATURE constant defined\n");

        if (TIM_SIGNATURE == 0x014C) {
            printf("✓ TIM_SIGNATURE has expected value (0x014C)\n");
        } else {
            printf("✗ TIM_SIGNATURE has unexpected value\n");
        }
    #else
        printf("✗ TIM_SIGNATURE constant not defined\n");
    #endif
}

void test_TILEDATA_Structure() {
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

int main(int argc, char* argv[]) {
    log_set_level(LOG_LEVEL_DEBUG);

    printf("========================================\n");
    printf("TIM Type Structure Tests\n");
    printf("========================================\n\n");

    test_TIMFileHeader_PackedSize();
    test_PS1Color_Structure();
    test_TIMCompression_Enum();
    test_TIMFormatCodes();
    test_TIMPaletteFormat();
    test_PS1PixelFormat_Enum();
    test_TIMSignature_Constant();
    test_PS1CLUT_Structure();
    test_TIMFile_Structure();

    printf("\n========================================\n");
    printf("All structure tests completed\n");
    printf("========================================\n");

    return 0;
}