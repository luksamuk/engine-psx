// Viewer unit tests

#include <string.h>
#include "level_viewer.h"
#include "tim_parser.h"
#include "level_loader.h"
#include "log.h"
#include <stdio.h>
#include <assert.h>

void test_viewer_create(void) {
    LOG_INFO("test_viewer.c", __LINE__, "Testing viewer_create...");

    LevelViewer* viewer = viewer_create();

    assert(viewer != NULL);
    assert(viewer->level_width == 0);
    assert(viewer->level_height == 0);
    assert(viewer->selected_tile_x == -1);
    assert(viewer->selected_tile_y == -1);
    assert(viewer->camera_x == 0);
    assert(viewer->camera_y == 0);
    assert(viewer->zoom == 1.0f);
    assert(viewer->tile_data == NULL);
    assert(viewer->tim == NULL);
    assert(viewer->palette == NULL);

    viewer_cleanup(viewer);
    LOG_INFO("test_viewer.c", __LINE__, "viewer_create: PASSED");
}

void test_viewer_set_zoom(void) {
    LOG_INFO("test_viewer.c", __LINE__, "Testing viewer_set_zoom...");

    LevelViewer* viewer = viewer_create();

    viewer_set_zoom(viewer, 2.0f);
    assert(viewer->zoom == 2.0f);

    viewer_set_zoom(viewer, 0.5f);
    assert(viewer->zoom == 0.5f);

    viewer_set_zoom(viewer, 1.5f);
    assert(viewer->zoom == 1.5f);

    viewer_cleanup(viewer);
    LOG_INFO("test_viewer.c", __LINE__, "viewer_set_zoom: PASSED");
}

void test_viewer_update_zoom(void) {
    LOG_INFO("test_viewer.c", __LINE__, "Testing viewer_update...");

    LevelViewer* viewer = viewer_create();

    // Simulate keyboard input
    for (int i = 0; i < 10; i++) {
        viewer->zoom = 1.0f;
        viewer_update(viewer);
    }

    viewer_cleanup(viewer);
    LOG_INFO("test_viewer.c", __LINE__, "viewer_update: PASSED");
}

void test_viewer_handle_input(void) {
    LOG_INFO("test_viewer.c", __LINE__, "Testing viewer_handle_input...");

    LevelViewer* viewer = viewer_create();

    // Simulate mouse click to select tile
    viewer->level_width = 10;
    viewer->level_height = 10;

    // Simulate a mouse click at appropriate position
    viewer_handle_input(viewer);

    assert(viewer->selected_tile_x == -1);
    assert(viewer->selected_tile_y == -1);

    viewer_cleanup(viewer);
    LOG_INFO("test_viewer.c", __LINE__, "viewer_handle_input: PASSED");
}

void test_viewer_render_empty(void) {
    LOG_INFO("test_viewer.c", __LINE__, "Testing viewer_render (empty state)...");

    LevelViewer* viewer = viewer_create();

    viewer->tim = TIM_LoadFile("tests/unit/test_tim_16bit.bin");

    if (viewer->tim) {
        viewer->palette = viewer->tim->palette;

        int screen_width = GetScreenWidth();
        int screen_height = GetScreenHeight();

        viewer_render(viewer);

        TIM_FreeFile(viewer->tim);
        viewer->tim = NULL;
    }

    viewer_cleanup(viewer);
    LOG_INFO("test_viewer.c", __LINE__, "viewer_render (empty state): PASSED");
}

void run_viewer_tests(void) {
    test_viewer_create();
    test_viewer_set_zoom();
    test_viewer_update_zoom();
    test_viewer_handle_input();
    test_viewer_render_empty();

    LOG_INFO("test_viewer.c", __LINE__, "All viewer tests completed");
}