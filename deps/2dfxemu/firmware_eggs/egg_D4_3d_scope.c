#include "egg_D4_3d_scope.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "2dfx.h"                /* Safe visible-area geometry constants. */
#include "egg_D3_elite_showcase.h" /* Reuse proven Q12 sine table, no D3 edits. */
#include "egg_D3_elite_showcase_data.h" /* Actual Asp Mk II mesh from the D3 showcase. */
#include "twodfx_api.h"
#include "twodfx_egg.h"
#include "pico/time.h"

/*
 * D4 cooperative 3D geometry + actual 2DPT rasterization benchmark.
 *
 * All F8 status bits keep their normal public meanings while D4 runs:
 * STATUS0 is the standard Core1 render-time probe; STATUS6 is READY;
 * STATUS7 is the most recently published sprite-collision result.
 * Preparatory timing metrics remain debugger-readable g_d4_* counters.
 * No diagnostic egg code drives any F8 status output.
 *
 * D4 deliberately keeps the model in Core0 SRAM and emits actual LINE/FILL_TRIANGLE
 * commands through the unchanged 2DPT API.  It is a pipeline/scheduling benchmark,
 * not a public 3D renderer or a Z-buffer benchmark.
 */
#define D4_SIDES              8u
#define D4_MAX_RINGS          12u
#define D4_MAX_VERTS          (D4_SIDES * D4_MAX_RINGS)
#define D4_MAX_FACES          (D4_SIDES * (D4_MAX_RINGS - 1u))
#define D4_MODE_TICKS         400u /* 8 seconds at 50 FPS */
#define D4_MAX_SLOT           254u
#define D4_NEAR_Z             96
#define D4_FIRST_Z            180
#define D4_CRUISE_SPEED       15u
#define D4_WILD_SPEED_EXTRA   19u /* Up to 34 world-Z units per completed frame. */
#define D4_CAMERA_BLEND_STEP  128u /* ~0.64s full-strength transitions at 50 FPS. */
/* Far planes always lie BEFORE the newly recycled outermost ring.
 * The surface can therefore be clipped at a continuous, stationary distance,
 * without ever creating a ring at radius zero (v1.0.5j's false vanishing hub).
 */
#define D4_FAR_4             920
#define D4_FAR_8            2070
#define D4_FAR_12           2490
#define D4_LOOKAHEAD         550
#define D4_POLY_CAP           12u
/* A chase-view Asp Mk II rendered in FOREGROUND. Ship Z stays safely beyond
 * the near plane; pilot steering keeps the craft in the visible near tunnel.
 * The pilot is an avoidance animation, not a physical collision detector. */
#define D4_SHIP_DEPTH         410  /* Fixed A/B option and initial distance. */
#define D4_SHIP_NEAR_DEPTH    315
#define D4_SHIP_FAR_DEPTH     575
#define D4_SHIP_Z_STEP_MAX    5    /* Cinematic dolly cannot jump between frames. */
#define D4_SHIP_MOVE_Q8_MAX  (5*256) /* Fractional screen-space pursuit. */
#define D4_SHIP_MAX_FACES     13u
/* D4 only: project to measured BASIC-safe viewport, clip before 10/9-bit DPBs. */
#define D4_SCOPE_BG           TWODFX_LAYER_BACKGROUND
#define D4_SCOPE_FG           TWODFX_LAYER_FOREGROUND

/* Private worker lifecycle; never grants ownership of public STATUS pins. */
static bool d4_worker_active = false;

/* Debugger-readable last COMPLETED preparation sample. In microseconds. */
volatile uint32_t g_d4_prep_wall_us = 0u;
volatile uint32_t g_d4_prep_cpu_us = 0u;
volatile uint32_t g_d4_prep_steps = 0u;
volatile uint32_t g_d4_prep_max_step_us = 0u;
volatile uint32_t g_d4_completed_scenes = 0u;
volatile uint32_t g_d4_faults = 0u;
volatile uint32_t g_d4_frame_phase = 0u;
volatile uint32_t g_d4_mode = 0u;
volatile uint32_t g_d4_visible_faces = 0u;
volatile uint32_t g_d4_near_clips = 0u;
volatile uint32_t g_d4_screen_clips = 0u;
volatile uint32_t g_d4_emitted_lines = 0u;
volatile uint32_t g_d4_emitted_triangles = 0u;
/* Debugger-readable last completed scene / camera values (not extra I/O). */
volatile int32_t g_d4_cam_world_z = 0;
volatile int32_t g_d4_cam_x = 0, g_d4_cam_y = 0;
volatile int32_t g_d4_yaw_s = 0, g_d4_pitch_s = 0;
volatile uint32_t g_d4_far_clips = 0u, g_d4_far_rim_lines = 0u;
volatile uint32_t g_d4_camera_profile = 0u, g_d4_camera_strength = 0u;
volatile uint32_t g_d4_ship_triangles = 0u, g_d4_ship_lines = 0u;
volatile uint32_t g_d4_ship_visible_faces = 0u, g_d4_ship_enabled = 0u;
volatile int32_t g_d4_ship_x = 0, g_d4_ship_y = 0;
volatile int32_t g_d4_ship_depth = D4_SHIP_DEPTH;
volatile uint32_t g_d4_ship_engine_visible = 0u;
volatile uint32_t g_d4_ship_engine_triangles = 0u, g_d4_ship_plume_triangles = 0u;

typedef struct { int32_t x, y, z; } d4_vertex_t;
typedef struct { int32_t x, y; } d4_point_t;
typedef struct { uint8_t ring, side; int32_t depth; } d4_face_t;
typedef enum {
    D4_READY=0, D4_PREP, D4_VERTEX, D4_FACE, D4_RESET_BG, D4_RESET_FG,
    D4_SET_BG_COLOUR, D4_EMIT_GEOMETRY, D4_FG_COLOUR,
    D4_SHIP_PREP, D4_SHIP_VERTEX, D4_SHIP_FACE, D4_SHIP_DRAW,
    D4_SHIP_PLUME_COLOUR, D4_SHIP_PLUME,
    D4_SHIP_ENGINE_FILL_COLOUR, D4_SHIP_ENGINE_FILL, D4_SHIP_ENGINE_RIM_COLOUR,
    D4_SHIP_ENGINE_RIM, D4_SHIP_ENGINE_CORE_COLOUR, D4_SHIP_ENGINE_CORE,
    D4_FG_HUD_COLOUR,
    D4_FG_PIPS, D4_FG_CAMERA_PIPS, D4_FG_BAR_COLOUR, D4_FG_BARS, D4_FINISH
} d4_state_t;

static struct {
    d4_vertex_t verts[D4_MAX_VERTS];
    d4_face_t faces[D4_MAX_FACES];
    uint8_t ordered[D4_MAX_FACES];
    uint8_t visible_count;
    uint8_t mode, rings, camera_profile, cam_pip;
    bool forced_mode, tvc, ship_enabled, ship_zoom_enabled;
    uint32_t tick;
    uint16_t index, op, slot, phase_tick;
    uint16_t ring_spacing;
    uint16_t ring_phase;
    uint32_t cycle;
    int32_t travel_z, far_z, travel_distance;
    uint16_t camera_strength_q12;
    int32_t ring_path_x, ring_path_y, ring_world_z, ring_scale;
    int32_t ring_twist_s, ring_twist_c;
    d4_point_t face_polygon[D4_POLY_CAP];
    uint8_t face_polygon_count;
    bool face_prepared;
    int32_t view_xmin, view_xmax, view_ymin, view_ymax;
    uint8_t emit_part, current_colour, pip;
    uint8_t bar;
    /* Built from the same completed camera pose as the tunnel vertices. */
    d4_vertex_t ship_cam[19];
    d4_point_t ship_screen[19];
    uint8_t ship_order[D4_SHIP_MAX_FACES],ship_visible_count;
    int32_t ship_face_depth[D4_SHIP_MAX_FACES];
    int32_t ship_x,ship_y,ship_x_q8,ship_y_q8,ship_vx_q8,ship_vy_q8;
    int32_t ship_depth,ship_roll_s,ship_roll_c,ship_pitch_s,ship_pitch_c;
    int32_t ship_yaw_s,ship_yaw_c,ship_world_centre_x,ship_world_centre_y;
    bool ship_engine_visible;
    uint8_t ship_engine_part,ship_engine_polygon_count;
    d4_point_t ship_engine_polygon[D4_POLY_CAP];
    d4_point_t ship_plume_polygon[D4_POLY_CAP];
    uint8_t ship_plume_count,ship_plume_part;
    d4_point_t ship_poly[D4_POLY_CAP];
    uint8_t ship_poly_count,ship_tri_count,ship_tri_index;
    uint8_t ship_edge,ship_stage,ship_index;
    d4_point_t ship_tris[D4_POLY_CAP - 2u][3];
    bool ship_prepared;
    int32_t yaw_s, yaw_c, pitch_s, pitch_c, cam_x, cam_y;
    int32_t centre_x, centre_y;
    uint32_t prep_start, cpu_us, steps, max_step_us;
    d4_state_t state;
    bool fault;
    uint32_t near_clips, far_clips, screen_clips, lines, triangles, far_rim_lines;
} d4;

static const int16_t ring_x[D4_SIDES] = {164, 116, 0, -116, -164, -116, 0, 116};
static const int16_t ring_y[D4_SIDES] = {0, 72, 102, 72, 0, -72, -102, -72};

/* Periodic but smoothly interpolated WORLD-SPACE terrain.  These waves have
 * periods of thousands of depth units, NOT a 128-frame camera phase.  Camera
 * follows this centreline, so the world does not jump when an 8-bit table
 * index wraps.  64-bit intermediates are limited to a single worker step. */
static int32_t d4_world_sin(int32_t z, int32_t period, uint16_t phase)
{
    const uint16_t p=(uint16_t)((int64_t)z*65536/period+phase);
    const uint8_t idx=(uint8_t)(p>>8);
    const uint32_t f=(uint8_t)p;
    return ((int32_t)d3_sine_q12[idx]*(int32_t)(256u-f)
          + (int32_t)d3_sine_q12[(uint8_t)(idx+1u)]*(int32_t)f)/256;
}

static void d4_world_centre(int32_t z,int32_t *x,int32_t *y)
{
    *x=(260*d4_world_sin(z,10000,0u)
       +85*d4_world_sin(z,21000,9000u))/4096;
    *y=(125*d4_world_sin(z,7800,11000u)
       +45*d4_world_sin(z,15500,38000u))/4096;
}

/* Integer-only, reproducible "gusts". Each target change is interpolated
 * with smoothstep; the camera never teleports when the hash changes segment.
 * Frames, not absolute ring indices, drive gust timing. All outputs Q12. */
static int32_t d4_gust_endpoint(uint32_t segment,uint32_t salt)
{
    uint32_t h=segment^salt;
    h^=h>>16; h*=0x7feb352du;
    h^=h>>15; h*=0x846ca68bu;
    h^=h>>16;
    return (int32_t)((h>>20)&4095u)*2-4095;
}

static int32_t d4_gust(uint32_t frame,uint32_t interval,uint32_t salt)
{
    const uint32_t section=frame/interval;
    const int32_t a=d4_gust_endpoint(section,salt);
    const int32_t b=d4_gust_endpoint(section+1u,salt);
    const int64_t t=(int64_t)(frame%interval)*4096/interval;
    const int32_t smooth=(int32_t)(t*t*(12288-2*t)/(4096ll*4096ll));
    return a+(int32_t)(((int64_t)(b-a)*smooth)/4096);
}

static int32_t d4_clamp_rotation(int32_t v,int32_t limit)
{
    return v>limit?limit:(v< -limit?-limit:v);
}


static bool d4_emit_ok(void)
{
    if (d4.slot < D4_MAX_SLOT) return true;
    d4.fault = true;
    return false;
}

static void d4_colour(uint8_t c)
{
    if (!d4_emit_ok()) return;
    twodfx_2d_set_color(D4_SCOPE_BG, (uint8_t)d4.slot++, c, 0u);
}

/* Unlike v1.0.5i there is no clamping of unprojected 3D points.
 * The near plane is clipped in CAMERA SPACE before perspective division.
 * All emitted endpoints are then clipped to the safe 2D viewport. */
static d4_vertex_t d4_depth_intersection(d4_vertex_t a,d4_vertex_t b,int32_t plane)
{
    const int32_t dz=b.z-a.z; /* Called only for a genuine straddle. */
    d4_vertex_t c;
    c.x=a.x+(int32_t)(((int64_t)(b.x-a.x)*(plane-a.z))/dz);
    c.y=a.y+(int32_t)(((int64_t)(b.y-a.y)*(plane-a.z))/dz);
    c.z=plane;
    return c;
}

static d4_point_t d4_project(d4_vertex_t v)
{
    /* Valid only AFTER near-plane clipping (z >= D4_NEAR_Z). */
    d4_point_t p;
    p.x = d4.centre_x + (int32_t)(((int64_t)v.x*365)/v.z);
    p.y = d4.centre_y - (int32_t)(((int64_t)v.y*300)/v.z);
    return p;
}

static uint8_t d4_outcode(d4_point_t a)
{
    return (uint8_t)((a.x<d4.view_xmin ? 1u : 0u) |
                     (a.x>d4.view_xmax ? 2u : 0u) |
                     (a.y<d4.view_ymin ? 4u : 0u) |
                     (a.y>d4.view_ymax ? 8u : 0u));
}

/* Clip 2D projected LINE to the screen. All divisions stay within signed
 * 64-bit range. The loop makes strictly monotonic outcode progress. */
static bool d4_screen_clip_line(d4_point_t *a, d4_point_t *b)
{
    for (unsigned pass=0;pass<10u;pass++) {
        const uint8_t ca=d4_outcode(*a), cb=d4_outcode(*b);
        if (!(ca|cb)) return true;
        if (ca&cb) return false;
        const uint8_t c=ca?ca:cb;
        d4_point_t i=*a;
        if (c&4u) {
            if (b->y==a->y) return false;
            i.x=a->x+(int32_t)((int64_t)(b->x-a->x)*(d4.view_ymin-a->y)/(b->y-a->y));
            i.y=d4.view_ymin;
        } else if (c&8u) {
            if (b->y==a->y) return false;
            i.x=a->x+(int32_t)((int64_t)(b->x-a->x)*(d4.view_ymax-a->y)/(b->y-a->y));
            i.y=d4.view_ymax;
        } else if (c&1u) {
            if (b->x==a->x) return false;
            i.y=a->y+(int32_t)((int64_t)(b->y-a->y)*(d4.view_xmin-a->x)/(b->x-a->x));
            i.x=d4.view_xmin;
        } else {
            if (b->x==a->x) return false;
            i.y=a->y+(int32_t)((int64_t)(b->y-a->y)*(d4.view_xmax-a->x)/(b->x-a->x));
            i.x=d4.view_xmax;
        }
        if (c==ca) *a=i; else *b=i;
    }
    return false; /* Defensive: never publish an unbounded endpoint. */
}

static void d4_line(uint8_t a, uint8_t b)
{
    if (!d4_emit_ok()) return;
    d4_vertex_t p=d4.verts[a], q=d4.verts[b];
    if ((p.z<D4_NEAR_Z && q.z<D4_NEAR_Z) ||
        (p.z>d4.far_z && q.z>d4.far_z)) return;
    if (p.z<D4_NEAR_Z) {
        p=d4_depth_intersection(p,q,D4_NEAR_Z); ++d4.near_clips;
    } else if (q.z<D4_NEAR_Z) {
        q=d4_depth_intersection(q,p,D4_NEAR_Z); ++d4.near_clips;
    }
    if (p.z>d4.far_z) {
        p=d4_depth_intersection(p,q,d4.far_z); ++d4.far_clips;
    } else if (q.z>d4.far_z) {
        q=d4_depth_intersection(q,p,d4.far_z); ++d4.far_clips;
    }
    d4_point_t x=d4_project(p), y=d4_project(q);
    const uint8_t was_outside=(uint8_t)(d4_outcode(x)|d4_outcode(y));
    if(!d4_screen_clip_line(&x,&y)) return;
    if(was_outside) ++d4.screen_clips;
    ++d4.lines;
    twodfx_2d_line(D4_SCOPE_BG, (uint8_t)d4.slot++,
                   (uint16_t)x.x,(uint16_t)x.y,
                   (uint16_t)y.x,(uint16_t)y.y,1u);
}

/* Sutherland-Hodgman polygon clipping in camera Z, then in 2D X/Y.
 * Convex tunnel quads can become a pentagon at the near plane and gain
 * at most four more vertices at screen edges. Kept in small SRAM arrays. */
/* Two camera-space depth planes.  Convex quad clipped to one plane has at
 * most five points; after both planes there are at most six. */
static uint8_t d4_polygon_depth_clip(const d4_vertex_t *in,unsigned n,
                                     d4_vertex_t *out,int32_t plane,bool far)
{
    uint8_t used=0;
    d4_vertex_t previous=in[n-1u];
    bool was_inside=far?(previous.z<=plane):(previous.z>=plane);
    for(unsigned j=0;j<n;j++) {
        d4_vertex_t current=in[j];
        bool inside=far?(current.z<=plane):(current.z>=plane);
        if(inside!=was_inside) out[used++]=d4_depth_intersection(previous,current,plane);
        if(inside)out[used++]=current;
        previous=current;was_inside=inside;
    }
    return used;
}

static bool d4_point_inside(d4_point_t p, unsigned edge)
{
    switch(edge) {
    case 0: return p.x>=d4.view_xmin;
    case 1: return p.x<=d4.view_xmax;
    case 2: return p.y>=d4.view_ymin;
    default:return p.y<=d4.view_ymax;
    }
}

static d4_point_t d4_point_intersection(d4_point_t a,d4_point_t b,unsigned edge)
{
    d4_point_t p=a;
    if(edge<2u) {
        int32_t x=(edge==0)?d4.view_xmin:d4.view_xmax;
        p.x=x;
        if(b.x!=a.x)
            p.y=a.y+(int32_t)((int64_t)(b.y-a.y)*(x-a.x)/(b.x-a.x));
    } else {
        int32_t y=(edge==2u)?d4.view_ymin:d4.view_ymax;
        p.y=y;
        if(b.y!=a.y)
            p.x=a.x+(int32_t)((int64_t)(b.x-a.x)*(y-a.y)/(b.y-a.y));
    }
    return p;
}

static uint8_t d4_polygon_edge_clip(const d4_point_t *in,uint8_t n,
                                     d4_point_t *out,unsigned edge)
{
    if(!n)return 0;
    uint8_t used=0;
    d4_point_t prev=in[n-1];
    bool was_inside=d4_point_inside(prev,edge);
    for(uint8_t j=0;j<n;j++) {
        d4_point_t cur=in[j];
        bool inside=d4_point_inside(cur,edge);
        if(inside!=was_inside) {
            if(used>=D4_POLY_CAP) return 0;
            out[used++]=d4_point_intersection(prev,cur,edge);
        }
        if(inside) {
            if(used>=D4_POLY_CAP) return 0;
            out[used++]=cur;
        }
        prev=cur;was_inside=inside;
    }
    return used;
}

static void d4_prepare_face_polygon(const d4_face_t *face)
{
    const uint8_t a=(uint8_t)(face->ring*D4_SIDES+face->side);
    const uint8_t b=(uint8_t)((face->ring+1u)*D4_SIDES+face->side);
    const uint8_t c=(uint8_t)((face->ring+1u)*D4_SIDES+(face->side+1u)%D4_SIDES);
    const uint8_t e=(uint8_t)(face->ring*D4_SIDES+(face->side+1u)%D4_SIDES);
    d4_vertex_t quad[4]={d4.verts[a],d4.verts[b],d4.verts[c],d4.verts[e]};
    d4_vertex_t clipped[6],far_clipped[8];
    uint8_t count=d4_polygon_depth_clip(quad,4u,clipped,D4_NEAR_Z,false);
    if(count!=4u) ++d4.near_clips;
    d4.face_polygon_count=0u;
    d4.emit_part=1u; /* first triangle is (0,1,2) */
    if (count<3u) return;
    uint8_t old_count=count;
    count=d4_polygon_depth_clip(clipped,count,far_clipped,d4.far_z,true);
    if(count!=old_count) ++d4.far_clips;
    if (count<3u) return;
    d4_point_t poly_a[D4_POLY_CAP],poly_b[D4_POLY_CAP];
    for (uint8_t i=0;i<count;i++) poly_a[i]=d4_project(far_clipped[i]);
    bool outside=false;
    for (uint8_t i=0;i<count;i++) if(d4_outcode(poly_a[i])) outside=true;
    if(outside) ++d4.screen_clips;
    for (unsigned edge=0;edge<4u;edge++) {
        count=d4_polygon_edge_clip(poly_a,count,poly_b,edge);
        if (count<3u) return;
        memcpy(poly_a,poly_b,count*sizeof(poly_a[0]));
    }
    memcpy(d4.face_polygon,poly_a,count*sizeof(poly_a[0]));
    d4.face_polygon_count=count;
}

static void d4_emit_face_triangle(void)
{
    if (d4.emit_part+1u>=d4.face_polygon_count) return;
    if(!d4_emit_ok()) return;
    const d4_point_t *a=&d4.face_polygon[0];
    const d4_point_t *b=&d4.face_polygon[d4.emit_part];
    const d4_point_t *c=&d4.face_polygon[d4.emit_part+1u];
    const int64_t area=(int64_t)(b->x-a->x)*(c->y-a->y)
                      -(int64_t)(b->y-a->y)*(c->x-a->x);
    if(area) {
        twodfx_2d_fill_triangle(D4_SCOPE_BG,(uint8_t)d4.slot++,
            (uint16_t)a->x,(uint16_t)a->y,
            (uint16_t)b->x,(uint16_t)b->y,
            (uint16_t)c->x,(uint16_t)c->y);
        ++d4.triangles;
    }
    ++d4.emit_part;
}

/* Cache the world centre and cross-section per ring: its shape and location
 * depend ONLY on the absolute world-depth of the ring, never on the recycling
 * index. This makes the geometry continuous across phase boundaries. */
static void d4_prepare_vertex(uint16_t i)
{
    const uint8_t ring=(uint8_t)(i/D4_SIDES);
    const uint8_t side=(uint8_t)(i%D4_SIDES);
    if(side==0u) {
        const int32_t world_ring_index=(int32_t)d4.cycle-1+(int32_t)ring;
        d4.ring_world_z=D4_FIRST_Z+world_ring_index*(int32_t)d4.ring_spacing;
        d4_world_centre(d4.ring_world_z,&d4.ring_path_x,&d4.ring_path_y);
        const int32_t a=d4_world_sin(d4.ring_world_z,10500,23000u);
        d4.ring_scale=1290+(a*320)/4096; /* ~0.95x..1.57x wide chamber */
        d4.ring_twist_s=d4_world_sin(d4.ring_world_z,14500,11000u)/11;
        d4.ring_twist_c=4096-(d4.ring_twist_s*d4.ring_twist_s)/8192;
    }
    const int32_t rx=(int32_t)ring_x[side]*d4.ring_scale/1024;
    const int32_t ry=(int32_t)ring_y[side]*d4.ring_scale/1024;
    const int32_t wx=d4.ring_path_x+(rx*d4.ring_twist_c-ry*d4.ring_twist_s)/4096-d4.cam_x;
    const int32_t wy=d4.ring_path_y+(rx*d4.ring_twist_s+ry*d4.ring_twist_c)/4096-d4.cam_y;
    const int32_t wz=d4.ring_world_z-d4.travel_z;
    const int32_t x=(wx*d4.yaw_c-wz*d4.yaw_s)/4096;
    int32_t z=(wx*d4.yaw_s+wz*d4.yaw_c)/4096;
    const int32_t y=(wy*d4.pitch_c-z*d4.pitch_s)/4096;
    z=(wy*d4.pitch_s+z*d4.pitch_c)/4096;
    d4_vertex_t *v=&d4.verts[i];
    v->x=x;v->y=y;v->z=z;
}

/* Continuous far cut through the tunnel wall. Each rail crosses the plane
 * within the loaded ring range; connect the eight intersections to create a
 * stable dark octagonal opening rather than a false convergence at (0,0). */
static bool d4_far_point(uint8_t side,d4_point_t *out)
{
    for(uint8_t ring=0;ring+1u<d4.rings;ring++) {
        const d4_vertex_t a=d4.verts[ring*D4_SIDES+side];
        const d4_vertex_t b=d4.verts[(ring+1u)*D4_SIDES+side];
        /* With large off-axis yaw the second ring can be *closer* in
         * camera space than the first.  Intersect either crossing direction;
         * an endpoint already on the far plane needs no division. */
        if (a.z==d4.far_z) { *out=d4_project(a); return true; }
        if (b.z==d4.far_z) { *out=d4_project(b); return true; }
        if ((a.z<d4.far_z && b.z>d4.far_z) ||
            (a.z>d4.far_z && b.z<d4.far_z)) {
            *out=d4_project(d4_depth_intersection(a,b,d4.far_z));
            return true;
        }
    }
    return false;
}

static void d4_emit_far_rim(uint8_t side)
{
    d4_point_t a,b;
    if(!d4_far_point(side,&a) || !d4_far_point((uint8_t)((side+1u)%D4_SIDES),&b))return;
    const uint8_t outside=(uint8_t)(d4_outcode(a)|d4_outcode(b));
    if(!d4_screen_clip_line(&a,&b))return;
    if(outside)++d4.screen_clips;
    if(!d4_emit_ok())return;
    twodfx_2d_line(D4_SCOPE_BG,(uint8_t)d4.slot++,
                   (uint16_t)a.x,(uint16_t)a.y,(uint16_t)b.x,(uint16_t)b.y,1u);
    ++d4.lines;++d4.far_rim_lines;
}

static void d4_build_face(uint16_t f)
{
    const uint8_t ring=(uint8_t)(f/D4_SIDES);
    const uint8_t side=(uint8_t)(f%D4_SIDES);
    const uint8_t a=(uint8_t)(ring*D4_SIDES+side);
    const uint8_t b=(uint8_t)((ring+1u)*D4_SIDES+side);
    const uint8_t c=(uint8_t)((ring+1u)*D4_SIDES+(side+1u)%D4_SIDES);
    const d4_vertex_t *A=&d4.verts[a],*B=&d4.verts[b],*C=&d4.verts[c];
    const int32_t dz4=d4.verts[ring*D4_SIDES+(side+1u)%D4_SIDES].z;
    if((A->z<D4_NEAR_Z && B->z<D4_NEAR_Z && C->z<D4_NEAR_Z && dz4<D4_NEAR_Z) ||
       (A->z>d4.far_z && B->z>d4.far_z && C->z>d4.far_z && dz4>d4.far_z)) return;
    const int32_t ux=B->x-A->x,uy=B->y-A->y,uz=B->z-A->z;
    const int32_t vx=C->x-A->x,vy=C->y-A->y,vz=C->z-A->z;
    const int32_t nx=uy*vz-uz*vy,ny=uz*vx-ux*vz,nz=ux*vy-uy*vx;
    const int64_t facing=-(int64_t)nx*A->x-(int64_t)ny*A->y-(int64_t)nz*A->z;
    if(facing<=0) return;
    d4_face_t *p=&d4.faces[f];
    p->ring=ring;p->side=side;
    p->depth=(A->z+B->z+C->z)/3;
    uint8_t n=d4.visible_count;
    while(n>0 && d4.faces[d4.ordered[n-1]].depth<p->depth) {
        d4.ordered[n]=d4.ordered[n-1];--n;
    }
    d4.ordered[n]=(uint8_t)f;
    ++d4.visible_count;
}

/* === Asp Mk II chase craft (D3's original data, D4's projection/clipper) === */
static int32_t d4_ship_clamp(int32_t x,int32_t lo,int32_t hi)
{
    return x<lo?lo:(x>hi?hi:x);
}

/* Transform model basis: roll around its nose, yaw into the turn, pitch up/
 * down. Original Elite Asp nose points towards positive model Z, which is
 * AWAY from our forward-facing camera; the rear/engine is seen from behind. */
static d4_vertex_t d4_ship_rotate(int32_t x,int32_t y,int32_t z)
{
    const int32_t a=(x*d4.ship_roll_c-y*d4.ship_roll_s)/4096;
    const int32_t b=(x*d4.ship_roll_s+y*d4.ship_roll_c)/4096;
    const int32_t c=(a*d4.ship_yaw_c+z*d4.ship_yaw_s)/4096;
    const int32_t depth=(-a*d4.ship_yaw_s+z*d4.ship_yaw_c)/4096;
    const int32_t up=(b*d4.ship_pitch_c-depth*d4.ship_pitch_s)/4096;
    const int32_t far=(b*d4.ship_pitch_s+depth*d4.ship_pitch_c)/4096;
    return (d4_vertex_t){c,up,far};
}

static void d4_ship_pilot(void)
{
    /* Change actual model Z, not a 2D scale. Smooth independent dolly motion
     * avoids discontinuities even if the 3D scene completes at a lower FPS. */
    int32_t desired_depth=D4_SHIP_DEPTH;
    if(d4.ship_zoom_enabled) {
        desired_depth=D4_SHIP_DEPTH
            +98*d4_world_sin((int32_t)d4.tick,307,9000u)/4096
            +60*d4_gust(d4.tick,169u,0x813a7c25u)/4096;
        desired_depth=d4_ship_clamp(desired_depth,D4_SHIP_NEAR_DEPTH,D4_SHIP_FAR_DEPTH);
    }
    int32_t dz=d4_ship_clamp(desired_depth-d4.ship_depth,
                             -D4_SHIP_Z_STEP_MAX,D4_SHIP_Z_STEP_MAX);
    d4.ship_depth+=dz;

    /* The ship looks down the locally curved tunnel. These are the world
     * centreline direction components, relative to the WILD camera gaze.
     * The model's +Z axis is its nose, and rear exhaust lives at model -Z. */
    int32_t cx,cy,fx,fy;
    d4_world_centre(d4.travel_z+d4.ship_depth,&cx,&cy);
    d4_world_centre(d4.travel_z+d4.ship_depth+280,&fx,&fy);
    d4.ship_world_centre_x=cx; d4.ship_world_centre_y=cy;
    const int32_t tangent_x=((fx-cx)*4096)/280;
    const int32_t tangent_y=((fy-cy)*4096)/280;
    const int32_t wanted_yaw=d4_ship_clamp(tangent_x-d4.yaw_s,-1950,1950);
    const int32_t wanted_pitch=d4_ship_clamp(d4.pitch_s-tangent_y,-1350,1350);

    /* Project a near tunnel-centre target into this SAME scene's camera.
     * Follow it with Q8 low-pass pursuit instead of the old bang-bang 7-px
     * correction. Velocity and bank are filtered independently to suppress
     * screen-pixel snapping / abrupt bank reversals. */
    const int32_t wx=cx-d4.cam_x;
    const int32_t wy=cy-d4.cam_y;
    const int32_t wz=d4.ship_depth;
    const int32_t vx=(wx*d4.yaw_c-wz*d4.yaw_s)/4096;
    const int32_t vz=(wx*d4.yaw_s+wz*d4.yaw_c)/4096;
    const int32_t vy=(wy*d4.pitch_c-vz*d4.pitch_s)/4096;
    const int32_t depth=(wy*d4.pitch_s+vz*d4.pitch_c)/4096;
    int32_t target_x=d4.centre_x,target_y=d4.centre_y;
    if(depth>D4_NEAR_Z) {
        target_x+=(int32_t)(((int64_t)vx*365)/depth);
        target_y-=(int32_t)(((int64_t)vy*300)/depth);
    }
    target_x+=16*d4_gust(d4.tick,107u,0x43289fc1u)/4096;
    target_y+=9*d4_gust(d4.tick,119u,0x013697abu)/4096;
    target_x=d4_ship_clamp(target_x,d4.centre_x-91,d4.centre_x+91);
    target_y=d4_ship_clamp(target_y,d4.centre_y-36,d4.centre_y+36);
    if(d4.tick==0u) {
        d4.ship_x_q8=target_x*256;d4.ship_y_q8=target_y*256;
        d4.ship_vx_q8=0;d4.ship_vy_q8=0;
        d4.ship_yaw_s=wanted_yaw;d4.ship_pitch_s=wanted_pitch;
    }
    const int32_t step_x=d4_ship_clamp((target_x*256-d4.ship_x_q8)/10,
                                       -D4_SHIP_MOVE_Q8_MAX,D4_SHIP_MOVE_Q8_MAX);
    const int32_t step_y=d4_ship_clamp((target_y*256-d4.ship_y_q8)/11,
                                       -D4_SHIP_MOVE_Q8_MAX,D4_SHIP_MOVE_Q8_MAX);
    d4.ship_x_q8+=step_x;d4.ship_y_q8+=step_y;
    d4.ship_x=(d4.ship_x_q8+128)/256;
    d4.ship_y=(d4.ship_y_q8+128)/256;
    d4.ship_vx_q8+=(step_x-d4.ship_vx_q8)/5;
    d4.ship_vy_q8+=(step_y-d4.ship_vy_q8)/5;

    /* Steer the 3D NOSE towards the tunnel path, with gentle yaw/pitch
     * tracking. Bank into the *filtered* lateral speed, not the one-frame
     * displacement, which caused the v1.0.6d jitter. */
    const int32_t wanted_roll=d4_ship_clamp(-d4.ship_vx_q8*165/256,
                                             -1250,1250);
    d4.ship_roll_s+=(wanted_roll-d4.ship_roll_s)/7;
    d4.ship_pitch_s+=(wanted_pitch-d4.ship_pitch_s)/7;
    d4.ship_yaw_s+=(wanted_yaw-d4.ship_yaw_s)/7;
    d4.ship_roll_c=4096-(d4.ship_roll_s*d4.ship_roll_s)/8192;
    d4.ship_pitch_c=4096-(d4.ship_pitch_s*d4.ship_pitch_s)/8192;
    d4.ship_yaw_c=4096-(d4.ship_yaw_s*d4.ship_yaw_s)/8192;
    d4.ship_visible_count=0u;d4.ship_index=0u;
    d4.ship_engine_visible=false;d4.ship_engine_part=0u;
    d4.ship_engine_polygon_count=0u;d4.ship_plume_count=0u;
    d4.ship_prepared=false;d4.ship_stage=0u;
    d4.ship_poly_count=0u;d4.ship_tri_count=0u;
    d4.ship_tri_index=0u;d4.ship_edge=0u;
}

static void d4_ship_vertex(uint8_t i)
{
    const d3_vec3_t v=d3_asp_vertices[i];
    d4_vertex_t p=d4_ship_rotate(v.x,v.y,v.z);
    /* Position a proper 3D model in camera space. This fixed near-depth
     * anchor lets the wild camera look elsewhere without losing the craft. */
    p.x+=(int32_t)(((int64_t)(d4.ship_x-d4.centre_x)*d4.ship_depth)/365);
    p.y-=(int32_t)(((int64_t)(d4.ship_y-d4.centre_y)*d4.ship_depth)/300);
    p.z+=d4.ship_depth;
    d4.ship_cam[i]=p;
    d4.ship_screen[i]=d4_project(p);
}

static void d4_ship_face(uint8_t f)
{
    const d3_face_t *face=&d3_asp_faces[f];
    const d3_vec3_t n=d3_asp_normals[f];
    const d4_vertex_t norm=d4_ship_rotate(n.x,n.y,n.z);
    int32_t x=0,y=0,z=0;
    for(uint8_t i=0;i<face->count;i++) {
        const d4_vertex_t v=d4.ship_cam[face->vertex[i]];
        x+=v.x;y+=v.y;z+=v.z;
    }
    x/=face->count;y/=face->count;z/=face->count;
    const int64_t facing=-(int64_t)norm.x*x-(int64_t)norm.y*y-(int64_t)norm.z*z;
    if(facing<=0) return;
    if(f==11u) d4.ship_engine_visible=true; /* Genuine Asp rear hatch. */
    /* Far to near: the last visible hull polygons cover earlier polygons. */
    uint8_t i=d4.ship_visible_count;
    while(i>0 && d4.ship_face_depth[d4.ship_order[i-1u]]<z) {
        d4.ship_order[i]=d4.ship_order[i-1u];--i;
    }
    d4.ship_face_depth[f]=z;d4.ship_order[i]=f;
    ++d4.ship_visible_count;
}

static int64_t d4_ship_cross(d4_point_t a,d4_point_t b,d4_point_t c)
{
    return (int64_t)(b.x-a.x)*(c.y-a.y)-(int64_t)(b.y-a.y)*(c.x-a.x);
}

static bool d4_ship_inside_triangle(d4_point_t p,d4_point_t a,
                                    d4_point_t b,d4_point_t c,int32_t sign)
{
    return d4_ship_cross(a,b,p)*sign>=0 &&
           d4_ship_cross(b,c,p)*sign>=0 &&
           d4_ship_cross(c,a,p)*sign>=0;
}

static void d4_ship_prepare_polygon(uint8_t face_id)
{
    const d3_face_t *face=&d3_asp_faces[face_id];
    d4_point_t a[D4_POLY_CAP],b[D4_POLY_CAP];
    uint8_t count=face->count;
    for(uint8_t j=0;j<count;j++) a[j]=d4.ship_screen[face->vertex[j]];
    for(uint8_t edge=0;edge<4u;edge++) {
        count=d4_polygon_edge_clip(a,count,b,edge);
        if(count<3u) break;
        memcpy(a,b,count*sizeof(a[0]));
    }
    d4.ship_poly_count=count>=3u?count:0u;
    d4.ship_tri_count=0u;d4.ship_tri_index=0u;
    if (d4.ship_poly_count) {
        memcpy(d4.ship_poly,a,count*sizeof(a[0]));
        /* Same ear-clipping principle as D3: avoid filling concave Elite
         * hull polygons by an erroneous triangle fan. Never write beyond
         * the 10-triangle bounded scratch list, even after screen clipping. */
        uint8_t indices[D4_POLY_CAP];
        int64_t area=0;
        for(uint8_t i=0;i<count;i++) {
            indices[i]=i;
            const d4_point_t u=a[i],v=a[(i+1u)%count];
            area+=(int64_t)u.x*v.y-(int64_t)u.y*v.x;
        }
        if(area!=0) {
            const int32_t sign=area>0?1:-1;
            uint8_t n=count;
            while(n>3u) {
                bool found=false;
                for(uint8_t i=0;i<n;i++) {
                    const uint8_t ia=indices[(i+n-1u)%n];
                    const uint8_t ib=indices[i];
                    const uint8_t ic=indices[(i+1u)%n];
                    if(d4_ship_cross(a[ia],a[ib],a[ic])*sign<=0)continue;
                    bool blocked=false;
                    for(uint8_t j=0;j<n;j++) {
                        const uint8_t it=indices[j];
                        if(it==ia || it==ib || it==ic) continue;
                        if(d4_ship_inside_triangle(a[it],a[ia],a[ib],a[ic],sign)) {
                            blocked=true;break;
                        }
                    }
                    if(blocked)continue;
                    d4_point_t (*t)[3]=&d4.ship_tris[d4.ship_tri_count++];
                    (*t)[0]=a[ia];(*t)[1]=a[ib];(*t)[2]=a[ic];
                    for(uint8_t j=i;j+1u<n;j++) indices[j]=indices[j+1u];
                    --n;found=true;break;
                }
                if(!found)break;
            }
            if(n==3u && d4_ship_cross(a[indices[0]],a[indices[1]],a[indices[2]])!=0) {
                d4_point_t (*t)[3]=&d4.ship_tris[d4.ship_tri_count++];
                for(uint8_t k=0;k<3u;k++) (*t)[k]=a[indices[k]];
            }
        }
    }
    d4.ship_edge=0u;d4.ship_stage=0u;
    d4.ship_prepared=true;
}

/* One API command maximum per worker step. No long continuous draw loop on
 * Core0, so the preemptive scanout and F8/F9 capture keep their priority. */
static void d4_ship_emit_step(void)
{
    if(d4.ship_index>=d4.ship_visible_count) {
        d4.state=d4.ship_engine_visible?D4_SHIP_PLUME_COLOUR:D4_FG_HUD_COLOUR;
        return;
    }
    if(!d4.ship_prepared) {
        d4_ship_prepare_polygon(d4.ship_order[d4.ship_index]);return;
    }
    const uint8_t face_id=d4.ship_order[d4.ship_index];
    if(!d4.ship_poly_count) {
        ++d4.ship_index;d4.ship_prepared=false;return;
    }
    if(d4.ship_stage==0u) {
        if(!d4_emit_ok())return;
        const uint8_t colour=d4.tvc?d3_asp_tvc[face_id]:d3_asp_ep[face_id];
        twodfx_2d_set_color(D4_SCOPE_FG,(uint8_t)d4.slot++,colour,0u);
        d4.ship_stage=1u;return;
    }
    if(d4.ship_stage==1u) {
        if(d4.ship_tri_index<d4.ship_tri_count) {
            if(!d4_emit_ok())return;
            const d4_point_t (*t)[3]=&d4.ship_tris[d4.ship_tri_index++];
            twodfx_2d_fill_triangle(D4_SCOPE_FG,(uint8_t)d4.slot++,
                (uint16_t)(*t)[0].x,(uint16_t)(*t)[0].y,
                (uint16_t)(*t)[1].x,(uint16_t)(*t)[1].y,
                (uint16_t)(*t)[2].x,(uint16_t)(*t)[2].y);
            ++g_d4_ship_triangles; ++d4.triangles;
            return;
        }
        d4.ship_stage=2u;
    }
    if(d4.ship_stage==2u) {
        if(!d4_emit_ok())return;
        twodfx_2d_set_color(D4_SCOPE_FG,(uint8_t)d4.slot++,d4.tvc?15u:4u,0u);
        d4.ship_stage=3u;return;
    }
    if(d4.ship_stage==3u && d4.ship_edge<d4.ship_poly_count) {
        if(!d4_emit_ok())return;
        const d4_point_t a=d4.ship_poly[d4.ship_edge];
        const d4_point_t b=d4.ship_poly[(d4.ship_edge+1u)%d4.ship_poly_count];
        ++d4.ship_edge;
        if(a.x!=b.x || a.y!=b.y) {
            twodfx_2d_line(D4_SCOPE_FG,(uint8_t)d4.slot++,
                (uint16_t)a.x,(uint16_t)a.y,(uint16_t)b.x,(uint16_t)b.y,1u);
            ++g_d4_ship_lines; ++d4.lines;
        }
        return;
    }
    ++d4.ship_index;d4.ship_prepared=false;
}

/* The Asp's original rear opening is face 11. Its little diamond 13-15-14-16
 * is separate interior model detail, not among the D3 hull faces. It was
 * invisible in v1.0.6d, because the rear panel itself is painted black.
 * Fill a real rear exhaust port, brighten its rim, and show a modest plume
 * pointing toward negative model Z (i.e. behind the +Z-facing nose). */
static void d4_ship_prepare_engine(void)
{
    static const uint8_t aperture[4]={13u,15u,14u,16u};
    d4_point_t a[D4_POLY_CAP],b[D4_POLY_CAP];
    uint8_t n=4u;
    for(uint8_t i=0;i<4u;i++)a[i]=d4.ship_screen[aperture[i]];
    for(uint8_t edge=0;edge<4u;edge++) {
        n=d4_polygon_edge_clip(a,n,b,edge);
        if(n<3u)break;
        memcpy(a,b,n*sizeof(a[0]));
    }
    d4.ship_engine_polygon_count=n>=3u?n:0u;
    if(d4.ship_engine_polygon_count)
        memcpy(d4.ship_engine_polygon,a,n*sizeof(a[0]));

    /* Build the plume as a small 3D triangle, not an always-on screen sprite.
     * The aft tip sits behind the actual engine aperture in MODEL space.
     * Smooth deterministic flicker affects the tail's length and direction. */
    const d4_vertex_t aft=d4_ship_rotate(
        9*d4_world_sin((int32_t)d4.tick,79,2500u)/4096,
        5*d4_world_sin((int32_t)d4.tick,101,13000u)/4096,
        -120-18*d4_world_sin((int32_t)d4.tick,67,6000u)/4096);
    d4_vertex_t tip=aft;
    tip.x+=(int32_t)(((int64_t)(d4.ship_x-d4.centre_x)*d4.ship_depth)/365);
    tip.y-=(int32_t)(((int64_t)(d4.ship_y-d4.centre_y)*d4.ship_depth)/300);
    tip.z+=d4.ship_depth;
    d4.ship_plume_count=0u;
    if(tip.z>=D4_NEAR_Z) {
        d4_point_t plume[D4_POLY_CAP] = {
            d4.ship_screen[13],d4.ship_screen[14],d4_project(tip)};
        uint8_t pn=3u;
        for(uint8_t edge=0;edge<4u;edge++) {
            pn=d4_polygon_edge_clip(plume,pn,b,edge);
            if(pn<3u)break;
            memcpy(plume,b,pn*sizeof(plume[0]));
        }
        d4.ship_plume_count=pn>=3u?pn:0u;
        if(d4.ship_plume_count)
            memcpy(d4.ship_plume_polygon,plume,pn*sizeof(plume[0]));
    }
    d4.ship_engine_part=0u;d4.ship_plume_part=1u;
}

/* One graphics instruction maximum per cooperative Core0 worker step. */
static void d4_ship_engine_emit_step(void)
{
    const uint8_t n=d4.ship_engine_polygon_count;
    const d4_point_t *a=d4.ship_engine_polygon;
    switch(d4.state) {
    case D4_SHIP_PLUME_COLOUR:
        d4_ship_prepare_engine();
        if(!d4_emit_ok())return;
        twodfx_2d_set_color(D4_SCOPE_FG,(uint8_t)d4.slot++,
                            ((d4.tick/6u)&1u)?(d4.tvc?11u:6u):(d4.tvc?9u:5u),0u);
        d4.state=D4_SHIP_PLUME;break;
    case D4_SHIP_PLUME:
        if(d4.ship_plume_count>=3u &&
           d4.ship_plume_part+1u<d4.ship_plume_count) {
            if(!d4_emit_ok())return;
            const d4_point_t *p=d4.ship_plume_polygon;
            const d4_point_t x=p[0],y=p[d4.ship_plume_part],z=p[d4.ship_plume_part+1u];
            ++d4.ship_plume_part;
            if(d4_ship_cross(x,y,z)) {
                twodfx_2d_fill_triangle(D4_SCOPE_FG,(uint8_t)d4.slot++,
                    (uint16_t)x.x,(uint16_t)x.y,(uint16_t)y.x,(uint16_t)y.y,
                    (uint16_t)z.x,(uint16_t)z.y);
                ++d4.triangles;
                ++g_d4_ship_plume_triangles;
            }
        } else d4.state=D4_SHIP_ENGINE_FILL_COLOUR;
        break;
    case D4_SHIP_ENGINE_FILL_COLOUR:
        if(!d4_emit_ok())return;
        twodfx_2d_set_color(D4_SCOPE_FG,(uint8_t)d4.slot++,d4.tvc?9u:5u,0u);
        d4.ship_engine_part=1u;
        d4.state=D4_SHIP_ENGINE_FILL;break;
    case D4_SHIP_ENGINE_FILL:
        if(n>=3u && d4.ship_engine_part+1u<n) {
            if(!d4_emit_ok())return;
            const d4_point_t p=a[0],q=a[d4.ship_engine_part],r=a[d4.ship_engine_part+1u];
            ++d4.ship_engine_part;
            if(d4_ship_cross(p,q,r)!=0) {
                twodfx_2d_fill_triangle(D4_SCOPE_FG,(uint8_t)d4.slot++,
                    (uint16_t)p.x,(uint16_t)p.y,(uint16_t)q.x,(uint16_t)q.y,
                    (uint16_t)r.x,(uint16_t)r.y);
                ++d4.triangles;
                ++g_d4_ship_engine_triangles;
            }
        } else {
            d4.state=D4_SHIP_ENGINE_RIM_COLOUR;d4.ship_engine_part=0u;
        }
        break;
    case D4_SHIP_ENGINE_RIM_COLOUR:
        if(!d4_emit_ok())return;
        twodfx_2d_set_color(D4_SCOPE_FG,(uint8_t)d4.slot++,d4.tvc?13u:7u,0u);
        d4.state=D4_SHIP_ENGINE_RIM;break;
    case D4_SHIP_ENGINE_RIM:
        if(n>=3u && d4.ship_engine_part<n) {
            if(!d4_emit_ok())return;
            const d4_point_t p=a[d4.ship_engine_part];
            const d4_point_t q=a[(d4.ship_engine_part+1u)%n];
            ++d4.ship_engine_part;
            if(p.x!=q.x || p.y!=q.y) {
                twodfx_2d_line(D4_SCOPE_FG,(uint8_t)d4.slot++,
                    (uint16_t)p.x,(uint16_t)p.y,(uint16_t)q.x,(uint16_t)q.y,1u);
                ++d4.lines;
            }
        } else d4.state=D4_SHIP_ENGINE_CORE_COLOUR;
        break;
    case D4_SHIP_ENGINE_CORE_COLOUR:
        if(!d4_emit_ok())return;
        twodfx_2d_set_color(D4_SCOPE_FG,(uint8_t)d4.slot++,
                            ((d4.tick/6u)&1u)?(d4.tvc?15u:4u):(d4.tvc?11u:6u),0u);
        d4.state=D4_SHIP_ENGINE_CORE;break;
    case D4_SHIP_ENGINE_CORE: {
        d4_point_t p=d4.ship_screen[13],q=d4.ship_screen[14];
        if(d4_screen_clip_line(&p,&q) && d4_emit_ok()) {
            twodfx_2d_line(D4_SCOPE_FG,(uint8_t)d4.slot++,
                (uint16_t)p.x,(uint16_t)p.y,(uint16_t)q.x,(uint16_t)q.y,1u);
            ++d4.lines;
        }
        d4.state=D4_FG_HUD_COLOUR;break;
    }
    default: break;
    }
}

static void d4_schedule(void)
{
    d4.rings=(d4.mode%3u==0u)?4u:(d4.mode%3u==1u)?8u:12u;
    /* Match far visibility across ring counts while keeping 4/8/12 rings. */
    d4.ring_spacing=(d4.rings==4u)?900u:(d4.rings==8u)?450u:300u;
    d4.far_z=(d4.rings==4u)?D4_FAR_4:(d4.rings==8u)?D4_FAR_8:D4_FAR_12;
    /* Incremental travel permits switching flight profiles with NO teleport. */
    d4.travel_z=d4.travel_distance;
    d4.ring_phase=(uint16_t)((uint32_t)d4.travel_distance%d4.ring_spacing);
    d4.cycle=(uint32_t)d4.travel_distance/d4.ring_spacing;
    /* With the camera turned obliquely, the outermost ring is closer in
     * camera-Z than its world-Z. Pull the far plane back to keep it within
     * the loaded geometry, instead of revealing a false terminal ring. */
    const int32_t reduce=(d4.rings==12u)?700:(d4.rings==8u)?420:180;
    d4.far_z-=(int32_t)((uint32_t)reduce*d4.camera_strength_q12/4096u);
    d4.view_xmin=(int32_t)TWODFX_BASIC_SAFE_MIN_X;
    d4.view_xmax=(int32_t)TWODFX_BASIC_SAFE_MAX_X;
    d4.view_ymin=(int32_t)TWODFX_BASIC_SAFE_MIN_Y;
    d4.view_ymax=(int32_t)TWODFX_BASIC_SAFE_MAX_Y;
    d4.index=0u; d4.op=0u; d4.slot=0u;
    g_d4_ship_triangles=g_d4_ship_lines=0u;
    g_d4_ship_engine_triangles=g_d4_ship_plume_triangles=0u;
    d4.emit_part=0u;d4.face_prepared=false;d4.face_polygon_count=0u;d4.current_colour=0xFFu;
    d4.visible_count=0u;d4.pip=0u;d4.cam_pip=0u;d4.bar=0u;
    d4.cpu_us=0u;d4.steps=0u;d4.max_step_us=0u;
    d4.fault=false;
    d4.near_clips=d4.far_clips=d4.screen_clips=d4.lines=d4.triangles=d4.far_rim_lines=0u;
    d4.state=D4_PREP;
    d4.prep_start=time_us_32();
    d4_worker_active=true;
}

void egg_D4_3d_scope_stop(void)
{
    if (!d4_worker_active) return;
    d4_worker_active=false;
    d4.state=D4_READY;
}

void egg_D4_3d_scope_start(void)
{
    memset(&d4,0,sizeof(d4));
    d4.tvc=twodfx_get_machine()==TWODFX_MACHINE_TVC;
    /* Default to a continuous full-detail wireframe flight.  Automatic
     * switching remains available with OUT 249,255 for scope sweeps. */
    d4.mode=2u;d4.forced_mode=true;d4.ship_enabled=true;
    d4.ship_zoom_enabled=true;d4.ship_depth=D4_SHIP_DEPTH;
    /* The aggressive camera is default in v1.0.6c; F9 240 restores the
     * original v1.0.6b smooth-centreline behaviour without restarting D4. */
    d4.camera_profile=2u;d4.camera_strength_q12=0u;
    twodfx_set_engine_mode(TWODFX_ENGINE_FORCED_ON);
    twodfx_set_fps(TWODFX_FPS_50);
    twodfx_reset_sprites();
    g_d4_prep_wall_us=g_d4_prep_cpu_us=g_d4_prep_steps=0u;
    g_d4_prep_max_step_us=g_d4_completed_scenes=g_d4_faults=0u;
    g_d4_visible_faces=g_d4_frame_phase=0u;
    g_d4_mode=d4.mode;
    g_d4_camera_profile=d4.camera_profile;
    g_d4_camera_strength=d4.camera_strength_q12;
    g_d4_near_clips=g_d4_screen_clips=0u;
    g_d4_emitted_lines=g_d4_emitted_triangles=0u;
    d4_schedule();
}

void egg_D4_3d_scope_tick(void)
{
    /* Invoked only when a complete scene has just been published. */
    uint8_t v;
    while (twodfx_egg_read_f9(&v)) {
        if (v<=5u) {d4.mode=v;d4.forced_mode=true;d4.phase_tick=0;}
        else if (v==255u) {d4.forced_mode=false;d4.phase_tick=0;}
        else if (v>=240u && v<=242u) {d4.camera_profile=(uint8_t)(v-240u);}
        else if (v==243u) {d4.ship_enabled=true;}
        else if (v==244u) {d4.ship_enabled=false;}
        else if (v==245u) {d4.ship_zoom_enabled=false;}
        else if (v==246u) {d4.ship_zoom_enabled=true;}
    }
    ++d4.tick;
    const uint16_t target=(d4.camera_profile==0u)?0u:
                          (d4.camera_profile==1u)?2304u:4096u;
    if (d4.camera_strength_q12<target) {
        uint16_t step=(uint16_t)(target-d4.camera_strength_q12);
        d4.camera_strength_q12+=(step>D4_CAMERA_BLEND_STEP)?D4_CAMERA_BLEND_STEP:step;
    } else if (d4.camera_strength_q12>target) {
        uint16_t step=(uint16_t)(d4.camera_strength_q12-target);
        d4.camera_strength_q12-=(step>D4_CAMERA_BLEND_STEP)?D4_CAMERA_BLEND_STEP:step;
    }
    d4.travel_distance+=(int32_t)(D4_CRUISE_SPEED+
        D4_WILD_SPEED_EXTRA*d4.camera_strength_q12/4096u);
    if (!d4.forced_mode && ++d4.phase_tick>=D4_MODE_TICKS) {
        d4.phase_tick=0;
        d4.mode=(uint8_t)((d4.mode+1u)%6u);
    }
    g_d4_frame_phase=d4.phase_tick;
    g_d4_mode=d4.mode;
    d4_schedule();
}

bool egg_D4_3d_scope_ready(void) {return d4.state==D4_READY;}

static void d4_worker_operation(void)
{
    switch(d4.state) {
    case D4_PREP: {
        /* Follow the SAME slowly curved world-space tunnel as the ring
         * geometry.  No integer 8-bit wrapped-angle / 2 discontinuity. */
        int32_t look_x,look_y;
        int32_t path_x,path_y;
        d4_world_centre(d4.travel_z,&path_x,&path_y);
        d4_world_centre(d4.travel_z+D4_LOOKAHEAD,&look_x,&look_y);
        /* In the original flight the camera follows the tunnel and looks
         * ahead.  The aggressive profile adds independent, smoothly changing
         * lateral movement and an intentionally off-axis gaze. This can
         * place the far opening outside the nearer projected rings or even
         * off-screen, exercising near/far/screen clipping for real. */
        const int32_t lateral_x=125*d4_gust(d4.tick,49u,0x1b8723adu)/4096
             +35*d4_world_sin((int32_t)d4.tick,131,6000u)/4096;
        const int32_t lateral_y=55*d4_gust(d4.tick,71u,0x9685de37u)/4096
             +18*d4_world_sin((int32_t)d4.tick,103,21000u)/4096;
        const int32_t steering_yaw=2650*d4_gust(d4.tick,67u,0x763ca74du)/4096
             +950*d4_world_sin((int32_t)d4.tick,121,30000u)/4096;
        const int32_t steering_pitch=1750*d4_gust(d4.tick,83u,0x502ca9b1u)/4096
             +650*d4_world_sin((int32_t)d4.tick,147,24000u)/4096;
        const int32_t strength=d4.camera_strength_q12;
        d4.cam_x=path_x+(lateral_x*strength)/4096;
        d4.cam_y=path_y+(lateral_y*strength)/4096;
        d4.yaw_s=d4_clamp_rotation((look_x-path_x)*4096/D4_LOOKAHEAD
                         +(steering_yaw*strength)/4096,3000);
        d4.pitch_s=d4_clamp_rotation((look_y-path_y)*4096/D4_LOOKAHEAD
                         +(steering_pitch*strength)/4096,2300);
        d4.yaw_c=4096-(d4.yaw_s*d4.yaw_s)/8192;
        d4.pitch_c=4096-(d4.pitch_s*d4.pitch_s)/8192;
        d4.centre_x=(int32_t)TWODFX_BASIC_SAFE_MIN_X+
                    (int32_t)TWODFX_BASIC_SAFE_WIDTH/2;
        d4.centre_y=(int32_t)TWODFX_BASIC_SAFE_MIN_Y+
                    (int32_t)TWODFX_BASIC_SAFE_HEIGHT/2;
        d4.state=D4_VERTEX;
        break;
    }
    case D4_VERTEX:
        d4_prepare_vertex(d4.index++);
        if (d4.index>=(uint16_t)d4.rings*D4_SIDES) {
            d4.index=0u;d4.state=D4_FACE;
        }
        break;
    case D4_FACE:
        d4_build_face(d4.index++);
        if (d4.index>=(uint16_t)(d4.rings-1u)*D4_SIDES)
            d4.state=D4_RESET_BG;
        break;
    case D4_RESET_BG:
        twodfx_reset_2dpt_background();d4.state=D4_RESET_FG;break;
    case D4_RESET_FG:
        twodfx_reset_2dpt_foreground();d4.state=D4_SET_BG_COLOUR;break;
    case D4_SET_BG_COLOUR:
        d4_colour(d4.tvc?15u:4u);d4.state=D4_EMIT_GEOMETRY;break;
    case D4_EMIT_GEOMETRY:
        if (d4.mode<3u) {
            const uint16_t ring_edge_count=(uint16_t)d4.rings*D4_SIDES;
            const uint16_t rail_count=(uint16_t)(d4.rings-1u)*D4_SIDES;
            if (d4.op<ring_edge_count) {
                uint8_t ring=(uint8_t)(d4.op/D4_SIDES);
                uint8_t side=(uint8_t)(d4.op%D4_SIDES);
                d4_line((uint8_t)(ring*D4_SIDES+side),
                        (uint8_t)(ring*D4_SIDES+(side+1u)%D4_SIDES));
            } else if (d4.op<ring_edge_count+rail_count) {
                uint16_t edge=(uint16_t)(d4.op-ring_edge_count);
                d4_line((uint8_t)edge,(uint8_t)(edge+D4_SIDES));
            } else if (d4.op<ring_edge_count+rail_count+D4_SIDES) {
                d4_emit_far_rim((uint8_t)(d4.op-ring_edge_count-rail_count));
            }
            ++d4.op;
            if (d4.op>=ring_edge_count+rail_count+D4_SIDES) d4.state=D4_FG_COLOUR;
        } else {
            if (d4.op>=d4.visible_count) {d4.state=D4_FG_COLOUR;break;}
            const d4_face_t *f=&d4.faces[d4.ordered[d4.op]];
            /* World-space parity stays constant as near rings recycle. */
        const uint8_t colour=(uint8_t)((((uint32_t)f->ring+d4.cycle-1u)&1u)?
                          (d4.tvc?9u:5u):(d4.tvc?13u:7u));
            if (d4.current_colour!=colour) {
                d4_colour(colour);d4.current_colour=colour;break;
            }
            if (!d4.face_prepared) {
                d4_prepare_face_polygon(f);
                d4.face_prepared=true;
                break;
            }
            if (d4.emit_part+1u<d4.face_polygon_count) {
                d4_emit_face_triangle();
            }
            if(d4.emit_part+1u>=d4.face_polygon_count) {
                d4.face_prepared=false;
                ++d4.op;
            }
        }
        break;
    case D4_FG_COLOUR:
        twodfx_2d_ret(D4_SCOPE_BG,(uint8_t)d4.slot);
        d4.slot=0u;
        d4.state=d4.ship_enabled?D4_SHIP_PREP:D4_FG_HUD_COLOUR;
        break;
    case D4_SHIP_PREP:
        d4_ship_pilot(); d4.index=0u;d4.state=D4_SHIP_VERTEX;break;
    case D4_SHIP_VERTEX:
        d4_ship_vertex((uint8_t)d4.index++);
        if(d4.index>=19u) {d4.index=0u;d4.state=D4_SHIP_FACE;}
        break;
    case D4_SHIP_FACE:
        d4_ship_face((uint8_t)d4.index++);
        if(d4.index>=12u) {d4.index=0u;d4.state=D4_SHIP_DRAW;}
        break;
    case D4_SHIP_DRAW:
        d4_ship_emit_step();break;
    case D4_SHIP_PLUME_COLOUR:
    case D4_SHIP_PLUME:
    case D4_SHIP_ENGINE_FILL_COLOUR:
    case D4_SHIP_ENGINE_FILL:
    case D4_SHIP_ENGINE_RIM_COLOUR:
    case D4_SHIP_ENGINE_RIM:
    case D4_SHIP_ENGINE_CORE_COLOUR:
    case D4_SHIP_ENGINE_CORE:
        d4_ship_engine_emit_step();break;
    case D4_FG_HUD_COLOUR:
        twodfx_2d_set_color(D4_SCOPE_FG,(uint8_t)d4.slot++,d4.tvc?15u:4u,0u);
        d4.state=D4_FG_PIPS;break;
    case D4_FG_PIPS: {
        uint16_t x=(uint16_t)(TWODFX_BASIC_SAFE_MIN_X+8u+15u*d4.pip);
        uint16_t y=(uint16_t)(TWODFX_BASIC_SAFE_MIN_Y+6u);
        uint16_t w=d4.pip==d4.mode?13u:6u;
        uint16_t h=d4.pip==d4.mode?7u:3u;
        twodfx_2d_fill_rect(D4_SCOPE_FG,(uint8_t)d4.slot++,x,y,w,h);
        if (++d4.pip>=6u) d4.state=D4_FG_CAMERA_PIPS;
        break;
    }
    case D4_FG_CAMERA_PIPS: {
        /* One/two/three small squares at top right: original / roam / wild.
         * Kept in FOREGROUND and independent of public STATUS bits. */
        const uint16_t x=(uint16_t)(TWODFX_BASIC_SAFE_MIN_X+121u+12u*d4.cam_pip);
        const uint16_t y=(uint16_t)(TWODFX_BASIC_SAFE_MIN_Y+6u);
        twodfx_2d_fill_rect(D4_SCOPE_FG,(uint8_t)d4.slot++,x,y,7u,6u);
        if (++d4.cam_pip>d4.camera_profile) d4.state=D4_FG_BAR_COLOUR;
        break;
    }
    case D4_FG_BAR_COLOUR:
        twodfx_2d_set_color(D4_SCOPE_FG,(uint8_t)d4.slot++,d4.tvc?7u:1u,0u);
        d4.state=D4_FG_BARS;break;
    case D4_FG_BARS: {
        /* PREVIOUS completed preparation; not the scene currently building.
         * Bar 1: wall interval in 100us units; Bar 2: summed worker CPU in 10us.
         * Both cap at 160 pixels. Compare exact figures using SWD globals. */
        uint32_t length=d4.bar==0u?g_d4_prep_wall_us/100u:g_d4_prep_cpu_us/10u;
        if (length>160u) length=160u;
        if (length) twodfx_2d_fill_rect(D4_SCOPE_FG,(uint8_t)d4.slot++,
                 (uint16_t)(TWODFX_BASIC_SAFE_MIN_X+8u),
                 (uint16_t)(TWODFX_BASIC_SAFE_MIN_Y+16u+8u*d4.bar),
                 (uint16_t)length,4u);
        ++d4.bar;
        if(d4.bar>=2u) d4.state=D4_FINISH;
        break;
    }
    case D4_FINISH:
        twodfx_2d_ret(D4_SCOPE_FG,(uint8_t)d4.slot);
        d4.state=D4_READY;
        break;
    case D4_READY: default: break;
    }
}

void egg_D4_3d_scope_step(void)
{
    if (!d4_worker_active || d4.state==D4_READY) return;
    /* A single bounded step; never executed inside scanline interrupt. */
    uint32_t start=time_us_32();
    d4_worker_operation();
    uint32_t spent=(uint32_t)(time_us_32()-start);
    d4.cpu_us+=spent;
    ++d4.steps;
    if (spent>d4.max_step_us) d4.max_step_us=spent;
    if (d4.state==D4_READY) {
        g_d4_prep_wall_us=(uint32_t)(time_us_32()-d4.prep_start);
        g_d4_prep_cpu_us=d4.cpu_us;
        g_d4_prep_steps=d4.steps;
        g_d4_prep_max_step_us=d4.max_step_us;
        g_d4_visible_faces=d4.visible_count;
        g_d4_near_clips=d4.near_clips;
        g_d4_far_clips=d4.far_clips;
        g_d4_far_rim_lines=d4.far_rim_lines;
        g_d4_camera_profile=d4.camera_profile;
        g_d4_camera_strength=d4.camera_strength_q12;
        g_d4_ship_visible_faces=d4.ship_visible_count;
        g_d4_ship_enabled=d4.ship_enabled?1u:0u;
        g_d4_ship_x=d4.ship_x;g_d4_ship_y=d4.ship_y;
        g_d4_ship_depth=d4.ship_depth;
        g_d4_ship_engine_visible=d4.ship_engine_visible?1u:0u;
        g_d4_cam_world_z=d4.travel_z;
        g_d4_cam_x=d4.cam_x;g_d4_cam_y=d4.cam_y;
        g_d4_yaw_s=d4.yaw_s;g_d4_pitch_s=d4.pitch_s;
        g_d4_screen_clips=d4.screen_clips;
        g_d4_emitted_lines=d4.lines;
        g_d4_emitted_triangles=d4.triangles;
        if (d4.fault) ++g_d4_faults;
        ++g_d4_completed_scenes;
    }
}
