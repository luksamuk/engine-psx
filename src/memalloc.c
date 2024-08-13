#include "memalloc.h"
#include <assert.h>

ArenaAllocator scratchpad_arena;

#define SCRATCHPAD_START 0x1f800000
#define SCRATCHPAD_SIZE  1024

void
alloc_arena_init(ArenaAllocator *arena, void *start, uint32_t size)
{
    arena->start = (uint32_t)start;
    arena->ptr   = arena->start;
    arena->size  = (uint32_t)size;
}

void
alloc_arena_free(ArenaAllocator *arena)
{
    arena->ptr = arena->start;
}

void *
alloc_arena_malloc(ArenaAllocator *arena, uint32_t size)
{
    void *p = (void *)arena->ptr;
    assert(arena->ptr + size < arena->start + arena->size);
    arena->ptr += size;
    return p;
}

uint32_t
alloc_arena_bytes_used(ArenaAllocator *arena)
{
    return arena->ptr - arena->start;
}

uint32_t
alloc_arena_bytes_free(ArenaAllocator *arena)
{
    return arena->size - alloc_arena_bytes_used(arena);
}

void
fastalloc_init()
{
    alloc_arena_init(&scratchpad_arena, (void*)SCRATCHPAD_START, SCRATCHPAD_SIZE);
}

void
fastalloc_free()
{
    alloc_arena_free(&scratchpad_arena);
}

void *
fastalloc_malloc(uint32_t size)
{
    alloc_arena_malloc(&scratchpad_arena, size);
}
