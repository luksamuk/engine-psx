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
#define DEFAULT_ZOOM 2.0f

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

    LOG_INFO("main.c", __LINE__, "Application initialized");
}

void app_cleanup(Application* app) {
    if (app->viewer) {
        viewer_cleanup(app->viewer);
        app->viewer = NULL;
    }

    if (app->level) {
        level_free(app->level);
        app->level = NULL;
    }
    
    if (app->texture) {
        if (app->texture->palette) {
            if (app->texture->palette->colors) {
                free(app->texture->palette->colors);
            }
            free(app->texture->palette);
        }
        if (app->texture->image_data) {
            free(app->texture->image_data);
        }
        TIM_FreeFile(app->texture);
        app->texture = NULL;
        app->palette = NULL;
    }

    LOG_INFO("main.c", __LINE__, "Application cleaned up");
}

void app_load_level(Application* app, const char* level_path, const char* texture_path) {
    app_cleanup(app);

    LOG_INFO("main.c", __LINE__, "Loading level: %s", level_path);
    LOG_INFO("main.c", __LINE__, "Loading texture: %s", texture_path);

    // Load texture first (TIM file)
    app->texture = TIM_LoadFile(texture_path);
    if (!app->texture) {
        LOG_ERROR("main.c", __LINE__, "Failed to load texture");
        return;
    }
    
    app->palette = app->texture->palette;

    // Load level (MAP file)
    app->level = level_load(level_path);
    if (!app->level) {
        LOG_ERROR("main.c", __LINE__, "Failed to load level");
        if (app->texture->palette) {
            free(app->texture->palette->colors);
            free(app->texture->palette);
        }
        if (app->texture->image_data) {
            free(app->texture->image_data);
        }
        TIM_FreeFile(app->texture);
        app->texture = NULL;
        app->palette = NULL;
        return;
    }

    // Create viewer
    app->viewer = viewer_create();
    if (!app->viewer) {
        LOG_ERROR("main.c", __LINE__, "Failed to create viewer");
        level_free(app->level);
        app->level = NULL;
        return;
    }

    // Setup viewer
    viewer_load_level(app->viewer, level_path, texture_path);

    app->level_loaded = true;
    strncpy(app->current_level_path, level_path, sizeof(app->current_level_path) - 1);
    app->current_level_path[sizeof(app->current_level_path) - 1] = 0;
    strncpy(app->current_texture_path, texture_path, sizeof(app->current_texture_path) - 1);
    app->current_texture_path[sizeof(app->current_texture_path) - 1] = 0;

    LOG_INFO("main.c", __LINE__, "Level and texture loaded successfully");
}

void app_update(Application* app) {
    if (!app->level_loaded || !app->viewer) return;

    viewer_update(app->viewer);
}

void app_render(Application* app) {
    if (!app->level_loaded || !app->viewer) {
        // Draw welcome screen
        ClearBackground((Color){30, 30, 40, 255});
        
        DrawText("PS1 Tile Visualization Tool (TileViz)", 20, 20, 32, WHITE);
        
        if (app->current_level_path[0] == 0) {
            DrawText("Usage: tileviz <MAP_FILE> <TIM_FILE>", 20, 80, 24, GRAY);
            DrawText("", 20, 110, 24, GRAY);
            DrawText("Example:", 20, 140, 24, (Color){200, 200, 100, 255});
            DrawText("  ./tileviz MAP16.MAP TILES.TIM", 20, 170, 20, GRAY);
        } else {
            DrawText("Loading...", 20, 80, 24, YELLOW);
        }
        
        DrawText("Press ESC to exit", 20, GetScreenHeight() - 40, 20, GRAY);
        return;
    }

    // Render the viewer
    viewer_render(app->viewer);
}

void app_handle_input(Application* app) {
    if (IsKeyPressed(KEY_ESCAPE)) {
        app->running = false;
    }

    if (IsKeyPressed(KEY_F5)) {
        if (app->current_level_path[0] != 0 && app->current_texture_path[0] != 0) {
            app_load_level(app, app->current_level_path, app->current_texture_path);
        }
    }
    
    // Pass input to viewer
    if (app->viewer) {
        viewer_handle_input(app->viewer);
    }
}

int main(int argc, char** argv) {
    LOG_INFO("main.c", __LINE__, "Initializing PS1 Tile Visualization Tool");

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "PS1 TileViz - Tile/Texture Viewer");
    SetTargetFPS(60);

    Application app;
    app_init(&app);

    // Load files from command line arguments
    if (argc >= 2) {
        strncpy(app.current_level_path, argv[1], sizeof(app.current_level_path) - 1);
        LOG_INFO("main.c", __LINE__, "Level path from CLI: %s", argv[1]);
    }
    if (argc >= 3) {
        strncpy(app.current_texture_path, argv[2], sizeof(app.current_texture_path) - 1);
        LOG_INFO("main.c", __LINE__, "Texture path from CLI: %s", argv[2]);
    }

    // Auto-load if both files provided
    if (argc >= 3) {
        LOG_INFO("main.c", __LINE__, "Auto-loading level and texture from CLI args");
        app_load_level(&app, app.current_level_path, app.current_texture_path);
    } else if (argc >= 2) {
        LOG_INFO("main.c", __LINE__, "Missing texture file, showing usage");
    }

    LOG_INFO("main.c", __LINE__, "Entering main loop");

    while (app.running && !WindowShouldClose()) {
        app_handle_input(&app);
        app_update(&app);
        
        BeginDrawing();
        app_render(&app);
        EndDrawing();
    }

    app_cleanup(&app);
    CloseWindow();

    LOG_INFO("main.c", __LINE__, "Application terminated");
    return 0;
}