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
    
    // Process based on BPP mode
    switch (tim->bpp) {
        case TIM_BPP_4BIT: {
            // 4-bit indexed color: each 16-bit word contains 4 pixels
            // Width is actual pixel width
            uint16_t img_w_words = tim->image_rect.width;
            
            image.width = width;
            image.height = height;
            image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
            image.mipmaps = 1;
            image.data = (unsigned char*)malloc(image.width * image.height * 4);
            
            Color* pixels = (Color*)image.data;
            
            for (uint32_t y = 0; y < height; y++) {
                for (uint32_t x = 0; x < width; x++) {
                    // Each 16-bit word contains 4 pixels (4 bits each)
                    // Word offset in image data
                    uint32_t word_x = x / 4;
                    uint32_t word_offset = (y * img_w_words + word_x);
                    uint16_t word_data = ((uint16_t*)src_data)[word_offset];
                    
                    // Pixel index within word (0-3)
                    uint32_t pixel_in_word = x % 4;
                    // Shift amount: bits 0-3 are different positions
                    // PS1 stores pixels as: [p3:p2:p1:p0] in big-endian sense
                    // Actually it's [hi:lo] where bits 15-12=p0, 11-8=p1, etc... no
                    // Let's try the correct order: bits 0-3=p0, 4-7=p1, 8-11=p2, 12-15=p3
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
            // 8-bit indexed color: each 16-bit word contains 2 pixels
            uint16_t img_w_words = tim->image_rect.width;
            
            image.width = width;
            image.height = height;
            image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
            image.mipmaps = 1;
            image.data = (unsigned char*)malloc(image.width * image.height * 4);
            
            Color* pixels = (Color*)image.data;
            
            for (uint32_t y = 0; y < height; y++) {
                for (uint32_t x = 0; x < width; x++) {
                    // Each 16-bit word contains 2 pixels (8 bits each)
                    uint32_t word_x = x / 2;
                    uint32_t word_offset = (y * img_w_words + word_x);
                    uint16_t word_data = ((uint16_t*)src_data)[word_offset];
                    
                    // Pixel index within word (0 or 1)
                    uint32_t pixel_in_word = x % 2;
                    uint8_t idx;
                    if (pixel_in_word == 0) {
                        idx = word_data & 0xFF;  // Low byte = first pixel
                    } else {
                        idx = (word_data >> 8) & 0xFF;  // High byte = second pixel
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
            // 16-bit direct color RGB555
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
                
                // Scale 5-bit to 8-bit
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