#include "object.h"
#include "object_state.h"


// Object type enums
#define OBJ_STEGWAY (MIN_LEVEL_OBJ_GID + 0)

// Update functions
static void _stegway_update(ObjectState *, ObjectTableEntry *, VECTOR *);

void
object_update_R3(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
    switch(state->id) {
    default: break;
    case OBJ_STEGWAY: _stegway_update(state, typedata, pos); break;
    }
}

static void
_stegway_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
    (void)(typedata);
    (void)(pos);

    state->anim_state.animation = 2;
}
