// TIM parser comprehensive tests

#include "tim_parser.h"
#include "log.h"
#include "tim_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void create_test_file_16bit_clut() {
    FILE* file = fopen("test_tim_16bit.bin", "wb");
    if (!file) {
        printf("Test skipped: cannot create test file\n");
        return;
    }

    uint8_t* data = (uint8_t*)malloc(512);
    if (!data) {
        fclose(file);
        return;
    }

    memset(data, 0, 512);

    // TIM files are little-endian, but parser does bswap, so write swapped values
    data[0] = 0x4C; data[1] = 0x01;         // Signature (0x014C after bswap)
    data[2] = 0x00; data[3] = 0x00;         // Image format: 16-bit (0x0000 after bswap = TIM_FORMAT_CLUT_RAW_16BIT)
    data[4] = 0x00;                       // Palette format: direct
    data[5] = 0x20;                       // 32 CLUT entries
    data[8] = 0x80; data[9] = 0x00; data[10] = 0x00; data[11] = 0x00;  // CLUT offset: 0x80
    data[12] = 0x80; data[13] = 0x00; data[14] = 0x00; data[15] = 0x00; // Image offset: 0x80

    for (int i = 0; i < 32; i++) {
        data[16 + i * 4 + 0] = 0xFF;
        data[16 + i * 4 + 1] = (i * 4 + 1) & 0xFF;
        data[16 + i * 4 + 2] = (i * 4 + 2) & 0xFF;
        data[16 + i * 4 + 3] = (i * 4 + 3) & 0xFF;
    }

    fwrite(data, 1, 512, file);
    fclose(file);
    free(data);
}

void create_test_file_4bit_clut() {
    FILE* file = fopen("test_tim_4bit.bin", "wb");
    if (!file) {
        printf("Test skipped: cannot create test file\n");
        return;
    }

    uint8_t* data = (uint8_t*)malloc(256);
    if (!data) {
        fclose(file);
        return;
    }

    memset(data, 0, 256);

    data[0] = 0x4C; data[1] = 0x01;         // Signature
    data[2] = 0x03; data[3] = 0x00;         // Image format: 4-bit
    data[4] = 0x00;                       // Palette format: direct
    data[5] = 0x10;                       // 16 CLUT entries
    data[8] = 0x80; data[9] = 0x00; data[10] = 0x00; data[11] = 0x00;  // CLUT offset: 0x80
    data[12] = 0xC0; data[13] = 0x00; data[14] = 0x00; data[15] = 0x00; // Image offset: 0xC0

    for (int i = 0; i < 16; i++) {
        data[16 + i * 4 + 0] = 0xFF;
        data[16 + i * 4 + 1] = i & 0xFF;
        data[16 + i * 4 + 2] = (i * 2) & 0xFF;
        data[16 + i * 4 + 3] = (i * 3) & 0xFF;
    }

    fwrite(data, 1, 256, file);
    fclose(file);
    free(data);
}

void create_test_file_8bit_clut() {
    FILE* file = fopen("test_tim_8bit.bin", "wb");
    if (!file) {
        printf("Test skipped: cannot create test file\n");
        return;
    }

    uint8_t* data = (uint8_t*)malloc(384);
    if (!data) {
        fclose(file);
        return;
    }

    memset(data, 0, 384);

    data[0] = 0x4C; data[1] = 0x01;         // Signature
    data[2] = 0x04; data[3] = 0x00;         // Image format: 8-bit
    data[4] = 0x00;                       // Palette format: direct
    data[5] = 0x80;                       // 128 CLUT entries
    data[8] = 0x80; data[9] = 0x00; data[10] = 0x00; data[11] = 0x00;  // CLUT offset: 0x80
    data[12] = 0x1E0; data[13] = 0x00; data[14] = 0x00; data[15] = 0x00; // Image offset: 0x1E0

    for (int i = 0; i < 128; i++) {
        data[16 + i * 4 + 0] = 0xFF;
        data[16 + i * 4 + 1] = i & 0xFF;
        data[16 + i * 4 + 2] = (i * 2) & 0xFF;
        data[16 + i * 4 + 3] = (i * 3) & 0xFF;
    }

    fwrite(data, 1, 384, file);
    fclose(file);
    free(data);
}

void create_test_file_1bit_clut() {
    FILE* file = fopen("test_tim_1bit.bin", "wb");
    if (!file) {
        printf("Test skipped: cannot create test file\n");
        return;
    }

    uint8_t* data = (uint8_t*)malloc(224);
    if (!data) {
        fclose(file);
        return;
    }

    memset(data, 0, 224);

    data[0] = 0x4C; data[1] = 0x01;         // Signature
    data[2] = 0x05; data[3] = 0x00;         // Image format: 1-bit
    data[4] = 0x00;                       // Palette format: direct
    data[5] = 0x08;                       // 8 CLUT entries
    data[8] = 0x80; data[9] = 0x00; data[10] = 0x00; data[11] = 0x00;  // CLUT offset: 0x80
    data[12] = 0x100; data[13] = 0x00; data[14] = 0x00; data[15] = 0x00; // Image offset: 0x100

    for (int i = 0; i < 8; i++) {
        data[16 + i * 4 + 0] = 0xFF;
        data[16 + i * 4 + 1] = i & 0xFF;
        data[16 + i * 4 + 2] = (i * 2) & 0xFF;
        data[16 + i * 4 + 3] = (i * 3) & 0xFF;
    }

    fwrite(data, 1, 224, file);
    fclose(file);
    free(data);
}

void test_TIM_ParseHeader_16bit() {
    create_test_file_16bit_clut();

    TIMFileHeader header;
    TIMFile* tim = TIM_LoadFile("test_tim_16bit.bin");

    if (tim) {
        // Use already-parsed header from TIMFile
        if (tim->header.signature == 0x014C) {
            printf("✓ test_TIM_ParseHeader_16bit: valid signature\n");
        }
        if (tim->header.image_format == TIM_FORMAT_CLUT_RAW_16BIT) {
            printf("✓ test_TIM_ParseHeader_16bit: correct image format\n");
        }
        if (tim->header.palette_format == TIM_PALETTE_DIRECT) {
            printf("✓ test_TIM_ParseHeader_16bit: correct palette format\n");
        }
        if (tim->header.clut_entries == 32) {
            printf("✓ test_TIM_ParseHeader_16bit: correct CLUT entry count\n");
        }
        TIM_FreeFile(tim);
    } else {
        printf("✗ test_TIM_ParseHeader_16bit failed\n");
    }

    remove("test_tim_16bit.bin");
}

void test_TIM_ParseHeader_4bit() {
    create_test_file_4bit_clut();

    TIMFileHeader header;
    TIMFile* tim = TIM_LoadFile("test_tim_4bit.bin");

    if (tim) {
        if (tim->header.image_format == TIM_FORMAT_CLUT_RAW_4BIT) {
            printf("✓ test_TIM_ParseHeader_4bit: correct image format\n");
        }
        if (tim->header.clut_entries == 16) {
            printf("✓ test_TIM_ParseHeader_4bit: correct CLUT entry count\n");
        }
        TIM_FreeFile(tim);
    } else {
        printf("✗ test_TIM_ParseHeader_4bit failed\n");
    }

    remove("test_tim_4bit.bin");
}

void test_TIM_ParseHeader_8bit() {
    create_test_file_8bit_clut();

    TIMFileHeader header;
    TIMFile* tim = TIM_LoadFile("test_tim_8bit.bin");

    if (tim) {
        if (tim->header.image_format == TIM_FORMAT_CLUT_RAW_8BIT) {
            printf("✓ test_TIM_ParseHeader_8bit: correct image format\n");
        }
        if (tim->header.clut_entries == 128) {
            printf("✓ test_TIM_ParseHeader_8bit: correct CLUT entry count\n");
        }
        TIM_FreeFile(tim);
    } else {
        printf("✗ test_TIM_ParseHeader_8bit failed\n");
    }

    remove("test_tim_8bit.bin");
}

void test_TIM_ParseHeader_1bit() {
    create_test_file_1bit_clut();

    TIMFileHeader header;
    TIMFile* tim = TIM_LoadFile("test_tim_1bit.bin");

    if (tim) {
        if (tim->header.image_format == TIM_FORMAT_CLUT_RAW_1BIT) {
            printf("✓ test_TIM_ParseHeader_1bit: correct image format\n");
        }
        if (tim->header.clut_entries == 8) {
            printf("✓ test_TIM_ParseHeader_1bit: correct CLUT entry count\n");
        }
        TIM_FreeFile(tim);
    } else {
        printf("✗ test_TIM_ParseHeader_1bit failed\n");
    }

    remove("test_tim_1bit.bin");
}

void test_TIM_LoadCLUT() {
    TIMFile* tim = TIM_LoadFile("test_tim_16bit.bin");
    if (!tim) {
        printf("✗ test_TIM_LoadCLUT: failed to load file\n");
        return;
    }

    FILE* file = fmemopen(tim->file_data, tim->file_size, "rb");
    if (!file) {
        printf("✗ test_TIM_LoadCLUT: failed to open memory stream\n");
        TIM_FreeFile(tim);
        return;
    }

    PS1CLUT* clut = NULL;
    if (TIM_LoadCLUT(file, 0x80, tim->header.palette_format, tim->header.clut_entries, &clut)) {
        if (clut && clut->entry_count == 32) {
            printf("✓ test_TIM_LoadCLUT: successfully loaded CLUT with 32 entries\n");
        }
        if (clut) {
            if (clut->colors) free(clut->colors);
            free(clut);
        }
    } else {
        printf("✗ test_TIM_LoadCLUT: failed to load CLUT\n");
    }

    fclose(file);
    TIM_FreeFile(tim);
}

void test_TIM_LoadFile_NULL() {
    TIMFile* tim = TIM_LoadFile("nonexistent_file.bin");
    if (!tim) {
        printf("✓ test_TIM_LoadFile_NULL: correctly returned NULL for missing file\n");
    } else {
        printf("✗ test_TIM_LoadFile_NULL: should return NULL for missing file\n");
        TIM_FreeFile(tim);
    }
}

void test_TIM_FreeFile() {
    create_test_file_16bit_clut();

    TIMFile* tim = TIM_LoadFile("test_tim_16bit.bin");
    if (tim) {
        TIM_FreeFile(tim);
        printf("✓ test_TIM_FreeFile: successfully freed TIM file\n");
    } else {
        printf("✗ test_TIM_LoadFile failed before free\n");
    }

    remove("test_tim_16bit.bin");
}

void test_TIM_InvalidSignature() {
    FILE* file = fopen("test_bad_signature.bin", "wb");
    if (!file) {
        printf("Test skipped: cannot create test file\n");
        return;
    }

    uint8_t bad_sig[] = {0x00, 0x00, 0x00, 0x00};
    fwrite(bad_sig, sizeof(bad_sig), 1, file);
    fclose(file);

    TIMFile* tim = TIM_LoadFile("test_bad_signature.bin");
    if (!tim) {
        printf("✓ test_TIM_InvalidSignature: correctly rejected invalid signature\n");
    } else {
        printf("✗ test_TIM_InvalidSignature: should reject invalid signature\n");
        TIM_FreeFile(tim);
    }

    remove("test_bad_signature.bin");
}

void test_TIM_Endianness() {
    create_test_file_4bit_clut();

    TIMFile* tim = TIM_LoadFile("test_tim_4bit.bin");
    if (tim) {
        // Header is already parsed by TIM_LoadFile
        if (tim->header.clut_offset == 0x80 && tim->header.image_offset == 0xC0) {
            printf("✓ test_TIM_Endianness: correctly handled little-endian format\n");
        }
        TIM_FreeFile(tim);
    } else {
        printf("✗ test_TIM_Endianness: failed to load file\n");
    }

    remove("test_tim_4bit.bin");
}

void test_TIM_LargeFile() {
    FILE* file = fopen("test_tim_large.bin", "wb");
    if (!file) {
        printf("Test skipped: cannot create test file\n");
        return;
    }

    uint8_t* data = (uint8_t*)malloc(1024);
    memset(data, 0, 1024);

    data[0] = 0x4C; data[1] = 0x01;         // Signature
    data[2] = 0x01; data[3] = 0x00;         // Image format
    data[4] = 0x00;                       // Palette format
    data[5] = 0x80;                       // CLUT entries
    data[8] = 0x80; data[9] = 0x00; data[10] = 0x00; data[11] = 0x00;  // CLUT offset
    data[12] = 0x400; data[13] = 0x00; data[14] = 0x00; data[15] = 0x00; // Image offset

    for (int i = 0; i < 128; i++) {
        data[16 + i * 4 + 0] = 0xFF;
        data[16 + i * 4 + 1] = i & 0xFF;
        data[16 + i * 4 + 2] = (i * 2) & 0xFF;
        data[16 + i * 4 + 3] = (i * 3) & 0xFF;
    }

    fwrite(data, 1, 1024, file);
    fclose(file);
    free(data);

    TIMFile* tim = TIM_LoadFile("test_tim_large.bin");
    if (tim) {
        printf("✓ test_TIM_LargeFile: successfully loaded large TIM file (1024 bytes)\n");
        TIM_FreeFile(tim);
    } else {
        printf("✗ test_TIM_LargeFile: failed to load large file\n");
    }

    remove("test_tim_large.bin");
}

int main(int argc, char* argv[]) {
    log_set_level(LOG_LEVEL_DEBUG);

    printf("==============================================\n");
    printf("TIM Parser Comprehensive Tests\n");
    printf("==============================================\n\n");

    test_TIM_ParseHeader_16bit();
    test_TIM_ParseHeader_4bit();
    test_TIM_ParseHeader_8bit();
    test_TIM_ParseHeader_1bit();
    test_TIM_LoadCLUT();
    test_TIM_LoadFile_NULL();
    test_TIM_FreeFile();
    test_TIM_InvalidSignature();
    test_TIM_Endianness();
    test_TIM_LargeFile();

    printf("\n==============================================\n");
    printf("All TIM parser tests completed\n");
    printf("==============================================\n");

    return 0;
}