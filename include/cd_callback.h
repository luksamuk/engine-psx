#ifndef CD_CALLBACK_H
#define CD_CALLBACK_H

typedef enum {
    PLAYBACK_XA,
    PLAYBACK_STR,
} PlaybackType;

void cd_set_callbacks(PlaybackType type);
void cd_detach_callbacks(void);

#endif