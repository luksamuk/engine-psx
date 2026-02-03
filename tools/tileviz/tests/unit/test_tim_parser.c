// TIM parser tests

#include "../include/tim_parser.h"
#include "../include/log.h"
#include <stdio.h>
#include <stdlib.h>

void test_TIM_ParseHeader_Valid() {
    FILE* file = fopen("test_tim.bin", "wb");
    if (!file) {
        printf("Test skipped: cannot create test file\n");
        return;
    }

    uint8_t test_data[] = {
        0x4C, 0x01,  // Signature: 0x014C
        0x01, 0x00,  // Image format: 16-bit CLUT
        0x00,        // Palette format: direct
        0x20,        // CLUT entries: 32
        0x00, 0x00, 0x00, 0x80,  // CLUT offset: 0x80
        0x00, 0x00, 0x00, 0x80,  // Image offset: 0x80
        0x00, 0x00, 0x00, 0x00,  // Reserved
        0xFF, 0x00, 0x00, 0x00,  // CLUT data (32-bit colors)
        0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00,
        0x00, 0x00, 0xFF, 0x00
    };

    fwrite(test_data, sizeof(test_data), 1, file);
    fclose(file);

    TIMFileHeader header;
    if (TIM_ParseHeader(file, &header)) {
        printf("✓ test_TIM_ParseHeader_Valid passed\n");
    } else {
        printf("✗ test_TIM_ParseHeader_Valid failed\n");
    }

    remove("test_tim.bin");
}

void test_TIM_FreeFile() {
    TIMFile* tim = TIM_LoadFile("test_tim.bin");
    if (tim) {
        TIM_FreeFile(tim);
        printf("✓ test_TIM_FreeFile passed\n");
    } else {
        printf("✗ test_TIM_FreeFile failed\n");
    }
}

int main(int argc, char* argv[]) {
    log_set_level(LOG_LEVEL_DEBUG);

    printf("Running TIM parser tests...\n\n");

    test_TIM_ParseHeader_Valid();
    test_TIM_FreeFile();

    return 0;
}