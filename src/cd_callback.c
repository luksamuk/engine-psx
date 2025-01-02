#include "cd_callback.h"
#include <psxcd.h>
#include <psxapi.h>
#include <psxetc.h>

extern void _xa_cd_event_callback(CdlIntrResult, uint8_t *);
/* extern void _mdec_cd_event_callback(CdlIntrResult, uint8_t *); */
/* extern void _mdec_dma_callback(void); */

void
cd_set_callbacks(PlaybackType type)
{
    EnterCriticalSection();
    switch(type) {
    case PLAYBACK_XA:
        CdReadyCallback(_xa_cd_event_callback);
        break;
    case PLAYBACK_STR:
        /* DMACallback(1, &_mdec_dma_callback); */
        /* CdReadyCallback(_mdec_cd_event_callback); */
        break;
    default: break; // ???????????
    }
    ExitCriticalSection();
}

void
cd_detach_callbacks(void)
{
    EnterCriticalSection();
    DMACallback(1, NULL);
    CdReadyCallback(NULL);
    ExitCriticalSection();
}

