// Level viewer implementation with zoom, pan, and tile info

#include "level_viewer.h"
#include "tim_parser.h"
#include "level_loader.h"
#include "log.h"
#include <raylib.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#define MIN_ZOOM 0.5f
#define MAX_ZOOM 8.0f
#define ZOOM_SPEED 0.1f
#define PAN_SPEED 10.0f

LevelViewer* viewer_create(void) {
    LevelViewer* viewer = (LevelViewer*)malloc(sizeof(LevelViewer));
    if (!viewer) {
        LOG_ERROR("level_viewer.c", __LINE__, "Failed to allocate level viewer");
        return NULL;
    }

    viewer->level_width = 0;
    viewer->level_height = 0;
    viewer->tile_data = NULL;
    viewer->tim = NULL;
    viewer->palette = NULL;
    viewer->selected_tile_x = -1;
    viewer->selected_tile_y = -1;
    viewer->hover_tile_x = -1;
    viewer->hover_tile_y = -1;
    viewer->camera_x = 0;
    viewer->camera_y = 0;
    viewer->zoom = 2.0f;
    viewer->tile_size = 16;
    viewer->show_grid = true;
    viewer->dragging = false;
    viewer->last_mouse_x = 0;
    viewer->last_mouse_y = 0;
    viewer->texture_width = 0;
    viewer->texture_height = 0;
    viewer->cached_texture.id = 0;  // Invalid texture
    viewer->texture_valid = false;

    LOG_INFO("level_viewer.c", __LINE__, "Level viewer created");

    return viewer;
}

bool viewer_load_level(LevelViewer* viewer, const char* level_path, const char* texture_path) {
    if (!viewer) return false;

    LOG_INFO("level_viewer.c", __LINE__, "Loading level: %s", level_path);
    LOG_INFO("level_viewer.c", __LINE__, "Loading texture: %s", texture_path);

    // Invalidate cached texture
    if (viewer->texture_valid && viewer->cached_texture.id != 0) {
        UnloadTexture(viewer->cached_texture);
        viewer->cached_texture.id = 0;
        viewer->texture_valid = false;
    }

    // Load texture
    TIMFile* texture = TIM_LoadFile(texture_path);
    if (!texture) {
        LOG_ERROR("level_viewer.c", __LINE__, "Failed to load texture");
        return false;
    }

    // Load level
    LevelData* level = level_load(level_path);
    if (!level) {
        LOG_ERROR("level_viewer.c", __LINE__, "Failed to load level");
        if (texture->palette) {
            free(texture->palette->colors);
            free(texture->palette);
        }
        if (texture->image_data) free(texture->image_data);
        TIM_FreeFile(texture);
        return false;
    }

    // Free old data
    if (viewer->tile_data) free(viewer->tile_data);
    if (viewer->tim) {
        if (viewer->tim->image_data) free(viewer->tim->image_data);
        if (viewer->tim->palette) {
            free(viewer->tim->palette->colors);
            free(viewer->tim->palette);
        }
        TIM_FreeFile(viewer->tim);
    }

    viewer->tim = texture;
    viewer->palette = texture->palette;
    viewer->level_width = level->width_tiles;
    viewer->level_height = level->height_tiles;
    viewer->tile_size = level->tile_size;
    viewer->texture_width = texture->width;
    viewer->texture_height = texture->height;
    
    uint32_t tile_data_size = level->width_tiles * level->height_tiles * sizeof(uint16_t);
    viewer->tile_data = (uint16_t*)malloc(tile_data_size);
    if (!viewer->tile_data) {
        LOG_ERROR("level_viewer.c", __LINE__, "Failed to allocate tile data");
        TIM_FreeFile(texture);
        level_free(level);
        return false;
    }

    memcpy(viewer->tile_data, level->tile_data, tile_data_size);
    level_free(level);

    viewer->selected_tile_x = -1;
    viewer->selected_tile_y = -1;
    viewer->hover_tile_x = -1;
    viewer->hover_tile_y = -1;
    viewer->camera_x = 0;
    viewer->camera_y = 0;
    viewer->zoom = 2.0f;

    LOG_INFO("level_viewer.c", __LINE__, "Level loaded successfully: %dx%d tiles (size=%u)", 
             viewer->level_width, viewer->level_height, viewer->tile_size);

    return true;
}

void viewer_update(LevelViewer* viewer) {
    if (!viewer) return;

    // Keyboard navigation - pan with arrow keys
    float pan_speed = PAN_SPEED / viewer->zoom;
    
    if (IsKeyDown(KEY_LEFT)) {
        viewer->camera_x -= pan_speed;
    }
    if (IsKeyDown(KEY_RIGHT)) {
        viewer->camera_x += pan_speed;
    }
    if (IsKeyDown(KEY_UP)) {
        viewer->camera_y -= pan_speed;
    }
    if (IsKeyDown(KEY_DOWN)) {
        viewer->camera_y += pan_speed;
    }

    // Keyboard zoom
    if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)) {
        viewer->zoom = viewer->zoom + ZOOM_SPEED;
        if (viewer->zoom > MAX_ZOOM) viewer->zoom = MAX_ZOOM;
    }
    if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) {
        viewer->zoom = viewer->zoom - ZOOM_SPEED;
        if (viewer->zoom < MIN_ZOOM) viewer->zoom = MIN_ZOOM;
    }
    
    // Reset view
    if (IsKeyPressed(KEY_R)) {
        viewer->camera_x = 0;
        viewer->camera_y = 0;
        viewer->zoom = 2.0f;
    }
    
    // Toggle grid
    if (IsKeyPressed(KEY_G)) {
        viewer->show_grid = !viewer->show_grid;
    }

    // Mouse wheel zoom
    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        float old_zoom = viewer->zoom;
        viewer->zoom += wheel * 0.25f;
        if (viewer->zoom < MIN_ZOOM) viewer->zoom = MIN_ZOOM;
        if (viewer->zoom > MAX_ZOOM) viewer->zoom = MAX_ZOOM;
        
        // Zoom centered on mouse position
        float zoom_ratio = viewer->zoom / old_zoom;
        float mouse_x = GetMouseX() - GetScreenWidth() / 2.0f;
        float mouse_y = GetMouseY() - GetScreenHeight() / 2.0f;
        
        viewer->camera_x = mouse_x - (mouse_x - viewer->camera_x) * zoom_ratio;
        viewer->camera_y = mouse_y - (mouse_y - viewer->camera_y) * zoom_ratio;
    }

    // Middle mouse button pan (drag)
    if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) {
        viewer->dragging = true;
        viewer->last_mouse_x = GetMouseX();
        viewer->last_mouse_y = GetMouseY();
    }
    
    if (IsMouseButtonReleased(MOUSE_BUTTON_MIDDLE)) {
        viewer->dragging = false;
    }
    
    if (viewer->dragging && IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
        float dx = GetMouseX() - viewer->last_mouse_x;
        float dy = GetMouseY() - viewer->last_mouse_y;
        viewer->camera_x += dx / viewer->zoom;
        viewer->camera_y += dy / viewer->zoom;
        viewer->last_mouse_x = GetMouseX();
        viewer->last_mouse_y = GetMouseY();
    }
}

Texture2D TIM_ToRaylibTexture(const TIMFile* tim) {
    Texture2D texture = {0};
    
    if (!tim || !tim->palette || !tim->image_data) {
        LOG_ERROR("level_viewer.c", __LINE__, "Invalid TIM file for texture conversion");
        return texture;
    }

    uint32_t width = tim->width;
    uint32_t height = tim->height;
    uint8_t* src_data = tim->image_data;
    PS1CLUT* palette = tim->palette;
    
    Image image = {0};
    
    // Process based on BPP mode
    switch (tim->bpp) {
        case TIM_BPP_4BIT: {
            uint16_t img_w_words = tim->image_rect.width;
            
            image.width = width;
            image.height = height;
            image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
            image.mipmaps = 1;
            image.data = (unsigned char*)malloc(image.width * image.height * 4);
            
            Color* pixels = (Color*)image.data;
            
            for (uint32_t y = 0; y < height; y++) {
                for (uint32_t x = 0; x < width; x++) {
                    uint32_t word_x = x / 4;
                    uint32_t word_offset = (y * img_w_words + word_x);
                    uint16_t word_data = ((uint16_t*)src_data)[word_offset];
                    
                    uint32_t pixel_in_word = x % 4;
                    uint8_t shift = pixel_in_word * 4;
                    uint8_t idx = (word_data >> shift) & 0x0F;
                    
                    if (idx < palette->entry_count) {
                        PS1Color* c = &palette->colors[idx];
                        pixels[y * width + x] = (Color){c->r, c->g, c->b, 255};
                    } else {
                        pixels[y * width + x] = (Color){0, 0, 0, 255};
                    }
                }
            }
            break;
        }
        
        case TIM_BPP_8BIT: {
            uint16_t img_w_words = tim->image_rect.width;
            
            image.width = width;
            image.height = height;
            image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
            image.mipmaps = 1;
            image.data = (unsigned char*)malloc(image.width * image.height * 4);
            
            Color* pixels = (Color*)image.data;
            
            for (uint32_t y = 0; y < height; y++) {
                for (uint32_t x = 0; x < width; x++) {
                    uint32_t word_x = x / 2;
                    uint32_t word_offset = (y * img_w_words + word_x);
                    uint16_t word_data = ((uint16_t*)src_data)[word_offset];
                    
                    uint32_t pixel_in_word = x % 2;
                    uint8_t idx;
                    if (pixel_in_word == 0) {
                        idx = word_data & 0xFF;
                    } else {
                        idx = (word_data >> 8) & 0xFF;
                    }
                    
                    if (idx < palette->entry_count) {
                        PS1Color* c = &palette->colors[idx];
                        pixels[y * width + x] = (Color){c->r, c->g, c->b, 255};
                    } else {
                        pixels[y * width + x] = (Color){0, 0, 0, 255};
                    }
                }
            }
            break;
        }
        
        case TIM_BPP_16BIT: {
            image.width = width;
            image.height = height;
            image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
            image.mipmaps = 1;
            image.data = (unsigned char*)malloc(image.width * image.height * 4);
            
            Color* pixels = (Color*)image.data;
            uint16_t* src_pixels = (uint16_t*)src_data;
            
            for (uint32_t i = 0; i < width * height; i++) {
                uint16_t psx_pixel = src_pixels[i];
                uint8_t r = (psx_pixel >> 0) & 0x1F;
                uint8_t g = (psx_pixel >> 5) & 0x1F;
                uint8_t b = (psx_pixel >> 10) & 0x1F;
                
                pixels[i].r = (r << 3) | (r >> 2);
                pixels[i].g = (g << 3) | (g >> 2);
                pixels[i].b = (b << 3) | (b >> 2);
                pixels[i].a = 255;
            }
            break;
        }
        
        default:
            LOG_ERROR("level_viewer.c", __LINE__, "Unsupported TIM BPP: %d", tim->bpp);
            return texture;
    }

    texture = LoadTextureFromImage(image);
    UnloadImage(image);
    
    return texture;
}

Texture2D viewer_get_texture(LevelViewer* viewer) {
    if (!viewer) {
        Texture2D empty = {0};
        return empty;
    }
    
    // Return cached texture if valid
    if (viewer->texture_valid && viewer->cached_texture.id != 0) {
        return viewer->cached_texture;
    }
    
    // Create and cache texture
    if (viewer->tim && viewer->palette && viewer->tim->image_data) {
        viewer->cached_texture = TIM_ToRaylibTexture(viewer->tim);
        viewer->texture_valid = (viewer->cached_texture.id != 0);
    }
    
    return viewer->cached_texture;
}

void viewer_render(LevelViewer* viewer) {
    if (!viewer) return;

    int screen_width = GetScreenWidth();
    int screen_height = GetScreenHeight();

    // Dark blue-gray background
    ClearBackground((Color){30, 30, 40, 255});

    // Get cached texture
    Texture2D texture = viewer_get_texture(viewer);
    
    if (texture.id != 0) {
        // Draw the tileset texture
        float tex_x = screen_width / 2.0f - viewer->camera_x * viewer->zoom;
        float tex_y = screen_height / 2.0f - viewer->camera_y * viewer->zoom;
        float tex_w = texture.width * viewer->zoom;
        float tex_h = texture.height * viewer->zoom;
        
        Rectangle source = {0, 0, (float)texture.width, (float)texture.height};
        Rectangle dest = {tex_x, tex_y, tex_w, tex_h};
        
        DrawTexturePro(texture, source, dest, (Vector2){0, 0}, 0.0f, WHITE);
        
        // Draw grid overlay if enabled
        if (viewer->show_grid && viewer->tile_data && viewer->level_width > 0) {
            float tile_px_size = viewer->tile_size * viewer->zoom;
            
            // Draw grid lines
            for (uint32_t y = 0; y <= viewer->level_height; y++) {
                float line_y = tex_y + y * tile_px_size;
                if (line_y >= 0 && line_y < screen_height) {
                    DrawLine(tex_x, line_y, tex_x + viewer->level_width * tile_px_size, line_y, 
                            (Color){100, 150, 200, 60});
                }
            }
            
            for (uint32_t x = 0; x <= viewer->level_width; x++) {
                float line_x = tex_x + x * tile_px_size;
                if (line_x >= 0 && line_x < screen_width) {
                    DrawLine(line_x, tex_y, line_x, tex_y + viewer->level_height * tile_px_size,
                            (Color){100, 150, 200, 60});
                }
            }
            
            // Draw tile indices on hover (limited range)
            if (viewer->hover_tile_x >= 0 && viewer->hover_tile_y >= 0) {
                uint32_t tile_idx = viewer->hover_tile_y * viewer->level_width + viewer->hover_tile_x;
                if (tile_idx < viewer->level_width * viewer->level_height) {
                    uint16_t tile_val = viewer->tile_data[tile_idx];
                    
                    float tx = tex_x + viewer->hover_tile_x * tile_px_size + 2;
                    float ty = tex_y + viewer->hover_tile_y * tile_px_size + 2;
                    
                    char text[32];
                    snprintf(text, sizeof(text), "%u", tile_val);
                    DrawText(text, tx, ty, 10 * fmaxf(1.0f, viewer->zoom / 2), (Color){255, 255, 100, 220});
                }
            }
        }
        
        // Draw hover highlight
        if (viewer->hover_tile_x >= 0 && viewer->hover_tile_y >= 0) {
            float hover_x = tex_x + viewer->hover_tile_x * viewer->tile_size * viewer->zoom;
            float hover_y = tex_y + viewer->hover_tile_y * viewer->tile_size * viewer->zoom;
            float hover_w = viewer->tile_size * viewer->zoom;
            float hover_h = viewer->tile_size * viewer->zoom;
            
            DrawRectangleLines(hover_x, hover_y, hover_w, hover_h, (Color){0, 255, 255, 220});
        }
    }

    // Draw UI overlay
    viewer_render_ui(viewer);
}

void viewer_update_hover(LevelViewer* viewer, float tex_x, float tex_y, float tex_w, float tex_h) {
    if (!viewer) return;
    
    float mouse_x = GetMouseX();
    float mouse_y = GetMouseY();
    
    // Convert screen coordinates to tile coordinates
    float rel_x = mouse_x - tex_x;
    float rel_y = mouse_y - tex_y;
    
    float tile_px_size = viewer->tile_size * viewer->zoom;
    
    int tile_x = (int)(rel_x / tile_px_size);
    int tile_y = (int)(rel_y / tile_px_size);
    
    // Check bounds
    if (tile_x >= 0 && tile_x < (int)viewer->level_width && 
        tile_y >= 0 && tile_y < (int)viewer->level_height) {
        viewer->hover_tile_x = tile_x;
        viewer->hover_tile_y = tile_y;
    } else {
        viewer->hover_tile_x = -1;
        viewer->hover_tile_y = -1;
    }
}

void viewer_render_ui(LevelViewer* viewer) {
    if (!viewer) return;

    int screen_height = GetScreenHeight();
    
    // Semi-transparent panel at top-left
    DrawRectangle(5, 5, 320, 130, (Color){0, 0, 0, 180});
    DrawRectangleLines(5, 5, 320, 130, (Color){50, 80, 120, 255});
    
    int y = 12;
    DrawText("PS1 TileViz", 15, y, 20, (Color){100, 200, 255, 255});
    y += 28;
    
    DrawRectangle(10, y, 310, 1, (Color){100, 100, 120, 150});
    y += 8;
    
    char info[256];
    
    // Show level info
    if (viewer->level_width > 0) {
        snprintf(info, sizeof(info), "Level: %ux%u tiles (%ux%u px)", 
                 viewer->level_width, viewer->level_height,
                 viewer->level_width * viewer->tile_size, 
                 viewer->level_height * viewer->tile_size);
        DrawText(info, 15, y, 13, (Color){200, 200, 200, 255});
        y += 18;
    }
    
    // Show texture info
    if (viewer->texture_width > 0) {
        snprintf(info, sizeof(info), "Texture: %ux%u px | Tile: %upx", 
                 viewer->texture_width, viewer->texture_height, viewer->tile_size);
        DrawText(info, 15, y, 13, (Color){200, 200, 200, 255});
        y += 18;
    }
    
    // Show zoom level
    snprintf(info, sizeof(info), "Zoom: %.1fx", viewer->zoom);
    DrawText(info, 15, y, 13, (Color){100, 255, 100, 255});
    y += 18;
    
    // Show grid state
    snprintf(info, sizeof(info), "Grid: %s (G)", viewer->show_grid ? "ON" : "OFF");
    DrawText(info, 15, y, 13, viewer->show_grid ? (Color){100, 255, 100, 255} : (Color){255, 100, 100, 255});
    y += 18;
    
    // Show hover tile info
    if (viewer->hover_tile_x >= 0 && viewer->hover_tile_y >= 0) {
        snprintf(info, sizeof(info), "Tile: (%d, %d)", viewer->hover_tile_x, viewer->hover_tile_y);
        DrawText(info, 15, y, 13, (Color){255, 255, 100, 255});
    }
    
    // Draw hover tooltip
    viewer_render_tooltip(viewer);
    
    // Draw controls help at bottom
    DrawRectangle(5, screen_height - 60, 450, 55, (Color){0, 0, 0, 180});
    DrawRectangleLines(5, screen_height - 60, 450, 55, (Color){50, 80, 120, 255});
    
    y = screen_height - 52;
    DrawText("Controls:", 15, y, 12, (Color){255, 255, 100, 255});
    y += 16;
    DrawText("Arrow/Middle-drag: Pan | Scroll/+/-: Zoom | R: Reset | G: Grid", 15, y, 11, (Color){180, 180, 180, 255});
    y += 14;
    DrawText("Left-Click: Select tile | ESC: Exit | F5: Reload", 15, y, 11, (Color){180, 180, 180, 255});
}

void viewer_render_tooltip(LevelViewer* viewer) {
    if (!viewer || viewer->hover_tile_x < 0 || viewer->hover_tile_y < 0) return;
    if (!viewer->tile_data) return;
    
    int mouse_x = GetMouseX();
    int mouse_y = GetMouseY();
    
    uint32_t tile_idx = viewer->hover_tile_y * viewer->level_width + viewer->hover_tile_x;
    uint16_t tile_value = 0;
    
    if (tile_idx < viewer->level_width * viewer->level_height) {
        tile_value = viewer->tile_data[tile_idx];
    }
    
    // Build tooltip text
    char tooltip[512];
    snprintf(tooltip, sizeof(tooltip), 
             "Tile: (%d, %d)\n"
             "Index: %u\n"
             "Value: 0x%04X (%u)",
             viewer->hover_tile_x, viewer->hover_tile_y,
             tile_idx,
             tile_value, tile_value);
    
    // Calculate tooltip size
    int tooltip_width = 160;
    int tooltip_height = 60;
    
    // Position tooltip to avoid going off screen
    int tooltip_x = mouse_x + 15;
    int tooltip_y = mouse_y + 15;
    
    if (tooltip_x + tooltip_width > GetScreenWidth()) {
        tooltip_x = mouse_x - tooltip_width - 15;
    }
    if (tooltip_y + tooltip_height > GetScreenHeight()) {
        tooltip_y = mouse_y - tooltip_height - 15;
    }
    
    // Draw tooltip background
    DrawRectangle(tooltip_x, tooltip_y, tooltip_width, tooltip_height, (Color){40, 50, 60, 230});
    DrawRectangleLines(tooltip_x, tooltip_y, tooltip_width, tooltip_height, (Color){80, 100, 140, 255});
    
    // Draw tooltip text
    DrawText(tooltip, tooltip_x + 8, tooltip_y + 8, 12, (Color){255, 255, 255, 255});
}

void viewer_handle_input(LevelViewer* viewer) {
    if (!viewer) return;

    // Select tile on left click
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (viewer->hover_tile_x >= 0 && viewer->hover_tile_y >= 0) {
            viewer->selected_tile_x = viewer->hover_tile_x;
            viewer->selected_tile_y = viewer->hover_tile_y;
        }
    }
}

void viewer_cleanup(LevelViewer* viewer) {
    if (!viewer) return;

    if (viewer->tile_data) {
        free(viewer->tile_data);
        viewer->tile_data = NULL;
    }

    // Free cached texture
    if (viewer->texture_valid && viewer->cached_texture.id != 0) {
        UnloadTexture(viewer->cached_texture);
        viewer->cached_texture.id = 0;
        viewer->texture_valid = false;
    }

    if (viewer->tim) {
        if (viewer->tim->image_data) {
            free(viewer->tim->image_data);
        }
        if (viewer->tim->palette) {
            free(viewer->tim->palette->colors);
            free(viewer->tim->palette);
        }
        TIM_FreeFile(viewer->tim);
        viewer->tim = NULL;
    }

    viewer->palette = NULL;
    viewer->level_width = 0;
    viewer->level_height = 0;
    viewer->selected_tile_x = -1;
    viewer->selected_tile_y = -1;

    free(viewer);

    LOG_INFO("level_viewer.c", __LINE__, "Level viewer cleaned up");
}

void viewer_set_zoom(LevelViewer* viewer, float zoom) {
    if (viewer) {
        viewer->zoom = zoom;
        if (viewer->zoom < MIN_ZOOM) viewer->zoom = MIN_ZOOM;
        if (viewer->zoom > MAX_ZOOM) viewer->zoom = MAX_ZOOM;
    }
}