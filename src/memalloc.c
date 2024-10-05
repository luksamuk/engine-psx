#include "memalloc.h"
#include <assert.h>

ArenaAllocator scratchpad_arena;

#define SCRATCHPAD_START 0x1f800000
#define SCRATCHPAD_SIZE  1024

void
alloc_arena_init(ArenaAllocator *arena, void *start, size_t size)
{
    uintptr_t st = (uintptr_t)start;
    arena->start = st;
    arena->ptr   = arena->start;
    arena->size  = size;
}

void
alloc_arena_free(ArenaAllocator *arena)
{
    arena->ptr = arena->start;
}

void *
alloc_arena_malloc(ArenaAllocator *arena, size_t size)
{
    // Align size so we don't get unaligned memory access
    size += 8 - (size % 8);

    uintptr_t p = (uintptr_t)arena->ptr;
    uintptr_t align_diff = p % 8;
    /* printf("Alotted size so far: %d / %d, requested: %lu\n", */
    /*        alloc_arena_bytes_used(arena), alloc_arena_bytes_free(arena), */
    /*        size); */
    assert((arena->ptr + size) < (arena->start + arena->size));
    arena->ptr += size + align_diff;
    return (void *)p;
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
fastalloc_malloc(size_t size)
{
    return alloc_arena_malloc(&scratchpad_arena, size);
}
