// Level viewer interface

#ifndef LEVEL_VIEWER_H
#define LEVEL_VIEWER_H

#include "tim_types.h"
#include "ps1_types.h"
#include <stdint.h>
#include <raylib.h>

typedef struct {
    uint32_t level_width;
    uint32_t level_height;
    uint16_t* tile_data;
    TIMFile* tim;
    PS1CLUT* palette;
    int selected_tile_x;
    int selected_tile_y;
    float camera_x;
    float camera_y;
    float zoom;
} LevelViewer;

// Create viewer
LevelViewer* viewer_create(void);

// Load level file
bool viewer_load_level(LevelViewer* viewer, const char* level_path, const char* texture_path);

// Update viewer state
void viewer_update(LevelViewer* viewer);

// Render level to window
void viewer_render(LevelViewer* viewer);

// Cleanup viewer
void viewer_cleanup(LevelViewer* viewer);

// Set zoom level
void viewer_set_zoom(LevelViewer* viewer, float zoom);

// Handle keyboard input
void viewer_handle_input(LevelViewer* viewer);

// Handle mouse input
void viewer_handle_mouse(LevelViewer* viewer);

#endif // LEVEL_VIEWER_H