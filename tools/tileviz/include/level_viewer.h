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
    int hover_tile_x;      // Tile under mouse cursor
    int hover_tile_y;
    float camera_x;
    float camera_y;
    float zoom;
    uint32_t tile_size;    // Tile size in pixels (16 or 128)
    bool show_grid;        // Toggle grid overlay
    bool dragging;         // Middle mouse drag state
    int last_mouse_x;
    int last_mouse_y;
    uint32_t texture_width;
    uint32_t texture_height;
    Texture2D cached_texture;  // Cached texture to avoid recreation every frame
    bool texture_valid;        // Whether cached texture is valid
} LevelViewer;

// Create viewer
LevelViewer* viewer_create(void);

// Load level file
bool viewer_load_level(LevelViewer* viewer, const char* level_path, const char* texture_path);

// Update viewer state (keyboard, mouse wheel, drag)
void viewer_update(LevelViewer* viewer);

// Convert TIM to Raylib texture
Texture2D TIM_ToRaylibTexture(const TIMFile* tim);

// Get cached texture (creates if needed)
Texture2D viewer_get_texture(LevelViewer* viewer);

// Render level to window
void viewer_render(LevelViewer* viewer);

// Update hover tile based on mouse position
void viewer_update_hover(LevelViewer* viewer, float tex_x, float tex_y, float tex_w, float tex_h);

// Render UI overlay
void viewer_render_ui(LevelViewer* viewer);

// Render tooltip for hovered tile
void viewer_render_tooltip(LevelViewer* viewer);

// Handle mouse click input
void viewer_handle_input(LevelViewer* viewer);

// Cleanup viewer
void viewer_cleanup(LevelViewer* viewer);

// Set zoom level
void viewer_set_zoom(LevelViewer* viewer, float zoom);

#endif // LEVEL_VIEWER_H