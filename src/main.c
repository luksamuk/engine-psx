#include <sys/types.h>
#include <stdio.h>
#include <libgte.h>
#include <libgpu.h>
#include <libetc.h>
#include <libpad.h>

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

// Estructuras para el Doble Buffer de la PS1 (Evita parpadeos)
DISPENV disp;
DRAWENV draw;
int db = 0;

// Variables de movimiento de Leon (Estilo Tanque RE4)
u_long pad_state;
int leon_x = 0;
int leon_z = 1000;    // Distancia inicial de la camara
int leon_angulo = 0;  // Rotacion de la camara al hombro

// Variables nativas del procesador geometrico 3D de la Play 1
SVECTOR cam_rot = { 0, 0, 0 };
VECTOR  cam_pos = { 0, 0, 0 };
MATRIX  matriz_vista;

void init_game_system() {
    ResetGraph(0);
    
    // Inicializamos el doble buffer en la memoria de video (VRAM)
    SetDefDispEnv(&disp, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    SetDefDrawEnv(&draw, 0, 240, SCREEN_WIDTH, SCREEN_HEIGHT);
    SetDefDispEnv(&disp, 0, 240, SCREEN_WIDTH, SCREEN_HEIGHT);
    SetDefDrawEnv(&draw, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    
    // Fondo negro puro (Estilo Survival Horror)
    setRGB0(&draw, 0, 0, 0);
    setRGB0(&draw, 0, 0, 0);
    
    PutDispEnv(&disp[db]);
    PutDrawEnv(&draw[db]);
    SetDispMask(1);
    
    // Encendemos el chip de calculo 3D (GTE) y los controles
    InitGeom();
    PadInit(0);
    
    // Seteamos la perspectiva del lente de la camara
    SetGeomOffset(SCREEN_WIDTH/2, SCREEN_HEIGHT/2);
    SetGeomScreen(512); 
}

void procesar_input() {
    // Leemos el Joystick en el Puerto 1
    pad_state = PadRead(0);
    
    // Configuracion de botones de RE4 Mobile
    if (pad_state & PADLup) {
        leon_z -= 20; // Camina hacia adelante en profundidad
    }
    if (pad_state & PADLdown) {
        leon_z += 20; // Camina hacia atras
    }
    if (pad_state & PADLleft) {
        leon_angulo -= 32; // Gira sobre su eje a la izquierda
    }
    if (pad_state & PADLright) {
        leon_angulo += 32; // Gira sobre su eje a la derecha
    }
}

void actualizar_camara_3d() {
    // Vinculamos tus variables de tanque con las matrices de la PS1
    cam_rot.vy = leon_angulo;
    cam_pos.vx = leon_x;
    cam_pos.vz = leon_z;
    cam_pos.vy = -100; // Altura fija al hombro de Leon
    
    RotMatrix(&cam_rot, &matriz_vista);
    TransMatrix(&matriz_vista, &cam_pos);
    SetTransMatrix(&matriz_vista);
}

void dibujar_piso_pueblo() {
    POLY_F4 piso;
    long p, flag;
    
    // Coordenadas de los 4 extremos del suelo en el mapa 3D
    SVECTOR v0 = { -500, 0, -500 };
    SVECTOR v1 = {  500, 0, -500 };
    SVECTOR v2 = { -500, 0,  500 };
    SVECTOR v3 = {  500, 0,  500 };
    
    SetPolyF4(&piso);
    setRGB0(&piso, 128, 128, 128); // Color gris para el suelo
    
    // El chip GTE procesa la perspectiva 3D
    RotTransPers(&v0, (long*)&piso.x0, &p, &flag);
    RotTransPers(&v1, (long*)&piso.x1, &p, &flag);
    RotTransPers(&v2, (long*)&piso.x2, &p, &flag);
    RotTransPers(&v3, (long*)&piso.x3, &p, &flag);
    
    // Mandamos el poligono directo a dibujar en la pantalla
    DrawPrim(&piso);
}

int main() {
    init_game_system();
    
    // Bucle principal del juego (Game Loop)
    while (1) {
        procesar_input();
        actualizar_camara_3d();
        
        PutDrawEnv(&draw[db]);
        
        // Renderizamos nuestro piso tridimensional
        dibujar_piso_pueblo();
        
        DrawSync(0);
        VSync(0);
        
        // Intercambio de buffers para evitar parpadeos
        db = !db;
        PutDispEnv(&disp[db]);
    }
    return 0;
}
