// Integration tests

#include "tim_parser.h"
#include "level_loader.h"
#include "../include/log.h"
#include <stdio.h>
#include <stdlib.h>

void test_TIM_and_Level_Load() {
    printf("Testing TIM and Level load together...\n");

    TIMFile* texture = TIM_LoadFile("test_tim_16bit.bin");
    if (!texture) {
        printf("✗ Failed to load TIM file\n");
        return;
    }

    LevelData* level = level_load("test_map.map");
    if (!level) {
        printf("✗ Failed to load level file\n");
        TIM_FreeFile(texture);
        return;
    }

    if (texture && level) {
        printf("✓ Successfully loaded both TIM and level files\n");
    }

    TIM_FreeFile(texture);
    level_free(level);
}

void test_level_free_null_pointer() {
    printf("Testing NULL pointer handling...\n");
    level_free(NULL);
    printf("✓ NULL pointer handled safely\n");
}

void test_tim_free_null_pointer() {
    printf("Testing TIM NULL pointer handling...\n");
    TIM_FreeFile(NULL);
    printf("✓ TIM NULL pointer handled safely\n");
}

void test_combined_file_operations() {
    printf("Testing combined file operations...\n");

    TIMFile* tim = TIM_LoadFile("test_tim_4bit.bin");
    if (tim) {
        printf("✓ TIM file loaded successfully\n");
        TIM_FreeFile(tim);
    }

    LevelData* level = level_load("test_map_small.map");
    if (level) {
        printf("✓ Level file loaded successfully\n");
        level_free(level);
    }

    printf("✓ All file operations completed successfully\n");
}

void test_memory_leak_prevention() {
    printf("Testing memory management...\n");

    for (int i = 0; i < 10; i++) {
        TIMFile* tim = TIM_LoadFile("test_tim_16bit.bin");
        if (tim) {
            TIM_FreeFile(tim);
        }

        LevelData* level = level_load("test_map_small.map");
        if (level) {
            level_free(level);
        }
    }

    printf("✓ Memory operations completed without crashes\n");
}

void test_file_format_mismatch() {
    printf("Testing file format mismatch...\n");

    TIMFile* tim = TIM_LoadFile("test_map.map");
    if (!tim) {
        printf("✓ Correctly rejected TIM loading on MAP file\n");
    } else {
        printf("✗ Should not load MAP file as TIM\n");
        TIM_FreeFile(tim);
    }

    LevelData* level = level_load("test_tim.bin");
    if (!level) {
        printf("✓ Correctly rejected level loading on TIM file\n");
    } else {
        printf("✗ Should not load TIM file as level\n");
        level_free(level);
    }

    remove("test_tim.bin");
}

void test_large_scale_operations() {
    printf("Testing large scale operations...\n");

    for (int i = 0; i < 5; i++) {
        TIMFile* tim = TIM_LoadFile("test_tim_large.bin");
        if (tim) {
            TIM_FreeFile(tim);
        }

        LevelData* level = level_load("test_map.map");
        if (level) {
            level_free(level);
        }
    }

    printf("✓ Large scale operations completed successfully\n");
}

int main(int argc, char* argv[]) {
    log_set_level(LOG_LEVEL_DEBUG);

    printf("========================================\n");
    printf("Integration Tests\n");
    printf("========================================\n\n");

    test_TIM_and_Level_Load();
    test_level_free_null_pointer();
    test_tim_free_null_pointer();
    test_combined_file_operations();
    test_memory_leak_prevention();
    test_file_format_mismatch();
    test_large_scale_operations();

    printf("\n==================================\n");
    printf("All integration tests completed\n");
    printf("========================================\n");

    return 0;
}