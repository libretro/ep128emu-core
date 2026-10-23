#ifndef EGG_D3_SHOWCASE_DATA_H
#define EGG_D3_SHOWCASE_DATA_H
#include <stdint.h>
/* Elite showcase immutable model data. New ships need only another model
 * descriptor, face/edge lists and colour tables; no new renderer. */
typedef struct { int16_t x,y,z; } d3_vec3_t;
typedef struct { uint8_t count, vertex[7]; } d3_face_t;
typedef struct { uint8_t a,b,face_a,face_b; } d3_edge_t;
typedef struct {
 const d3_vec3_t *vertices;
 const d3_face_t *faces;
 const d3_edge_t *edges;
 const d3_vec3_t *normals; /* null for original per-triangle Cobra culling */
 const uint8_t *enterprise_palette;
 const uint8_t *tvc_palette;
 uint8_t vertex_count, face_count, edge_count;
 int16_t camera, scale_x, scale_y, centre_y_offset;
 uint8_t kind;
} d3_model_t;
#define D3_COBRA 0u
#define D3_ASP   1u
#define D3_SIDEWINDER 2u
#define D3_VIPER 3u
#define D3_KRAIT 4u
#define D3_MODEL_COUNT 5u
#define D3_PHASE_TICKS 1500u

static const d3_vec3_t d3_cobra_vertices[28] = {
    {  32,   0,  76 }, { -32,   0,  76 }, {   0,  26,  24 },
    {-120,  -3,  -8 }, { 120,  -3,  -8 }, { -88,  16, -40 },
    {  88,  16, -40 }, { 128,  -8, -40 }, {-128,  -8, -40 },
    {   0,  26, -40 }, { -32, -24, -40 }, {  32, -24, -40 },
    { -36,   8, -40 }, {  -8,  12, -40 }, {   8,  12, -40 },
    {  36,   8, -40 }, {  36, -12, -40 }, {   8, -16, -40 },
    {  -8, -16, -40 }, { -36, -12, -40 }, {   0,   0,  76 },
    {   0,   0,  90 }, { -80,  -6, -40 }, { -80,   6, -40 },
    { -88,   0, -40 }, {  80,   6, -40 }, {  88,   0, -40 },
    {  80,  -6, -40 }
};

static const d3_face_t d3_cobra_faces[13] = {
    {3, { 0, 1, 2 }},
    {3, { 1, 5, 2 }},
    {3, { 0, 2, 6 }},
    {3, { 1, 3, 5 }},
    {3, { 0, 6, 4 }},
    {3, { 2, 5, 9 }},
    {3, { 2, 9, 6 }},
    {3, { 3, 8, 5 }},
    {3, { 4, 6, 7 }},
    {7, { 9, 5, 8, 10, 11, 7, 6 }},
    {4, { 1, 10, 8, 3 }},
    {4, { 0, 11, 10, 1 }},
    {4, { 0, 4, 7, 11 }}
};

static const uint8_t d3_cobra_ep[13]={4,7,7,5,1,6,5,3,3,4,5,6,1};
static const uint8_t d3_cobra_tvc[13]={15,13,13,9,5,11,9,3,3,14,9,11,5};

static const d3_vec3_t d3_asp_vertices[19] = {
    {    0,  -18,    0 },
    {    0,   -9,  -45 },
    {   43,    0,  -45 },
    {   69,   -3,    0 },
    {   43,  -14,   28 },
    {  -43,    0,  -45 },
    {  -69,   -3,    0 },
    {  -43,  -14,   28 },
    {   26,   -7,   73 },
    {  -26,   -7,   73 },
    {   43,   14,   28 },
    {  -43,   14,   28 },
    {    0,    9,  -45 },
    {  -17,    0,  -45 },
    {   17,    0,  -45 },
    {    0,   -4,  -45 },
    {    0,    4,  -45 },
    {    0,   -7,   73 },
    {    0,   -7,   83 }
};

static const d3_face_t d3_asp_faces[12] = {
    { 5u, { 0, 4, 8, 9, 7 } },
    { 5u, { 0, 1, 2, 3, 4 } },
    { 5u, { 0, 1, 5, 6, 7 } },
    { 3u, { 10, 12, 11 } },
    { 4u, { 8, 9, 11, 10 } },
    { 4u, { 5, 6, 11, 12 } },
    { 4u, { 2, 3, 10, 12 } },
    { 3u, { 3, 8, 4 } },
    { 3u, { 6, 9, 7 } },
    { 3u, { 3, 8, 10 } },
    { 3u, { 6, 9, 11 } },
    { 4u, { 1, 2, 12, 5 } }
};

static const d3_vec3_t d3_asp_normals[12] = {
    {    0,  -35,    5 },
    {    8,  -38,   -7 },
    {   -8,  -38,   -7 },
    {    0,   24,   -1 },
    {    0,   43,   19 },
    {   -6,   28,   -2 },
    {    6,   28,   -2 },
    {   59,  -64,   31 },
    {  -59,  -64,   31 },
    {   80,   46,   50 },
    {  -80,   46,   50 },
    {    0,    0,  -90 }
};

static const d3_edge_t d3_asp_edges[28] = {
    {    0,    1,    2,    1 },
    {    0,    4,    1,    0 },
    {    0,    7,    2,    0 },
    {    1,    2,   11,    1 },
    {    2,    3,    6,    1 },
    {    3,    8,    9,    7 },
    {    8,    9,    4,    0 },
    {    6,    9,   10,    8 },
    {    5,    6,    5,    2 },
    {    1,    5,   11,    2 },
    {    3,    4,    7,    1 },
    {    4,    8,    7,    0 },
    {    6,    7,    8,    2 },
    {    7,    9,    8,    0 },
    {    2,   12,   11,    6 },
    {    5,   12,   11,    5 },
    {   10,   12,    6,    3 },
    {   11,   12,    5,    3 },
    {   10,   11,    4,    3 },
    {    6,   11,   10,    5 },
    {    9,   11,   10,    4 },
    {    3,   10,    9,    6 },
    {    8,   10,    9,    4 },
    {   13,   15,   11,   11 },
    {   15,   14,   11,   11 },
    {   14,   16,   11,   11 },
    {   16,   13,   11,   11 },
    {   18,   17,    4,    0 }
};

static const uint8_t d3_asp_ep[12]={4,5,6,3,7,1,5,3,6,4,7,0};
static const uint8_t d3_asp_tvc[12]={15,9,11,3,13,5,9,3,11,15,13,0};

/* Additional original BBC Micro disc Elite blueprints, ship_blueprints_p.
 * Edges and normals transcribed from Mark Moxon's documented original source;
 * hull face cycles follow the corresponding edge/face adjacency.
 * Additional original interior details remain represented by their edges.
 * Sources:
 * https://elite.bbcelite.com/disc/ship_blueprints_p/variable/ship_sidewinder.html
 * https://elite.bbcelite.com/disc/ship_blueprints_p/variable/ship_viper.html
 * https://elite.bbcelite.com/disc/ship_blueprints_p/variable/ship_krait.html
 */
static const d3_vec3_t d3_sidewinder_vertices[10] = {
 {-32,0,36}, {32,0,36}, {64,0,-28}, {-64,0,-28}, {0,16,-28},
 {0,-16,-28}, {-12,6,-28}, {12,6,-28}, {12,-6,-28}, {-12,-6,-28}
};
static const d3_face_t d3_sidewinder_faces[7] = {
 {3,{0,1,4}}, {3,{0,4,3}}, {3,{1,2,4}}, {4,{3,4,2,5}},
 {3,{0,3,5}}, {3,{0,5,1}}, {3,{1,5,2}}
};
static const d3_vec3_t d3_sidewinder_normals[7] = {
 {0,32,8}, {-12,47,6}, {12,47,6}, {0,0,-112},
 {-12,-47,6}, {0,-32,8}, {12,-47,6}
};
static const d3_edge_t d3_sidewinder_edges[15] = {
 {0,1,0,5}, {1,2,2,6}, {1,4,0,2}, {0,4,0,1},
 {0,3,1,4}, {3,4,1,3}, {2,4,2,3}, {3,5,3,4},
 {2,5,3,6}, {1,5,5,6}, {0,5,4,5}, {6,7,3,3},
 {7,8,3,3}, {6,9,3,3}, {8,9,3,3}
};
static const uint8_t d3_sidewinder_ep[7]={7,5,6,3,1,4,3};
static const uint8_t d3_sidewinder_tvc[7]={13,9,11,3,5,15,14};

static const d3_vec3_t d3_viper_vertices[15] = {
 {0,0,72}, {0,16,24}, {0,-16,24}, {48,0,-24}, {-48,0,-24},
 {24,-16,-24}, {-24,-16,-24}, {24,16,-24}, {-24,16,-24},
 {-32,0,-24}, {32,0,-24}, {8,8,-24}, {-8,8,-24},
 {-8,-8,-24}, {8,-8,-24}
};
static const d3_face_t d3_viper_faces[7] = {
 {3,{1,7,8}}, {4,{0,1,8,4}}, {4,{0,3,7,1}},
 {4,{0,4,6,2}}, {4,{0,2,5,3}}, {3,{2,6,5}},
 {6,{7,3,5,6,4,8}}
};
static const d3_vec3_t d3_viper_normals[7] = {
 {0,32,0}, {-22,33,11}, {22,33,11}, {-22,-33,11},
 {22,-33,11}, {0,-32,0}, {0,0,-48}
};
static const d3_edge_t d3_viper_edges[20] = {
 {0,3,2,4}, {0,1,1,2}, {0,2,3,4}, {0,4,1,3},
 {1,7,0,2}, {1,8,0,1}, {2,5,4,5}, {2,6,3,5},
 {7,8,0,6}, {5,6,5,6}, {4,8,1,6}, {4,6,3,6},
 {3,7,2,6}, {3,5,6,4}, {9,12,6,6}, {9,13,6,6},
 {10,11,6,6}, {10,14,6,6}, {11,14,6,6}, {12,13,6,6}
};
static const uint8_t d3_viper_ep[7]={7,5,6,3,1,4,2};
static const uint8_t d3_viper_tvc[7]={13,9,11,3,5,15,14};

static const d3_vec3_t d3_krait_vertices[17] = {
 {0,0,96}, {0,18,-48}, {0,-18,-48}, {90,0,-3}, {-90,0,-3},
 {90,0,87}, {-90,0,87}, {0,5,53}, {0,7,38}, {-18,7,19},
 {18,7,19}, {18,11,-39}, {18,-11,-39}, {36,0,-30},
 {-18,11,-39}, {-18,-11,-39}, {-36,0,-30}
};
static const d3_face_t d3_krait_faces[6] = {
 {3,{0,3,1}}, {3,{0,2,3}}, {3,{0,4,2}},
 {3,{0,1,4}}, {3,{1,3,2}}, {3,{1,2,4}}
};
static const d3_vec3_t d3_krait_normals[6] = {
 {3,24,3}, {3,-24,3}, {-3,-24,3}, {-3,24,3},
 {38,0,-77}, {-38,0,-77}
};
static const d3_edge_t d3_krait_edges[21] = {
 {0,1,3,0}, {0,2,2,1}, {0,3,1,0}, {0,4,3,2},
 {1,4,5,3}, {4,2,5,2}, {2,3,4,1}, {3,1,4,0},
 {3,5,1,0}, {4,6,3,2}, {1,2,5,4}, {7,10,0,0},
 {8,10,0,0}, {7,9,3,3}, {8,9,3,3}, {11,13,4,4},
 {13,12,4,4}, {12,11,4,4}, {14,15,5,5},
 {15,16,5,5}, {16,14,5,5}
};
static const uint8_t d3_krait_ep[6]={7,5,6,1,3,4};
static const uint8_t d3_krait_tvc[6]={13,9,11,5,3,15};

static const d3_model_t d3_models[D3_MODEL_COUNT] = {
    {d3_cobra_vertices,d3_cobra_faces,0,0,d3_cobra_ep,d3_cobra_tvc,
     28,13,0,380,550,275,-6,D3_COBRA},
    {d3_asp_vertices,d3_asp_faces,d3_asp_edges,d3_asp_normals,d3_asp_ep,d3_asp_tvc,
     19,12,28,370,860,405,-4,D3_ASP},
    {d3_sidewinder_vertices,d3_sidewinder_faces,d3_sidewinder_edges,d3_sidewinder_normals,
     d3_sidewinder_ep,d3_sidewinder_tvc,10,7,15,300,850,425,-4,D3_SIDEWINDER},
    {d3_viper_vertices,d3_viper_faces,d3_viper_edges,d3_viper_normals,
     d3_viper_ep,d3_viper_tvc,15,7,20,300,850,425,-4,D3_VIPER},
    {d3_krait_vertices,d3_krait_faces,d3_krait_edges,d3_krait_normals,
     d3_krait_ep,d3_krait_tvc,17,6,21,370,665,260,-4,D3_KRAIT}
};


#endif
