#ifndef SCREEN_SPRITE_TEST_H
#define SCREEN_SPRITE_TEST_H

#include "player.h"

void screen_sprite_test_load();
void screen_sprite_test_unload(void *);
void screen_sprite_test_update(void *);
void screen_sprite_test_draw(void *);

void screen_sprite_test_setcharacter(PlayerCharacter character);

#endif
