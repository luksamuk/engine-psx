#ifndef MEMALLOC_H
#define MEMALLOC_H

#include <stdint.h>
#include <stdlib.h>

typedef struct {
    uintptr_t start;
    uintptr_t ptr;
    size_t    size;
} ArenaAllocator;

void  alloc_arena_init(ArenaAllocator *arena, void *start, uintptr_t size);
void  alloc_arena_free(ArenaAllocator *arena);
void *alloc_arena_malloc(ArenaAllocator *arena, size_t size);

uint32_t alloc_arena_bytes_used(ArenaAllocator *arena);
uint32_t alloc_arena_bytes_free(ArenaAllocator *arena);

void fastalloc_init();
void fastalloc_free();
void *fastalloc_malloc(size_t size);

#endif
