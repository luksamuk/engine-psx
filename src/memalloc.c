#include "memalloc.h"
#include <assert.h>

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
    arena->ptr += size;
    assert(arena->ptr < arena->start + arena->size);
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
