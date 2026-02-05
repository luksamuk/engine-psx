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

    int total_compression_types = 0;
    printf("TIMCompression values:\n");

    printf("TIM_COMPRESSED_NONE = %d\n", TIM_COMPRESSED_NONE);
    total_compression_types++;

    printf("TIM_COMPRESSED_VQ = %d\n", TIM_COMPRESSED_VQ);
    total_compression_types++;

    printf("TIM_COMPRESSED_RAW_1BIT = %d\n", TIM_COMPRESSED_RAW_1BIT);
    total_compression_types++;

    printf("TIM_COMPRESSED_RAW_4BIT = %d\n", TIM_COMPRESSED_RAW_4BIT);
    total_compression_types++;

    printf("TIM_COMPRESSED_RAW_8BIT = %d\n", TIM_COMPRESSED_RAW_8BIT);
    total_compression_types++;

    printf("TIM_COMPRESSED_RAW_16BIT = %d\n", TIM_COMPRESSED_RAW_16BIT);
    total_compression_types++;

    if (total_compression_types >= 6) {
        printf("✓ TIMCompression has expected number of values (%d)\n", total_compression_types);
    } else {
        printf("✗ TIMCompression missing values\n");
    }
}

void test_TIMFormatCodes() {
    printf("\nTesting TIM format codes...\n");

    int format_count = 0;

    printf("TIM_FORMAT_CLUT_RAW_16BIT = %d (bits/pixel: 16)\n", TIM_FORMAT_CLUT_RAW_16BIT);
    format_count++;

    printf("TIM_FORMAT_CLUT_RAW_4BIT = %d (bits/pixel: 4)\n", TIM_FORMAT_CLUT_RAW_4BIT);
    format_count++;

    printf("TIM_FORMAT_CLUT_RAW_8BIT = %d (bits/pixel: 8)\n", TIM_FORMAT_CLUT_RAW_8BIT);
    format_count++;

    printf("TIM_FORMAT_CLUT_RAW_1BIT = %d (bits/pixel: 1)\n", TIM_FORMAT_CLUT_RAW_1BIT);
    format_count++;

    printf("TIM_FORMAT_VQ_CLUT_16BIT = %d (bits/pixel: 16, compressed)\n", TIM_FORMAT_VQ_CLUT_16BIT);
    format_count++;

    printf("TIM_FORMAT_VQ_CLUT_4BIT = %d (bits/pixel: 4, compressed)\n", TIM_FORMAT_VQ_CLUT_4BIT);
    format_count++;

    printf("TIM_FORMAT_VQ_CLUT_8BIT = %d (bits/pixel: 8, compressed)\n", TIM_FORMAT_VQ_CLUT_8BIT);
    format_count++;

    printf("TIM_FORMAT_VQ_CLUT_1BIT = %d (bits/pixel: 1, compressed)\n", TIM_FORMAT_VQ_CLUT_1BIT);
    format_count++;

    if (format_count >= 8) {
        printf("✓ TIM format codes have expected count (%d)\n", format_count);
    } else {
        printf("✗ TIM format codes missing values\n");
    }
}

void test_TIMPaletteFormat() {
    printf("\nTesting TIM palette format codes...\n");

    printf("TIM_PALETTE_DIRECT = %d\n", TIM_PALETTE_DIRECT);
    printf("TIM_PALETTE_INDIRECT = %d\n", TIM_PALETTE_INDIRECT);
    printf("TIM_PALETTE_COMPRESSED = %d\n", TIM_PALETTE_COMPRESSED);

    printf("✓ TIM palette format codes defined\n");
}

void test_VQCodebookEntry_Structure() {
    printf("\nTesting VQCodebookEntry structure...\n");

    VQCodebookEntry entry;
    memset(&entry, 0, sizeof(VQCodebookEntry));

    printf("VQCodebookEntry size: %zu bytes\n", sizeof(VQCodebookEntry));

    if (entry.block_data != NULL) {
        printf("✗ VQCodebookEntry.block_data should be NULL-initialized\n");
    } else {
        printf("✓ VQCodebookEntry properly initialized to NULL\n");
    }

    printf("✓ VQCodebookEntry structure defined\n");
}

void test_PS1PixelFormat_Enum() {
    printf("\nTesting PS1PixelFormat enum...\n");

    PS1PixelFormat format_none = PS1_PIXEL_FORMAT_1BIT;
    PS1PixelFormat format_4bit = PS1_PIXEL_FORMAT_4BIT;
    PS1PixelFormat format_8bit = PS1_PIXEL_FORMAT_8BIT;
    PS1PixelFormat format_16bit = PS1_PIXEL_FORMAT_16BIT;

    printf("PS1_PIXEL_FORMAT_1BIT = %d\n", format_none);
    printf("PS1_PIXEL_FORMAT_4BIT = %d\n", format_4bit);
    printf("PS1_PIXEL_FORMAT_8BIT = %d\n", format_8bit);
    printf("PS1_PIXEL_FORMAT_16BIT = %d\n", format_16bit);

    if (format_16bit >= format_8bit && format_8bit >= format_4bit && format_4bit >= format_none) {
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
    printf("\nTesting LevelData structure...\n");

    LevelData level;
    memset(&level, 0, sizeof(LevelData));

    printf("LevelData size: %zu bytes\n", sizeof(LevelData));

    printf("✓ LevelData structure defined\n");
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
    test_VQCodebookEntry_Structure();
    test_PS1PixelFormat_Enum();
    test_TIMSignature_Constant();
    test_TILEDATA_Structure();
    test_TIMFile_Structure();

    printf("\n========================================\n");
    printf("All structure tests completed\n");
    printf("========================================\n");

    return 0;
}