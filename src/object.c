#include "object.h"
#include "screen.h"
#include "util.h"
#include "render.h"

#include <stdio.h>
#include <inline_c.h>

#define PolyLoadColor1(pv, pbytes, pb)  \
    {                                  \
        pv->r0 = get_byte(pbytes, pb); \
        pv->g0 = get_byte(pbytes, pb); \
        pv->b0 = get_byte(pbytes, pb); \
    }

#define PolyLoadColor3(pv, pbytes, pb)  \
    {                                  \
        pv->r0 = get_byte(pbytes, pb); \
        pv->g0 = get_byte(pbytes, pb); \
        pv->b0 = get_byte(pbytes, pb); \
        pv->r1 = get_byte(pbytes, pb); \
        pv->g1 = get_byte(pbytes, pb); \
        pv->b1 = get_byte(pbytes, pb); \
        pv->r2 = get_byte(pbytes, pb); \
        pv->g2 = get_byte(pbytes, pb); \
        pv->b2 = get_byte(pbytes, pb); \
    }


#define PolyLoadColor4(pv, pbytes, pb)  \
    {                                  \
        pv->r0 = get_byte(pbytes, pb); \
        pv->g0 = get_byte(pbytes, pb); \
        pv->b0 = get_byte(pbytes, pb); \
        pv->r1 = get_byte(pbytes, pb); \
        pv->g1 = get_byte(pbytes, pb); \
        pv->b1 = get_byte(pbytes, pb); \
        pv->r2 = get_byte(pbytes, pb); \
        pv->g2 = get_byte(pbytes, pb); \
        pv->b2 = get_byte(pbytes, pb); \
        pv->r3 = get_byte(pbytes, pb); \
        pv->g3 = get_byte(pbytes, pb); \
        pv->b3 = get_byte(pbytes, pb); \
    }

#define PolyLoadIV3(pv, pbytes, pb)        \
    {                                      \
        pv->iv0 = get_short_be(pbytes, pb); \
        pv->iv1 = get_short_be(pbytes, pb); \
        pv->iv2 = get_short_be(pbytes, pb); \
    }

#define PolyLoadIV4(pv, pbytes, pb)        \
    {                                      \
        pv->iv0 = get_short_be(pbytes, pb); \
        pv->iv1 = get_short_be(pbytes, pb); \
        pv->iv2 = get_short_be(pbytes, pb); \
        pv->iv3 = get_short_be(pbytes, pb); \
    }

#define PolyLoadIN1(pv, pbytes, pb)             \
    pv->in0 = get_short_be(pbytes, pb);

#define PolyLoadIN3(pv, pbytes, pb)             \
    {                                           \
        pv->in0 = get_short_be(pbytes, pb);     \
        pv->in1 = get_short_be(pbytes, pb);     \
        pv->in2 = get_short_be(pbytes, pb);     \
    }

#define PolyLoadIN4(pv, pbytes, pb)             \
    {                                           \
        pv->in0 = get_short_be(pbytes, pb);     \
        pv->in1 = get_short_be(pbytes, pb);     \
        pv->in2 = get_short_be(pbytes, pb);     \
        pv->in3 = get_short_be(pbytes, pb);     \
    }
    
void
load_polygon(ObjPolygon *p, uint8_t *bytes, uint32_t *b)
{
    p->ftype = get_byte(bytes, b);
    switch(p->ftype) {
    case TYPE_F3: {
        OBJF3 *info = screen_alloc(sizeof(OBJF3));
        PolyLoadColor1(info, bytes, b);
        PolyLoadIV3(info, bytes, b);
        PolyLoadIN1(info, bytes, b);
        p->info = (uint8_t *)info;
    } break;
    case TYPE_G3: {
        OBJG3 *info = screen_alloc(sizeof(OBJG3));
        PolyLoadColor3(info, bytes, b);
        PolyLoadIV3(info, bytes, b);
        PolyLoadIN3(info, bytes, b);
        p->info = (uint8_t *)info;
    } break;
    case TYPE_F4: {
        OBJF4 *info = screen_alloc(sizeof(OBJF4));
        PolyLoadColor1(info, bytes, b);
        PolyLoadIV4(info, bytes, b);
        PolyLoadIN1(info, bytes, b);
        p->info = (uint8_t *)info;
    } break;
    case TYPE_G4: {
        OBJG4 *info = screen_alloc(sizeof(OBJG4));
        PolyLoadColor4(info, bytes, b);
        PolyLoadIV4(info, bytes, b);
        PolyLoadIN4(info, bytes, b);
        p->info = (uint8_t *)info;
    } break;
    default:
        printf("WARNING! UNIMPLEMENTED POLYGON TYPE %d\n", p->ftype);
        p->info = NULL;
        break;
    };
}


void
load_model(Model *m, const char *filename)
{
    m->pos.vx = 0;
    m->pos.vy = 0;
    m->pos.vz = 0;
    m->rot.vx = 0;
    m->rot.vy = 0;
    m->rot.vz = 0;
    m->scl.vx = ONE;
    m->scl.vy = ONE;
    m->scl.vz = ONE;

    uint8_t *bytes;
    uint32_t b, length;
    bytes = file_read(filename, &length);
    if(bytes == NULL) {
        printf("Error reading MDL file %s from the CD.\n", filename);
        return;
    }

    b = 0;

    m->num_vertices = get_short_be(bytes, &b);
    m->num_normals  = get_short_be(bytes, &b);
    m->num_polygons = get_short_be(bytes, &b);

    m->vertices = screen_alloc(sizeof(VECTOR) * m->num_vertices);
    m->normals  = screen_alloc(sizeof(VECTOR) * m->num_normals);
    m->polygons = screen_alloc(sizeof(ObjPolygon) * m->num_polygons);

    for(uint16_t i = 0; i < m->num_vertices; i++) {
        SVECTOR *v = &m->vertices[i];
        v->vx = (int16_t)(get_long_be(bytes, &b) >> 2);
        v->vy = (int16_t)(get_long_be(bytes, &b) >> 2);
        v->vz = (int16_t)(get_long_be(bytes, &b) >> 2);
    }

    for(uint16_t i = 0; i < m->num_normals; i++) {
        SVECTOR *n = &m->normals[i];
        n->vx = (int16_t)(get_long_be(bytes, &b) >> 2);
        n->vy = (int16_t)(get_long_be(bytes, &b) >> 2);
        n->vz = (int16_t)(get_long_be(bytes, &b) >> 2);
    }

    for(uint16_t i = 0; i < m->num_polygons; i++) {
        ObjPolygon *p = &m->polygons[i];
        load_polygon(p, bytes, &b);
    }

    printf("Model %s loaded.\n", filename);
}


void
render_model(Model *m)
{
    RotMatrix(&m->rot, &m->world);
    TransMatrix(&m->world, &m->pos);
    ScaleMatrix(&m->world, &m->scl);
    gte_SetRotMatrix(&m->world);
    gte_SetTransMatrix(&m->world);

    for(uint16_t i = 0; i < m->num_polygons; i++) {
        int nclip = -1, otz = -1;
        ObjPolygon *op = &m->polygons[i];

        switch(op->ftype) {
        case TYPE_F3: {
            OBJF3 *info = (OBJF3 *)op->info;
            POLY_F3 *poly = (POLY_F3 *)get_next_prim();
            setPolyF3(poly);
            nclip = RotAverageNclip3(
                &m->vertices[info->iv0],
                &m->vertices[info->iv1],
                &m->vertices[info->iv2],
                (uint32_t *)&poly->x0,
                (uint32_t *)&poly->x1,
                (uint32_t *)&poly->x2,
                &otz);
            if((nclip < 0) || (otz < 0) || (otz >= OT_LENGTH))
                continue;

            setRGB0(poly, info->r0, info->g0, info->b0);
            sort_prim(poly, otz);
            increment_prim(sizeof(POLY_F3));
        } break;
        case TYPE_G3: {
            OBJG3 *info = (OBJG3 *)op->info;
            POLY_G3 *poly = (POLY_G3 *)get_next_prim();
            setPolyG3(poly);
            nclip = RotAverageNclip3(
                &m->vertices[info->iv0],
                &m->vertices[info->iv1],
                &m->vertices[info->iv2],
                (uint32_t *)&poly->x0,
                (uint32_t *)&poly->x1,
                (uint32_t *)&poly->x2,
                &otz);
            if((nclip < 0) || (otz < 0) || (otz >= OT_LENGTH))
                continue;

            setRGB0(poly, info->r0, info->g0, info->b0);
            setRGB1(poly, info->r1, info->g1, info->b1);
            setRGB2(poly, info->r2, info->g2, info->b2);
            sort_prim(poly, otz);
            increment_prim(sizeof(POLY_G3));
        } break;
        case TYPE_F4: {
            OBJF4 *info = (OBJF4 *)op->info;
            POLY_F4 *poly = (POLY_F4 *)get_next_prim();
            setPolyF4(poly);
            nclip = RotAverageNclip4(
                &m->vertices[info->iv0],
                &m->vertices[info->iv1],
                &m->vertices[info->iv2],
                &m->vertices[info->iv3],
                (uint32_t *)&poly->x0,
                (uint32_t *)&poly->x1,
                (uint32_t *)&poly->x2,
                (uint32_t *)&poly->x3,
                &otz);
            if((nclip < 0) || (otz < 0) || (otz >= OT_LENGTH))
                continue;

            setRGB0(poly, info->r0, info->g0, info->b0);
            sort_prim(poly, otz);
            increment_prim(sizeof(POLY_F4));
        } break;
        case TYPE_G4: {
            OBJG4 *info = (OBJG4 *)op->info;
            POLY_G4 *poly = (POLY_G4 *)get_next_prim();
            setPolyG4(poly);
            nclip = RotAverageNclip4(
                &m->vertices[info->iv0],
                &m->vertices[info->iv1],
                &m->vertices[info->iv2],
                &m->vertices[info->iv3],
                (uint32_t *)&poly->x0,
                (uint32_t *)&poly->x1,
                (uint32_t *)&poly->x2,
                (uint32_t *)&poly->x3,
                &otz);
            if((nclip < 0) || (otz < 0) || (otz >= OT_LENGTH))
                continue;

            setRGB0(poly, info->r0, info->g0, info->b0);
            setRGB1(poly, info->r1, info->g1, info->b1);
            setRGB2(poly, info->r2, info->g2, info->b2);
            setRGB3(poly, info->r3, info->g3, info->b3);
            sort_prim(poly, otz);
            increment_prim(sizeof(POLY_G4));
        } break;
        default:
            // Unused
            break;
        };
    }
}
