#include "object.h"
#include "object_state.h"
#include "collision.h"

// Object type enums
#define OBJ_BALLHOG  (MIN_LEVEL_OBJ_GID + 0)

// Update functions
static void _ballhog_update(ObjectState *, ObjectTableEntry *, VECTOR *);

void
object_update_R0(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
    switch(state->id) {
    default: break;
    case OBJ_BALLHOG: _ballhog_update(state, typedata, pos); break;
    }
}

static void
_ballhog_update(ObjectState *state, ObjectTableEntry *typedata, VECTOR *pos)
{
}
