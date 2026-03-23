// Level viewer implementation

#include "level_viewer.h"
#include "tim_parser.h"
#include "level_loader.h"
#include "log.h"
#include <raylib.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
    viewer->camera_x = 0;
    viewer->camera_y = 0;
    viewer->zoom = 1.0f;

    LOG_INFO("level_viewer.c", __LINE__, "Level viewer created");

    return viewer;
}

bool viewer_load_level(LevelViewer* viewer, const char* level_path, const char* texture_path) {
    if (!viewer) return false;

    LOG_INFO("level_viewer.c", __LINE__, "Loading level: %s", level_path);
    LOG_INFO("level_viewer.c", __LINE__, "Loading texture: %s", texture_path);

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
        TIM_FreeFile(texture);
        return false;
    }

    viewer->tim = texture;
    viewer->palette = texture->palette;
    viewer->level_width = level->width_tiles;
    viewer->level_height = level->height_tiles;
    
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
    viewer->camera_x = 0;
    viewer->camera_y = 0;
    viewer->zoom = 1.0f;

    LOG_INFO("level_viewer.c", __LINE__, "Level loaded successfully: %dx%d tiles", 
             viewer->level_width, viewer->level_height);

    return true;
}

void viewer_update(LevelViewer* viewer) {
    if (!viewer) return;

    const float speed = 5.0f;

    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
        viewer->camera_x -= speed;
    }
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
        viewer->camera_x += speed;
    }
    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) {
        viewer->camera_y -= speed;
    }
    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) {
        viewer->camera_y += speed;
    }

    if (IsKeyDown(KEY_EQUAL) || IsKeyDown(KEY_KP_ADD)) {
        viewer->zoom += 0.1f;
    }
    if (IsKeyDown(KEY_MINUS) || IsKeyDown(KEY_KP_SUBTRACT)) {
        viewer->zoom -= 0.1f;
        if (viewer->zoom < 0.5f) viewer->zoom = 0.5f;
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
    
    // Standard PS1 TIM formats (no compression)
    switch (tim->header.image_format) {
        case TIM_FORMAT_CLUT_RAW_4BIT: {
            image.width = (width + 1) / 2;
            image.height = height;
            image.format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE;
            image.data = (unsigned char*)malloc(image.width * image.height * 2);
            
            for (int y = 0; y < image.height; y++) {
                for (int x = 0; x < image.width; x++) {
                    uint8_t byte = src_data[(y * image.width) + x];
                    
                    uint16_t pixel1 = 0, pixel2 = 0;
                    if ((x * 2 + 1) < (int)width && palette->entry_count > 0) {
                        uint8_t idx = byte & 0x0F;
                        pixel1 = ((palette->colors[idx].r & 0x1F) << 11) |
                               ((palette->colors[idx].g & 0x1F) << 6) |
                               (palette->colors[idx].b & 0x1F);
                    } else if (palette->entry_count > 0) {
                        uint8_t idx = byte >> 4;
                        pixel1 = ((palette->colors[idx].r & 0x1F) << 11) |
                               ((palette->colors[idx].g & 0x1F) << 6) |
                               (palette->colors[idx].b & 0x1F);
                    }
                    
                    if ((x * 2 + 1) < (int)width && palette->entry_count > 0) {
                        uint8_t idx = (byte >> 4) & 0x0F;
                        pixel2 = ((palette->colors[idx].r & 0x1F) << 11) |
                               ((palette->colors[idx].g & 0x1F) << 6) |
                               (palette->colors[idx].b & 0x1F);
                    } else if ((x * 2) >= (int)width) {
                        uint8_t idx = byte >> 4;
                        pixel2 = ((palette->colors[idx].r & 0x1F) << 11) |
                               ((palette->colors[idx].g & 0x1F) << 6) |
                               (palette->colors[idx].b & 0x1F);
                    }
                    
                    uint16_t* dst = &(((uint16_t*)image.data)[(y * image.width) + x]);
                    dst[0] = pixel1;
                    dst[1] = pixel2;
                }
            }
            break;
        }
        
        case TIM_FORMAT_CLUT_RAW_8BIT: {
            image.width = width;
            image.height = height;
            image.format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE;
            image.data = (unsigned char*)malloc(image.width * image.height * 2);
            
            for (int y = 0; y < image.height; y++) {
                for (int x = 0; x < image.width; x++) {
                    uint8_t idx = src_data[(y * image.width) + x];
                    uint16_t pixel = 0;
                    if (idx < palette->entry_count) {
                        pixel = ((palette->colors[idx].r & 0x1F) << 11) |
                               ((palette->colors[idx].g & 0x1F) << 6) |
                               (palette->colors[idx].b & 0x1F);
                    }
                    memcpy(&(((uint16_t*)image.data)[(y * image.width) + x]), &pixel, sizeof(uint16_t));
                }
            }
            break;
        }
        
        case TIM_FORMAT_CLUT_RAW_16BIT: {
            image.width = width;
            image.height = height;
            image.format = PIXELFORMAT_UNCOMPRESSED_R5G6B5;
            image.data = (unsigned char*)malloc(image.width * image.height * 2);
            
            // PS1 TIM 16-bit is RGB555, convert to RGB565 for Raylib
            for (uint32_t i = 0; i < image.width * image.height; i++) {
                uint16_t psx_pixel = ((uint16_t*)src_data)[i];
                // PS1 RGB555: 0-4=Blue, 5-9=Green, 10-14=Red, 15=mask
                // Raylib RGB565: 0-4=Blue, 5-10=Green, 11-15=Red
                uint8_t r = (psx_pixel >> 10) & 0x1F;
                uint8_t g = (psx_pixel >> 5) & 0x1F;
                uint8_t b = psx_pixel & 0x1F;
                // Expand 5-bit to 6-bit for green
                uint16_t raylib_pixel = (r << 11) | ((g << 1) | (g >> 4)) | b;
                memcpy(&(((uint16_t*)image.data)[i]), &raylib_pixel, sizeof(uint16_t));
            }
            break;
        }
        
        default:
            LOG_ERROR("level_viewer.c", __LINE__, "Unsupported TIM format for texture: 0x%04X", tim->header.image_format);
            return texture;
    }

    texture = LoadTextureFromImage(image);
    UnloadImage(image);
    
    return texture;
}

void viewer_render(LevelViewer* viewer) {
    if (!viewer) return;

    int screen_width = GetScreenWidth();
    int screen_height = GetScreenHeight();

    ClearBackground(RAYWHITE);

    if (viewer->tim && viewer->palette && viewer->tim->image_data) {
        Texture2D tex = TIM_ToRaylibTexture(viewer->tim);
        Texture2D texture = tex;
        
        if (texture.id != 0) {
            Rectangle source = {0, 0, (float)texture.width, (float)texture.height};
            Rectangle dest = {
                (float)(screen_width / 2 - (viewer->camera_x * viewer->zoom)),
                (float)(screen_height / 2 - (viewer->camera_y * viewer->zoom)),
                (float)texture.width * viewer->zoom,
                (float)texture.height * viewer->zoom
            };
            
            DrawTexturePro(texture, source, dest, (Vector2){0, 0}, 0.0f, WHITE);
            
            UnloadTexture(texture);
        }
    }

    if (viewer->tile_data && viewer->level_width > 0 && viewer->level_height > 0) {
        int tile_size = 64;
        
        for (uint32_t y = 0; y < viewer->level_height; y++) {
            for (uint32_t x = 0; x < viewer->level_width; x++) {
                int tile_idx = viewer->tile_data[y * viewer->level_width + x];
                
                if (tile_idx == 0) continue;
                
                Texture2D texture = TIM_ToRaylibTexture(viewer->tim);
                
                Rectangle tile_source = {(float)((tile_idx - 1) % 4) * tile_size,
                                       (float)((tile_idx - 1) / 4) * tile_size,
                                       (float)tile_size, (float)tile_size};
                
                int tile_screen_x = screen_width / 2 - (viewer->camera_x * viewer->zoom) +
                                   (int)(x * tile_size * viewer->zoom);
                int tile_screen_y = screen_height / 2 - (viewer->camera_y * viewer->zoom) +
                                   (int)(y * tile_size * viewer->zoom);
                
                Rectangle tile_dest = {(float)tile_screen_x, (float)tile_screen_y,
                                     (float)tile_size * viewer->zoom,
                                     (float)tile_size * viewer->zoom};
                
                DrawTexturePro(texture, tile_source, tile_dest,
                             (Vector2){0, 0}, 0.0f, WHITE);
                
                if (texture.id != 0) {
                    UnloadTexture(texture);
                }
                
                if (viewer->selected_tile_x >= 0 && viewer->selected_tile_y >= 0) {
                    if (viewer->selected_tile_x == (int)x && viewer->selected_tile_y == (int)y) {
                        DrawRectangleLines(tile_screen_x, tile_screen_y,
                                          (int)(tile_size * viewer->zoom),
                                          (int)(tile_size * viewer->zoom),
                                          RED);
                    }
                }
            }
        }
    }

    if (viewer->selected_tile_x >= 0 && viewer->selected_tile_y >= 0) {
        int screen_x = screen_width / 2 - (viewer->camera_x * viewer->zoom) +
                      (int)(viewer->selected_tile_x * 64 * viewer->zoom);
        int screen_y = screen_height / 2 - (viewer->camera_y * viewer->zoom) +
                      (int)(viewer->selected_tile_y * 64 * viewer->zoom);
        
        char selection_text[256];
        snprintf(selection_text, sizeof(selection_text), "Selected: Tile(%d, %d)", 
                 viewer->selected_tile_x, viewer->selected_tile_y);
        DrawText(selection_text, screen_x, screen_y + 10, 16, GRAY);
    }

    DrawFPS(10, 10);
}

void viewer_cleanup(LevelViewer* viewer) {
    if (!viewer) return;

    if (viewer->tile_data) {
        free(viewer->tile_data);
        viewer->tile_data = NULL;
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
    }
}

void viewer_handle_input(LevelViewer* viewer) {
    if (!viewer) return;

    if (IsKeyPressed(KEY_ESCAPE)) {
        viewer->selected_tile_x = -1;
        viewer->selected_tile_y = -1;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        int mouse_x = GetMouseX();
        int mouse_y = GetMouseY();

        if (viewer->level_width > 0 && viewer->level_height > 0) {
            int tile_w = 64;
            int tile_h = 64;

            int tile_x = (int)((mouse_x - viewer->camera_x) / viewer->zoom / tile_w);
            int tile_y = (int)((mouse_y - viewer->camera_y) / viewer->zoom / tile_h);

            if (tile_x >= 0 && tile_x < viewer->level_width &&
                tile_y >= 0 && tile_y < viewer->level_height) {
                viewer->selected_tile_x = tile_x;
                viewer->selected_tile_y = tile_y;
            }
        }
    }
}