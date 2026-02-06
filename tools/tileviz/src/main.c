// Tile visualization tool - main application

#include "raylib.h"
#include "log.h"
#include "tim_parser.h"
#include "level_loader.h"
#include "tile_renderer.h"
#include "level_viewer.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define DEFAULT_ZOOM 1.0f

typedef struct {
    bool running;
    bool level_loaded;
    LevelData* level;
    TIMFile* texture;
    PS1CLUT* palette;
    LevelViewer* viewer;
    char current_level_path[512];
    char current_texture_path[512];
} Application;

void app_init(Application* app) {
    app->running = true;
    app->level_loaded = false;
    app->level = NULL;
    app->texture = NULL;
    app->palette = NULL;
    app->viewer = NULL;
    app->current_level_path[0] = 0;
    app->current_texture_path[0] = 0;

    LOG_INFO("app.c", __LINE__, "Application initialized");
}

void app_cleanup(Application* app) {
    if (app->viewer) {
        viewer_cleanup(app->viewer);
        app->viewer = NULL;
    }

    if (app->palette) {
        if (app->texture->palette) {
            free(app->texture->palette->colors);
            free(app->texture->palette);
        }
        TIM_FreeFile(app->texture);
        app->texture = NULL;
        app->palette = NULL;
    }

    if (app->level) {
        level_free(app->level);
        app->level = NULL;
    }

    LOG_INFO("app.c", __LINE__, "Application cleaned up");
}

void app_load_level(Application* app, const char* level_path, const char* texture_path) {
    app_cleanup(app);

    LOG_INFO("app.c", __LINE__, "Loading level: %s", level_path);
    LOG_INFO("app.c", __LINE__, "Loading texture: %s", texture_path);

    // Load level
    app->level = level_load(level_path);
    if (!app->level) {
        LOG_ERROR("app.c", __LINE__, "Failed to load level");
        return;
    }

    // Load texture
    app->texture = TIM_LoadFile(texture_path);
    if (!app->texture) {
        LOG_ERROR("app.c", __LINE__, "Failed to load texture");
        level_free(app->level);
        app->level = NULL;
        return;
    }

    // Load palette
    app->palette = app->texture->palette;

    // Create viewer
    app->viewer = viewer_create();

    // Setup viewer
    viewer_load_level(app->viewer, level_path, texture_path);

    app->level_loaded = true;
    strncpy(app->current_level_path, level_path, sizeof(app->current_level_path) - 1);
    strncpy(app->current_texture_path, texture_path, sizeof(app->current_texture_path) - 1);

    LOG_INFO("app.c", __LINE__, "Level and texture loaded successfully");
}

void app_update(Application* app) {
    if (!app->level_loaded) return;

    viewer_update(app->viewer);
    viewer_handle_input(app->viewer);
}

void app_render(Application* app) {
    if (!app->level_loaded) {
        DrawText("Tile Visualization Tool", 20, 20, 32, WHITE);
        DrawText("Press F5 to load level", 20, 80, 24, GRAY);
        DrawText("Press ESC to exit", 20, 120, 24, GRAY);

        return;
    }

    // Clear background
    ClearBackground(RAYWHITE);

    // Draw level
    viewer_render(app->viewer);

    // Draw UI
    DrawRectangle(10, 10, 300, 100, BLACK);
    DrawText(app->current_level_path, 20, 20, 20, WHITE);
    char size_text[256];
    snprintf(size_text, sizeof(size_text), "Size: %dx%d tiles", app->level->width_tiles, app->level->height_tiles);
    DrawText(size_text, 20, 45, 20, WHITE);
    char file_text[256];
    snprintf(file_text, sizeof(file_text), "File size: %.2f KB", app->level->file_size / 1024.0f);
    DrawText(file_text, 20, 70, 20, WHITE);
    DrawText("ESC - Exit, F5 - Reload", 20, 95, 16, GRAY);
}

void app_handle_input(Application* app) {
    if (IsKeyPressed(KEY_ESCAPE)) {
        app->running = false;
    }

    if (IsKeyPressed(KEY_F5)) {
        if (app->current_level_path[0] != 0) {
            app_load_level(app, app->current_level_path, app->current_texture_path);
        }
    }
}

int main(void) {
    LOG_INFO("main.c", __LINE__, "Initializing tile visualization tool");

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "PS1 Tile Visualization Tool");
    SetTargetFPS(60);

    Application app;
    app_init(&app);

    LOG_INFO("main.c", __LINE__, "Entering main loop");

    while (app.running) {
        app_handle_input(&app);
        app_update(&app);
        app_render(&app);
        EndDrawing();
    }

    app_cleanup(&app);
    CloseWindow();

    LOG_INFO("main.c", __LINE__, "Application terminated");
    return 0;
}