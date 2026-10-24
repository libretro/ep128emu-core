#include "egg_D3_elite_showcase.h"
#include "egg_D3_elite_showcase_data.h"
#include "twodfx_api.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/*
 * D3 Elite showcase cooperative scene builder (core0 ONLY).
 *
 * Each worker_step() performs ONE bounded task in main-loop idle time:
 * a vertex projection, face classification, planning one face, or writing
 * one 2DPT command.  Only complete shadow scenes are published at VSYNC.
 * This is egg-only scheduling; PIO, DMA and normal 2DPT API are unchanged.
 *
 * Adding a ship: add immutable geometry/metadata to egg_D3_elite_showcase_data.h,
 * then append its model descriptor.  No new worker or scene builder needed.
 */


static int16_t sample_q12(uint32_t phase_q8)
{
    const uint32_t idx = (phase_q8 >> 8) & 255u;
    const uint32_t frac = phase_q8 & 255u;
    const int32_t first = d3_sine_q12[idx];
    const int32_t delta = (int32_t)d3_sine_q12[(idx+1u)&255u] - first;
    return (int16_t)(first + egg3d_round_div(delta * (int32_t)frac, 256));
}

static egg3d_angles_q12_t egg3d_angles_for_tick(uint16_t tick)
{
    const uint32_t yaw_q8 = (uint32_t)tick * 256u;
    const uint32_t pitch_q8 = 14u * 256u + (uint32_t)tick * 256u / 3u;
    const uint32_t roll_q8 = 11u * 256u + (uint32_t)tick * 256u / 5u;
    return (egg3d_angles_q12_t){
        sample_q12(yaw_q8), sample_q12(yaw_q8 + 64u * 256u),
        sample_q12(pitch_q8), sample_q12(pitch_q8 + 64u * 256u),
        sample_q12(roll_q8), sample_q12(roll_q8 + 64u * 256u)
    };
}

typedef struct { int16_t x,y,z; } d3_rotated_t;
typedef struct { uint16_t x,y; } d3_screen_t;
typedef struct { uint8_t type,a,b,c,colour; } d3_op_t;
enum { D3_OP_COLOR=1, D3_OP_BORDER, D3_OP_LINE, D3_OP_FILL_TRI,
       D3_OP_TRI };
typedef enum {
    D3_IDLE=0, D3_SETUP, D3_VERTICES, D3_FACES, D3_SORT,
    D3_PLAN_INIT, D3_PLAN_FACES, D3_EMIT_RESET_BG,
    D3_EMIT_RESET_FG, D3_EMIT_BG, D3_EMIT_RET, D3_READY
} d3_state_t;

static struct {
    const d3_model_t *model;
    egg3d_angles_q12_t angles;
    d3_rotated_t rotated[42];
    d3_screen_t screen[42];
    bool visible[13];
    bool visible_tri[13][5];
    int16_t depth[13];
    uint8_t sorted[13];
    d3_op_t bg[252];
    uint16_t bg_count, emit_index;
    uint8_t item, model_idx;
    uint16_t phase_tick;
    bool tvc, fault;
    d3_state_t state;
} d3;

static bool d3_add(uint8_t layer, uint8_t type, uint8_t a, uint8_t b,
                   uint8_t c, uint8_t colour)
{
    d3_op_t op = {type,a,b,c,colour};
    if (layer != 0u || d3.bg_count >= 251u) {
        d3.fault = true;
        return false;
    }
    d3.bg[d3.bg_count++] = op;
    return true;
}
#define BG(t,a,b,c,col) ((void)d3_add(0u,t,a,b,c,col))

static uint8_t d3_colour(uint8_t face)
{
    return d3.tvc ? d3.model->tvc_palette[face] :
                    d3.model->enterprise_palette[face];
}

static void d3_project_one(uint8_t i)
{
    const d3_vec3_t v = d3.model->vertices[i];
    egg3d_vec_q8_t r=egg3d_rotate_q8(&d3.angles,v.x,v.y,v.z);
    d3.rotated[i]=(d3_rotated_t){
        (int16_t)egg3d_round_div(r.x,256),
        (int16_t)egg3d_round_div(r.y,256),
        (int16_t)egg3d_round_div(r.z,256)};
    const int32_t cx=TWODFX_BASIC_SAFE_MIN_X+TWODFX_BASIC_SAFE_WIDTH/2;
    const int32_t cy=TWODFX_BASIC_SAFE_MIN_Y+TWODFX_BASIC_SAFE_HEIGHT/2+
                     d3.model->centre_y_offset;
    d3.screen[i].x=egg3d_project_axis(cx,r.x,d3.model->camera*256,r.z,
                                       d3.model->scale_x,1023u,+1);
    d3.screen[i].y=egg3d_project_axis(cy,r.y,d3.model->camera*256,r.z,
                                       d3.model->scale_y,511u,-1);
}

static void d3_classify_one(uint8_t f)
{
    const d3_face_t *p=&d3.model->faces[f];
    int32_t sx=0,sy=0,sz=0;
    for(uint8_t j=0;j<p->count;j++) {
        d3_rotated_t v=d3.rotated[p->vertex[j]];
        sx+=v.x; sy+=v.y; sz+=v.z;
    }
    sx/=p->count; sy/=p->count; sz/=p->count;
    d3.depth[f]=(int16_t)sz;
    d3.sorted[f]=f;
    d3.visible[f]=false;
    memset(d3.visible_tri[f],0,sizeof(d3.visible_tri[f]));
    if(d3.model->kind!=D3_COBRA) {
        const d3_vec3_t norm=d3.model->normals[f];
        egg3d_vec_q8_t r=egg3d_rotate_q8(&d3.angles,norm.x,norm.y,norm.z);
        int32_t nx=egg3d_round_div(r.x,256);
        int32_t ny=egg3d_round_div(r.y,256);
        int32_t nz=egg3d_round_div(r.z,256);
        int32_t facing=nx*(-sx)+ny*(-sy)+nz*(d3.model->camera-sz);
        d3.visible[f]=(facing>0);
        return;
    }
    /* Preserve v1.0.4a per-triangle Cobra face visibility, including the
     * original inward winding and the bent lower wing quadrilaterals. */
    for(uint8_t k=1;k+1u<p->count;k++) {
        d3_rotated_t a=d3.rotated[p->vertex[0]];
        d3_rotated_t b=d3.rotated[p->vertex[k]];
        d3_rotated_t c=d3.rotated[p->vertex[k+1u]];
        int32_t ux=b.x-a.x,uy=b.y-a.y,uz=b.z-a.z;
        int32_t vx=c.x-a.x,vy=c.y-a.y,vz=c.z-a.z;
        int32_t nx=uy*vz-uz*vy,ny=uz*vx-ux*vz,nz=ux*vy-uy*vx;
        int64_t facing=(int64_t)nx*-a.x+(int64_t)ny*-a.y+
                        (int64_t)nz*(380-a.z);
        bool shown=facing<0;
        d3.visible_tri[f][k-1u]=shown;
        if(shown) d3.visible[f]=true;
    }
}

static void d3_sort(void)
{
    for(uint8_t i=0;i<d3.model->face_count;i++)
        for(uint8_t j=(uint8_t)(i+1u);j<d3.model->face_count;j++)
            if(d3.depth[d3.sorted[j]]<d3.depth[d3.sorted[i]]) {
                uint8_t t=d3.sorted[i]; d3.sorted[i]=d3.sorted[j];d3.sorted[j]=t;
            }
}

static void d3_plan_engine_port(const uint8_t *v,uint8_t n)
{
    BG(D3_OP_COLOR,0,0,0,0);
    for(uint8_t k=1;k+1u<n;k++) BG(D3_OP_FILL_TRI,v[0],v[k],v[k+1u],0);
    BG(D3_OP_COLOR,0,0,0,d3.tvc?14u:7u);
    for(uint8_t k=0;k<n;k++) BG(D3_OP_LINE,v[k],v[(k+1u)%n],0,0);
}
static void d3_plan_cobra_details(uint8_t face_id)
{
    if(face_id==9u) {
        static const uint8_t main_l[4]={12,13,18,19};
        static const uint8_t main_r[4]={14,15,16,17};
        static const uint8_t wing_l[3]={22,23,24};
        static const uint8_t wing_r[3]={25,26,27};
        d3_plan_engine_port(main_l,4);d3_plan_engine_port(main_r,4);
        d3_plan_engine_port(wing_l,3);d3_plan_engine_port(wing_r,3);
    } else if(face_id==0u || face_id==11u) {
        BG(D3_OP_COLOR,0,0,0,d3.tvc?15u:4u);
        BG(D3_OP_LINE,20,21,0,0);
    }
}
static void d3_plan_cobra_face(uint8_t f)
{
    const d3_face_t *face=&d3.model->faces[f];
    BG(D3_OP_COLOR,0,0,0,d3_colour(f));
    for(uint8_t k=1;k+1u<face->count;k++)
        if(d3.visible_tri[f][k-1u])
            BG(D3_OP_FILL_TRI,face->vertex[0],face->vertex[k],face->vertex[k+1u],0);
    BG(D3_OP_COLOR,0,0,0,d3.tvc?15u:4u);
    if(face->count==3u) {
        if(d3.visible_tri[f][0])
            BG(D3_OP_TRI,face->vertex[0],face->vertex[1],face->vertex[2],0);
    } else {
        for(uint8_t k=0;k<face->count;k++) {
            uint8_t tri=k==0u?0u:k==face->count-1u?
                        (uint8_t)(face->count-3u):(uint8_t)(k-1u);
            if(d3.visible_tri[f][tri])
                BG(D3_OP_LINE,face->vertex[k],face->vertex[(k+1u)%face->count],0,0);
        }
    }
    d3_plan_cobra_details(f);
}

static int32_t d3_cross(uint8_t a,uint8_t b,uint8_t c)
{
    d3_screen_t A=d3.screen[a],B=d3.screen[b],C=d3.screen[c];
    return ((int32_t)B.x-A.x)*((int32_t)C.y-A.y)-
           ((int32_t)B.y-A.y)*((int32_t)C.x-A.x);
}
static bool d3_point_in_tri(uint8_t p,uint8_t a,uint8_t b,uint8_t c,int32_t sign)
{
    return d3_cross(a,b,p)*sign>=0 && d3_cross(b,c,p)*sign>=0 &&
           d3_cross(c,a,p)*sign>=0;
}
static void d3_plan_poly_fill(const d3_face_t *face)
{
    uint8_t ids[7];
    uint8_t n=face->count;
    for(uint8_t i=0;i<n;i++) ids[i]=face->vertex[i];
    int32_t area=0;
    for(uint8_t i=0;i<n;i++) {
        d3_screen_t A=d3.screen[ids[i]],B=d3.screen[ids[(i+1u)%n]];
        area+=(int32_t)A.x*B.y-(int32_t)A.y*B.x;
    }
    if(area==0) return;
    int32_t sign=area>0?1:-1;
    while(n>3u) {
        bool found=false;
        for(uint8_t i=0;i<n;i++) {
            uint8_t a=ids[(i+n-1u)%n],b=ids[i],c=ids[(i+1u)%n];
            if(d3_cross(a,b,c)*sign<=0) continue;
            bool blocked=false;
            for(uint8_t j=0;j<n;j++) {
                uint8_t test=ids[j];
                if(test==a || test==b || test==c) continue;
                if(d3_point_in_tri(test,a,b,c,sign)) {blocked=true;break;}
            }
            if(blocked) continue;
            BG(D3_OP_FILL_TRI,a,b,c,0);
            for(uint8_t j=i;j+1u<n;j++) ids[j]=ids[j+1u];
            n--;found=true;break;
        }
        if(!found) return;
    }
    if(d3_cross(ids[0],ids[1],ids[2])!=0)
        BG(D3_OP_FILL_TRI,ids[0],ids[1],ids[2],0);
}
static void d3_plan_poly_face(uint8_t f)
{
    const d3_face_t *face=&d3.model->faces[f];
    BG(D3_OP_COLOR,0,0,0,d3_colour(f));
    d3_plan_poly_fill(face);
    BG(D3_OP_COLOR,0,0,0,d3.tvc?15u:4u);
    for(uint8_t e=0;e<d3.model->edge_count;e++) {
        const d3_edge_t edge=d3.model->edges[e];
        uint8_t a=edge.face_a,b=edge.face_b;
        if(a!=f && b!=f) continue;
        if(a!=b && d3.visible[a] && d3.visible[b] &&
           d3.depth[a==f?b:a]>d3.depth[f]) continue;
        BG(D3_OP_LINE,edge.a,edge.b,0,0);
    }
}

static void d3_execute(uint8_t layer,uint8_t slot,const d3_op_t *op)
{
    twodfx_layer_t target=layer?TWODFX_LAYER_FOREGROUND:TWODFX_LAYER_BACKGROUND;
    d3_screen_t a,b,c;
    switch(op->type) {
        case D3_OP_COLOR:
            twodfx_2d_set_color(target,slot,op->colour,0);break;
        case D3_OP_BORDER:
            twodfx_2d_rect(target,slot,TWODFX_BASIC_SAFE_MIN_X,
              TWODFX_BASIC_SAFE_MIN_Y,TWODFX_BASIC_SAFE_WIDTH,
              TWODFX_BASIC_SAFE_HEIGHT,1);break;
        case D3_OP_LINE:
            a=d3.screen[op->a];b=d3.screen[op->b];
            twodfx_2d_line(target,slot,a.x,a.y,b.x,b.y,1);break;
        case D3_OP_FILL_TRI:
        case D3_OP_TRI:
            a=d3.screen[op->a];b=d3.screen[op->b];c=d3.screen[op->c];
            if(op->type==D3_OP_FILL_TRI)
                twodfx_2d_fill_triangle(target,slot,a.x,a.y,b.x,b.y,c.x,c.y);
            else twodfx_2d_triangle(target,slot,a.x,a.y,b.x,b.y,c.x,c.y);
            break;
        default: break;
    }
}

static void d3_schedule(uint8_t model_idx,uint16_t phase_tick)
{
    d3.model_idx=model_idx;
    d3.model=&d3_models[model_idx];
    d3.phase_tick=phase_tick;
    d3.bg_count=0;d3.emit_index=0;d3.item=0;
    d3.fault=false;
    d3.state=D3_SETUP;
}

void egg_D3_elite_start(void)
{
    memset(&d3,0,sizeof(d3));
    d3.tvc=twodfx_get_machine()==TWODFX_MACHINE_TVC;
    twodfx_set_engine_mode(TWODFX_ENGINE_FORCED_ON);
    twodfx_reset_sprites();
    d3_schedule(D3_COBRA,0);
}

/* Invoked only AFTER main.c atomically snapshots the previously completed
 * scene at VSYNC; it schedules work for the NEXT VSYNC. */
void egg_D3_elite_tick(void)
{
    uint16_t next = (uint16_t)(d3.phase_tick + 1u);
    uint8_t ship = d3.model_idx;
    if (next >= D3_PHASE_TICKS) {
        next = 0u;
        ship = (uint8_t)((ship + 1u) % D3_MODEL_COUNT);
    }
    d3_schedule(ship, next);
}

bool egg_D3_elite_ready(void) {return d3.state==D3_READY;}

void egg_D3_elite_step(void)
{
    switch(d3.state) {
    case D3_SETUP:
        d3.angles=egg3d_angles_for_tick(d3.phase_tick);
        d3.item=0u;d3.state=D3_VERTICES;break;
    case D3_VERTICES:
        d3_project_one(d3.item++);
        if(d3.item>=d3.model->vertex_count) {d3.item=0;d3.state=D3_FACES;}
        break;
    case D3_FACES:
        d3_classify_one(d3.item++);
        if(d3.item>=d3.model->face_count) d3.state=D3_SORT;
        break;
    case D3_SORT:
        d3_sort();d3.item=0;d3.state=D3_PLAN_INIT;break;
    case D3_PLAN_INIT:
        BG(D3_OP_COLOR,0,0,0,d3.tvc?13u:7u);
        BG(D3_OP_BORDER,0,0,0,0);
        d3.state=D3_PLAN_FACES;break;
    case D3_PLAN_FACES:
        if(d3.item<d3.model->face_count) {
            uint8_t f=d3.sorted[d3.item++];
            if(d3.visible[f]) {
                if(d3.model->kind==D3_COBRA) d3_plan_cobra_face(f);
                else d3_plan_poly_face(f);
            }
        }
        if(d3.item>=d3.model->face_count) d3.state=D3_EMIT_RESET_BG;
        break;
    case D3_EMIT_RESET_BG:
        if(d3.fault) {d3.state=D3_READY;break;}
        twodfx_reset_2dpt_background();
        d3.state=D3_EMIT_RESET_FG;break;
    case D3_EMIT_RESET_FG:
        twodfx_reset_2dpt_foreground();
        d3.emit_index=0;d3.state=D3_EMIT_BG;break;
    case D3_EMIT_BG:
        if(d3.emit_index<d3.bg_count) {
            d3_execute(0u,(uint8_t)d3.emit_index,&d3.bg[d3.emit_index]);
            d3.emit_index++;
        }
        if(d3.emit_index>=d3.bg_count) d3.state=D3_EMIT_RET;
        break;
    case D3_EMIT_RET:
        twodfx_2d_ret(TWODFX_LAYER_BACKGROUND,(uint8_t)d3.bg_count);
        twodfx_2d_ret(TWODFX_LAYER_FOREGROUND,0u);
        d3.state=D3_READY;break;
    case D3_READY: case D3_IDLE: default: break;
    }
}
