#include "sound.h"

SoundEffect sfx_jump   = { 0 };
SoundEffect sfx_skid   = { 0 };
SoundEffect sfx_roll   = { 0 };
SoundEffect sfx_dash   = { 0 };
SoundEffect sfx_relea  = { 0 };
SoundEffect sfx_dropd  = { 0 };
SoundEffect sfx_ring   = { 0 };
SoundEffect sfx_pop    = { 0 };
SoundEffect sfx_sprn   = { 0 };
SoundEffect sfx_chek   = { 0 };
SoundEffect sfx_death  = { 0 };
SoundEffect sfx_ringl  = { 0 };
SoundEffect sfx_shield = { 0 };
SoundEffect sfx_yea    = { 0 };
SoundEffect sfx_switch = { 0 };
SoundEffect sfx_splash = { 0 };
SoundEffect sfx_count  = { 0 };
SoundEffect sfx_bubble = { 0 };
SoundEffect sfx_sign   = { 0 };
SoundEffect sfx_bomb   = { 0 };
SoundEffect sfx_grab   = { 0 };
SoundEffect sfx_land   = { 0 };

void
sound_sfx_init()
{
    if(sfx_jump.addr == 0)   sfx_jump    = sound_load_vag("\\SFX\\JUMP.VAG;1");
    if(sfx_skid.addr == 0)   sfx_skid    = sound_load_vag("\\SFX\\SKIDDING.VAG;1");
    if(sfx_roll.addr == 0)   sfx_roll    = sound_load_vag("\\SFX\\ROLL.VAG;1");
    if(sfx_dash.addr == 0)   sfx_dash    = sound_load_vag("\\SFX\\DASH.VAG;1");
    if(sfx_relea.addr == 0)  sfx_relea   = sound_load_vag("\\SFX\\RELEA.VAG;1");
    if(sfx_dropd.addr == 0)  sfx_dropd   = sound_load_vag("\\SFX\\DROPD.VAG;1");
    if(sfx_ring.addr == 0)   sfx_ring    = sound_load_vag("\\SFX\\RING.VAG;1");
    if(sfx_pop.addr == 0)    sfx_pop     = sound_load_vag("\\SFX\\POP.VAG;1");
    if(sfx_sprn.addr == 0)   sfx_sprn    = sound_load_vag("\\SFX\\SPRN.VAG;1");
    if(sfx_chek.addr == 0)   sfx_chek    = sound_load_vag("\\SFX\\CHEK.VAG;1");
    if(sfx_death.addr == 0)  sfx_death   = sound_load_vag("\\SFX\\DEATH.VAG;1");
    if(sfx_ringl.addr == 0)  sfx_ringl   = sound_load_vag("\\SFX\\RINGLOSS.VAG;1");
    if(sfx_shield.addr == 0) sfx_shield  = sound_load_vag("\\SFX\\SHIELD.VAG;1");
    if(sfx_yea.addr == 0)    sfx_yea     = sound_load_vag("\\SFX\\YEA.VAG;1");
    if(sfx_switch.addr == 0) sfx_switch  = sound_load_vag("\\SFX\\SWITCH.VAG;1");
    if(sfx_splash.addr == 0) sfx_splash  = sound_load_vag("\\SFX\\SPLASH.VAG;1");
    if(sfx_count.addr == 0)  sfx_count   = sound_load_vag("\\SFX\\COUNT.VAG;1");
    if(sfx_bubble.addr == 0) sfx_bubble  = sound_load_vag("\\SFX\\BUBBLE.VAG;1");
    if(sfx_sign.addr == 0)   sfx_sign    = sound_load_vag("\\SFX\\SIGN.VAG;1");
    if(sfx_bomb.addr == 0)   sfx_bomb    = sound_load_vag("\\SFX\\BOMB.VAG;1");
    if(sfx_grab.addr == 0)   sfx_grab    = sound_load_vag("\\SFX\\GRAB.VAG;1");
    if(sfx_land.addr == 0)   sfx_land    = sound_load_vag("\\SFX\\LAND.VAG;1");
}
