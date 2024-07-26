#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdint.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

static char *tileset    = NULL;
static char *tilemap16  = NULL;
static char *tilemap128 = NULL;
#define INT16_LE(x) ((x >> 8) | (x << 8))


typedef struct {
    uint16_t *subtiles;
} TilemapTile;

typedef struct {
    uint16_t    tile_width;
    uint16_t    num_tiles;
    uint16_t    frame_side;
    TilemapTile *tiles;
} Tilemap;



/* Globals */

static GLuint texture;
static GLdouble tilew, tileh;

static Tilemap map16;
static Tilemap map128;

static uint16_t cur = 1;
static uint16_t timer = 60;

void
load_map(Tilemap *map, const char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if(!fp) {
        fprintf(stderr, "Error opening file %s\n", filename);
    }

    assert(fp != NULL);

    uint16_t buf;
    fread(&buf, sizeof(uint16_t), 1, fp);
    map->tile_width = INT16_LE(buf);
    fread(&buf, sizeof(uint16_t), 1, fp);
    map->num_tiles = INT16_LE(buf);
    fread(&buf, sizeof(uint16_t), 1, fp);
    map->frame_side = INT16_LE(buf);

    uint16_t num_subtiles = map->frame_side * map->frame_side;

    map->tiles = malloc(map->num_tiles * sizeof(TilemapTile));
    for(uint16_t i = 0; i < map->num_tiles; i++) {
        map->tiles[i].subtiles = malloc(num_subtiles * sizeof(uint16_t));
        for(uint16_t j = 0; j < num_subtiles; j++) {
            fread(&buf, sizeof(uint16_t), 1, fp);
            map->tiles[i].subtiles[j] = INT16_LE(buf);
        }
    }

    fclose(fp);
}


void
free_resources()
{
    if(tileset) free(tileset);
    if(tilemap16) free(tilemap16);
    if(tilemap128) free(tilemap128);
}

void
draw_tile8(uint16_t idx)
{
    // Calculate UV tile coordinates
    // Texture has a fixed width of 256, so we know there is a max number
    // of 32 tiles per row.
    uint16_t v_idx = idx >> 5; // div 32
    uint16_t u_idx = idx - (v_idx << 5);
    uint8_t
        u = u_idx << 3,
        v = v_idx << 3;

    // Convert discrete UV to ST coordinates
    double
        s = (float)u / 256.0,
        t = (float)v / 256.0;

    // Draw quad at upper left corner as 8x8. Delegate
    // positioning and scaling to pushed matrix
    glBegin(GL_QUADS);
    glColor3f(1, 1, 1);
    glTexCoord2d(s, t);
    glVertex2d(0, 0);
    
    glTexCoord2d(s + tilew, t);
    glVertex2d(8, 0);

    glTexCoord2d(s + tilew, t + tileh);
    glVertex2d(8, 8);

    glTexCoord2d(s, t + tileh);
    glVertex2d(0, 8);
    glEnd();
}

void
draw_tile16(TilemapTile *tile, uint16_t frame_side)
{
    uint16_t idx = 0;
    for(int j = 0; j < frame_side; j++) {
        for(int i = 0; i < frame_side; i++) {
            glPushMatrix();
            glTranslatef(i * 8, j * 8, 0);
            draw_tile8(tile->subtiles[idx]);
            glPopMatrix();
            idx++;
        }
    }
}

void
draw_tile128(TilemapTile *tile, uint16_t frame_side)
{
    uint16_t idx = 0;
    for(int j = 0; j < frame_side; j++) {
        for(int i = 0; i < frame_side; i++) {
            TilemapTile *tile16 = &map16.tiles[tile->subtiles[idx]];

            glPushMatrix();
            glTranslatef((float)(i * 16), (float)(j * 16), 0);
            draw_tile16(tile16, map16.frame_side);
            glPopMatrix();
            idx++;
        }
    }
}

void
draw_map_tile()
{
    TilemapTile *tile128 = &map128.tiles[cur];
    //float left = (float)(512 >> 1) - 64.0f;
    //float top  = (float)(512 >> 1) - 64.0f;

    glBindTexture(GL_TEXTURE_2D, texture);
    glPushMatrix();
    glLoadIdentity();
    //glTranslatef(left, top, 0);
    glScalef(4.0f, 4.0f, 0.0f);
    draw_tile128(tile128, map128.frame_side);
    glPopMatrix();
    glBindTexture(GL_TEXTURE_2D, 0);
}

int
main(int argc, char **argv)
{
    for(int i = 1; i < argc; i++) {
        if(!strcmp(argv[i], "--tileset")) {
            assert(argc > i + 1);
            tileset = malloc(sizeof(char) * strlen(argv[i + 1]));
            strcpy(tileset, argv[i + 1]);
            i++;
        }

        if(!strcmp(argv[i], "--map16")) {
            assert(argc > i + 1);
            tilemap16 = malloc(sizeof(char) * strlen(argv[i + 1]));
            strcpy(tilemap16, argv[i + 1]);
            i++;
        }

        if(!strcmp(argv[i], "--map128")) {
            assert(argc > i + 1);
            tilemap128 = malloc(sizeof(char) * strlen(argv[i + 1]));
            strcpy(tilemap128, argv[i + 1]);
            i++;
        }
    }

    if(!tileset || !tilemap16 || !tilemap128) {
        fprintf(stderr, "Must provide --tileset, --map16 and --map128\n");
        free_resources();
        return -1;
    }
    
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow* window = glfwCreateWindow(512, 512, "tiledrawer", NULL, NULL);
    if(window == NULL)
    {
        fprintf(stderr, "Could not create window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        fprintf(stderr, "Failed to initialize GLAD\n");
        return -1;
    }    

    glViewport(0, 0, 512, 512);

    glMatrixMode(GL_PROJECTION);
    glOrtho(0.0f, 512.0f, 512.0f, 0.0f, -1.0f, 10.0f);
    glMatrixMode(GL_MODELVIEW);


    // Load image
    int w;
    int h;
    int comp;
    unsigned char* image = stbi_load(tileset, &w, &h, &comp, STBI_rgb_alpha);
    if(!image) {
        fprintf(stderr, "Failed to load texture\n");
        return -1;
    }

    // Load tile maps
    load_map(&map16, tilemap16);
    printf("%u, %u\n", map16.tile_width, map16.num_tiles);
    load_map(&map128, tilemap128);
    printf("%u, %u\n", map128.tile_width, map128.num_tiles);

    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    float border_color[] = { 0.815f, 0.062f, 0.878f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    if(comp == 3)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    else if(comp == 4)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(image);

    tilew = 8.0 / ((GLdouble)w);
    tileh = 8.0 / ((GLdouble)h);

    
    while(!glfwWindowShouldClose(window))
    {
        /* glBindTexture(GL_TEXTURE_2D, texture); */
        /* glLoadIdentity(); */
        /* glPushMatrix(); */
        /* glTranslatef(250 - (w >> 1), 250 - (h >> 1), 0); */
        /* glBegin(GL_QUADS); */
        /* glColor3f(1, 1, 1); */
        /* glTexCoord2d(0, 0); */
        /* glVertex2d(0, 0); */
        /* glTexCoord2d(1, 0); */
        /* glVertex2d((double)w, 0); */
        /* glTexCoord2d(1, 1); */
        /* glVertex2d((double)w, (double)h); */
        /* glTexCoord2d(0, 1); */
        /* glVertex2d(0, (double)h); */
        /* glEnd(); */
        /* glPopMatrix(); */

        timer--;
        if(timer == 0) {
            cur = (cur + 1) % map128.num_tiles;
            timer = 60;
        }
        draw_map_tile();
        
        glfwSwapBuffers(window);
        glfwPollEvents();    
    }

    glfwTerminate();

    free_resources();
    return 0;
}
