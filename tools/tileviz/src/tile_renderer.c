// Tile renderer implementation

#include "tile_renderer.h"
#include "log.h"
#include <raylib.h>
#include <stdlib.h>

TileRenderer* tr_create(int width, int height, int tile_size) {
    TileRenderer* renderer = (TileRenderer*)malloc(sizeof(TileRenderer));
    if (!renderer) {
        LOG_ERROR("tile_renderer.c", __LINE__, "Failed to allocate tile renderer");
        return NULL;
    }

    renderer->width = width;
    renderer->height = height;
    renderer->tile_size = tile_size;
    renderer->texture = NULL;
    renderer->palette = NULL;
    renderer->use_clut = false;
    renderer->dither_enable = false;

    LOG_INFO("tile_renderer.c", __LINE__, "Tile renderer created: %dx%d with tile size %d", width, height, tile_size);

    return renderer;
}

void tr_set_texture(TileRenderer* renderer, TIMFile* texture) {
    if (!renderer) return;

    renderer->texture = texture;
    // Standard PS1 TIMs use CLUT-based color indexing
    renderer->use_clut = (texture->bpp == TIM_BPP_4BIT ||
                          texture->bpp == TIM_BPP_8BIT);
    LOG_INFO("tile_renderer.c", __LINE__, "Texture set for rendering: %dx%d", texture->width, texture->height);
}

void tr_set_palette(TileRenderer* renderer, PS1CLUT* palette) {
    if (!renderer) return;

    renderer->palette = palette;
    renderer->use_clut = true;
    LOG_INFO("tile_renderer.c", __LINE__, "Palette set for rendering: %d colors", palette->entry_count);
}

void tr_render_tile(TileRenderer* renderer, uint16_t tile_index, int x, int y) {
    if (!renderer || !renderer->texture) return;

    int img_width = renderer->texture->width;
    int img_height = renderer->texture->height;

    int tile_w = img_width / 4;
    int tile_h = img_height / 4;

    int tex_x = (tile_index % 4) * tile_w;
    int tex_y = (tile_index / 4) * tile_h;

    Image image = {
        .width = img_width,
        .height = img_height,
        .data = malloc(img_width * img_height * 4),
        .mipmaps = 1
    };

    if (!image.data) {
        LOG_ERROR("tile_renderer.c", __LINE__, "Failed to allocate image data");
        return;
    }

    uint8_t* src_data = renderer->texture->image_data;
    uint32_t* dest_data = (uint32_t*)image.data;

    for (int iy = 0; iy < img_height; iy++) {
        for (int ix = 0; ix < img_width; ix++) {
            int pixel_idx = iy * img_width + ix;

            uint16_t pixel_value = 0;

            if (src_data) {
                if (pixel_idx < img_width * img_height) {
                    pixel_value = src_data[pixel_idx];
                }
            }

            if (renderer->use_clut && renderer->palette) {
                PS1Color clut_color = renderer->palette->colors[pixel_value % renderer->palette->entry_count];
                dest_data[pixel_idx] = (255 << 24) | (clut_color.r << 16) | (clut_color.g << 8) | clut_color.b;
            } else {
                dest_data[pixel_idx] = (255 << 24) | (pixel_value << 16) | (pixel_value << 8) | pixel_value;
            }
        }
    }

    Texture2D texture = LoadTextureFromImage(image);
    free(image.data);

    if (texture.id == 0) {
        LOG_ERROR("tile_renderer.c", __LINE__, "Failed to load texture from image");
        return;
    }

    Rectangle src_rect = { (float)tex_x, (float)tex_y, (float)tile_w, (float)tile_h };
    Rectangle dest_rect = { (float)x, (float)y, (float)renderer->tile_size, (float)renderer->tile_size };

    DrawTexturePro(texture, src_rect, dest_rect, (Vector2){0, 0}, 0, WHITE);

    UnloadTexture(texture);

    LOG_TRACE("tile_renderer.c", __LINE__, "Rendered tile %u at (%d,%d)", tile_index, x, y);
}

void tr_render_level(TileRenderer* renderer, uint16_t* tile_data, int level_width, int level_height) {
    if (!renderer || !tile_data) return;

    LOG_INFO("tile_renderer.c", __LINE__, "Rendering level: %dx%d", level_width, level_height);

    for (int y = 0; y < level_height; y++) {
        for (int x = 0; x < level_width; x++) {
            uint16_t tile_idx = tile_data[y * level_width + x];
            if (tile_idx > 0) {
                tr_render_tile(renderer, tile_idx, x * renderer->tile_size, y * renderer->tile_size);
            }
        }
    }

    LOG_INFO("tile_renderer.c", __LINE__, "Level rendering complete");
}

int tr_get_tile_size(TileRenderer* renderer) {
    return renderer ? renderer->tile_size : 64;
}

void tr_cleanup(TileRenderer* renderer) {
    if (!renderer) return;

    renderer->texture = NULL;
    renderer->palette = NULL;

    free(renderer);

    LOG_INFO("tile_renderer.c", __LINE__, "Tile renderer cleaned up");
}