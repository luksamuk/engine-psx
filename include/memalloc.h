#ifndef MEMALLOC_H
#define MEMALLOC_H

#include <stdint.h>

typedef struct {
    uint32_t start;
    uint32_t ptr;
    uint32_t size;
} ArenaAllocator;

void  alloc_arena_init(ArenaAllocator *arena, void *start, uint32_t size);
void  alloc_arena_free(ArenaAllocator *arena);
void *alloc_arena_malloc(ArenaAllocator *arena, uint32_t size);

uint32_t alloc_arena_bytes_used(ArenaAllocator *arena);
uint32_t alloc_arena_bytes_free(ArenaAllocator *arena);

#endif
