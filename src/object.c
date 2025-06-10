#include <stdio.h>
#include <stdlib.h>

#include "object.h"
#include "memalloc.h"
#include "util.h"

extern ArenaAllocator _level_arena;

void
_load_animation(ObjectAnim *animation, uint8_t *bytes, uint32_t *b)
{
    animation->frames = NULL;
    animation->num_frames = get_short_be(bytes, b);
    animation->loopback   = get_byte(bytes, b);
    animation->duration   = get_byte(bytes, b);
    if(animation->num_frames > 0) {
        animation->frames = alloc_arena_malloc(
            &_level_arena,
            sizeof(ObjectAnimFrame) * animation->num_frames);

        for(uint16_t i = 0; i < animation->num_frames; i++) {
            ObjectAnimFrame *frame = &animation->frames[i];
            frame->u0 = get_byte(bytes, b);
            frame->v0 = get_byte(bytes, b);
            frame->w = get_byte(bytes, b);
            frame->h = get_byte(bytes, b);
            frame->flipmask = get_byte(bytes, b);
        }
    }
}

void
load_object_table(const char *filename, ObjectTable *tbl)
{
    uint8_t *bytes;
    uint32_t b, length;

    bytes = file_read(filename, &length);
    if(bytes == NULL) {
        printf("Error reading OTD file %s from the CD.\n", filename);
        return;
    }

    b = 0;

    tbl->is_level_specific = get_byte(bytes, &b);
    tbl->num_entries       = get_short_be(bytes, &b);

    if(tbl->num_entries == 0) goto end;

    tbl->entries = alloc_arena_malloc(
        &_level_arena,
        sizeof(ObjectTableEntry) * tbl->num_entries);

    for(uint16_t i = 0; i < tbl->num_entries; i++) {
        ObjectTableEntry *entry = &tbl->entries[i];
        entry->is_level_specific = tbl->is_level_specific;
        entry->animations = NULL;
        entry->fragment = NULL;

        // Entry ID, always sequential; discarded for now
        uint8_t _id = get_byte(bytes, &b);                          (void)(_id);
        /* printf("Load object 0x%04x\n", _id); */

        uint8_t has_fragment = get_byte(bytes, &b);
        entry->num_animations = get_short_be(bytes, &b);

        if(entry->num_animations > 0) {
            entry->animations = alloc_arena_malloc(
                &_level_arena,
                sizeof(ObjectAnim) * entry->num_animations);

            for(uint16_t j = 0; j < entry->num_animations; j++) {
                ObjectAnim *animation = &entry->animations[j];
                _load_animation(animation, bytes, &b);
            }
        }

        if(has_fragment) {
            ObjectFrag *fragment = alloc_arena_malloc(
                &_level_arena,
                sizeof(ObjectFrag));
            entry->fragment = fragment;

            fragment->offsetx = get_short_be(bytes, &b);
            fragment->offsety = get_short_be(bytes, &b);
            fragment->num_animations = get_short_be(bytes, &b);
            fragment->animations = alloc_arena_malloc(
                &_level_arena,
                sizeof(ObjectAnim) * fragment->num_animations);
            
            for(uint16_t j = 0; j < fragment->num_animations; j++) {
                ObjectAnim *animation = &fragment->animations[j];
                _load_animation(animation, bytes, &b);
            }
        }
    }

end:
    free(bytes);
    printf("Loaded %d object types.\n", tbl->num_entries);
}
