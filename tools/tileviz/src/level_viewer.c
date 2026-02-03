// Level viewer implementation

#include "level_viewer.h"
#include "tim_parser.h"
#include "tile_renderer.h"
#include "log.h"
#include <raylib.h>
#include <stdlib.h>

LevelViewer* viewer_create(void) {
    LevelViewer* viewer = (LevelViewer*)malloc(sizeof(LevelViewer));
    if (!viewer) {
        LOG_ERROR("level_viewer.c", __LINE__, "Failed to allocate level viewer");
        return NULL;
    }

    viewer->level_width = 0;
    viewer->level_height = 0;
    viewer->tile_data = NULL;
    viewer->texture = NULL;
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

    // Would load level and texture data here
    // For now, just store paths
    LOG_INFO("level_viewer.c", __LINE__, "Load level: %s, texture: %s", level_path, texture_path);

    viewer->level_width = 64;
    viewer->level_height = 64;
    viewer->selected_tile_x = -1;
    viewer->selected_tile_y = -1;

    // Would allocate and load tile data here

    return true;
}

void viewer_update(LevelViewer* viewer) {
    if (!viewer) return;

    // Handle camera movement
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

    // Handle zoom
    if (IsKeyDown(KEY_EQUAL)) {
        viewer->zoom += 0.1f;
    }
    if (IsKeyDown(KEY_MINUS)) {
        viewer->zoom -= 0.1f;
        if (viewer->zoom < 0.5f) viewer->zoom = 0.5f;
    }
}

void viewer_render(LevelViewer* viewer) {
    if (!viewer) return;

    // Would render level with camera and zoom here
}

void viewer_cleanup(LevelViewer* viewer) {
    if (!viewer) return;

    if (viewer->tile_data) {
        free(viewer->tile_data);
    }

    if (viewer->palette) {
        if (viewer->texture->palette) {
            free(viewer->texture->palette->colors);
            free(viewer->texture->palette);
        }
        TIM_FreeFile(viewer->texture);
    }

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

    // Handle tile selection
    if (viewer->selected_tile_x >= 0 && viewer->selected_tile_y >= 0) {
        if (IsKeyDown(KEY_ESCAPE)) {
            viewer->selected_tile_x = -1;
            viewer->selected_tile_y = -1;
        }
    }

    // Mouse interaction for tile selection
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