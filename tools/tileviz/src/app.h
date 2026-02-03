// Application interface

#ifndef APP_H
#define APP_H

#include "tim_types.h"
#include "tim_parser.h"
#include "ps1_types.h"
#include "level_loader.h"
#include "tile_renderer.h"
#include "level_viewer.h"

typedef struct {
    bool running;
    bool level_loaded;
    LevelData* level;
    TIMFile* texture;
    PS1CLUT* palette;
    TileRenderer* renderer;
    LevelViewer* viewer;
    char current_level_path[512];
    char current_texture_path[512];
} Application;

void app_init(Application* app);
void app_cleanup(Application* app);
void app_load_level(Application* app, const char* level_path, const char* texture_path);
void app_update(Application* app);
void app_render(Application* app);
void app_handle_input(Application* app);

#endif // APP_H