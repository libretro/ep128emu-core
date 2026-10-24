#include "egg_D5_public_mesh.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "2dfx.h"
#include "twodfx_api.h"
#include "twodfx_egg.h"
#include "egg_D3_elite_showcase.h"
#include "egg_D3_elite_showcase_data.h"

/* D5 is deliberately authored solely through the PUBLIC mesh and 2DPT APIs.
 * No access to D4's private vertex/triangle emitters or private 3D renderer.
 * Two tunnel resource banks are alternated ONLY after a completed frame is
 * accepted, so Core1 never reads a bank being rebuilt by Core0. */
#define D5_SIDES 8u
#define D5_MAX_RINGS 12u
#define D5_VERTS (D5_SIDES*D5_MAX_RINGS)
#define D5_TRIS (D5_SIDES*(D5_MAX_RINGS-1u)*2u)
#define D5_TUNNEL_BYTES (D5_VERTS*6u+D5_TRIS*4u)
#define D5_TUNNEL0 0x700000u
#define D5_TUNNEL1 0x701000u
#define D5_ASP 0x702000u
#define D5_PLUME 0x703000u

static const int16_t d5_ring_x[8]={164,116,0,-116,-164,-116,0,116};
static const int16_t d5_ring_y[8]={0,72,102,72,0,-72,-102,-72};
static uint8_t d5_buffer[D5_TUNNEL_BYTES];
static uint8_t d5_ship_buffer[19u*6u+64u*4u];
static uint8_t d5_plume_buffer[6u*6u+8u*4u];

static struct {
    bool active,ready,forced,ship,zoom,tvc;
    uint8_t bank, next_bank,ring,mode,profile;
    uint16_t phase;
    uint32_t ticks;
    int32_t travel,ship_z;
    twodfx_pose3d_t camera, ship_pose;
} d5;
volatile uint32_t g_d5_public_frames=0u, g_d5_public_faults=0u;
volatile uint32_t g_d5_public_mode=0u,g_d5_public_bank=0u;

static void d5_put16(uint8_t *p,int16_t n){p[0]=(uint8_t)n;p[1]=(uint8_t)((uint16_t)n>>8);}
static int16_t d5_limit(int32_t n)
{if(n>32767)n=32767;if(n< -32768)n=-32768;return (int16_t)n;}
static void d5_vertex(uint8_t *dst,uint16_t idx,int32_t x,int32_t y,int32_t z)
{
    uint8_t *p=dst+(uint32_t)idx*6u;
    d5_put16(p,d5_limit(x));d5_put16(p+2,d5_limit(y));d5_put16(p+4,d5_limit(z));
}
static void d5_face(uint8_t *p,uint16_t idx,uint8_t a,uint8_t b,uint8_t c,uint8_t att)
{p+=(uint32_t)idx*4u;p[0]=a;p[1]=b;p[2]=c;p[3]=att;}
/* The SAME continuous world-centre wave family as D4, but controlled by D5. */
static int32_t d5_sin(int32_t z,int32_t period,uint16_t phase)
{
    uint16_t p=(uint16_t)((int64_t)z*65536/period+phase);
    uint8_t k=(uint8_t)(p>>8),f=(uint8_t)p;
    return ((int32_t)d3_sine_q12[k]*(256-f)+
            (int32_t)d3_sine_q12[(uint8_t)(k+1u)]*f)/256;
}
static void d5_world_centre(int32_t z,int32_t *x,int32_t *y)
{
    *x=(260*d5_sin(z,10000,0)+85*d5_sin(z,21000,9000))/4096;
    *y=(125*d5_sin(z,7800,11000)+45*d5_sin(z,15500,38000))/4096;
}
static void d5_make_ring(uint8_t ring)
{
    uint8_t rings=(d5.mode%3u==0u)?4u:(d5.mode%3u==1u)?8u:12u;
    int32_t spacing=rings==4u?900:rings==8u?450:300;
    int32_t phase=d5.travel%spacing;
    int32_t z=180+(int32_t)ring*spacing-phase;
    int32_t cx,cy,x0,y0;
    d5_world_centre(d5.travel+z,&cx,&cy);
    d5_world_centre(d5.travel,&x0,&y0);
    cx-=x0;cy-=y0;
    /* D4-style gentle twist. The elliptical profile compensates for
     * display pixel aspect and should not undergo large rotations. */
    int32_t world_z=d5.travel+z;
    int32_t scale=1290+320*d5_sin(world_z,10500,23000)/4096;
    int32_t sn=d5_sin(world_z,14500,11000)/11;
    int32_t cs=4096-(sn*sn)/8192;
    for(uint8_t side=0;side<D5_SIDES;side++){
        int32_t x=(int32_t)d5_ring_x[side]*scale/1024;
        int32_t y=(int32_t)d5_ring_y[side]*scale/1024;
        int32_t rx=(x*cs-y*sn)/4096,ry=(x*sn+y*cs)/4096;
        d5_vertex(d5_buffer,(uint16_t)ring*D5_SIDES+side,cx+rx,cy+ry,z+d5.camera.z);
    }
}
static void d5_make_faces(uint8_t rings)
{
    uint8_t *dst=d5_buffer+D5_VERTS*6u;
    uint16_t t=0;
    for(uint8_t ring=0;ring+1u<rings;ring++){
        for(uint8_t s=0;s<D5_SIDES;s++){
            uint8_t n=(uint8_t)((s+1u)%D5_SIDES);
            uint8_t a=(uint8_t)(ring*D5_SIDES+s);
            uint8_t b=(uint8_t)(ring*D5_SIDES+n);
            uint8_t c=(uint8_t)((ring+1u)*D5_SIDES+n);
            uint8_t e=(uint8_t)((ring+1u)*D5_SIDES+s);
            uint8_t colour=d5.tvc?((ring+s)&1u?9u:13u):((ring+s)&1u?5u:7u);
            /* A-B, B-C visible; C-A is the artificial diagonal. */
            d5_face(dst,t++,a,b,c,(uint8_t)(colour|0x30u|0x80u));
            /* A-C hidden, C-E and E-A visible. */
            d5_face(dst,t++,a,c,e,(uint8_t)(colour|0x60u|0x80u));
        }
    }
}
static uint16_t d5_make_asp(void)
{
    memset(d5_ship_buffer,0,sizeof(d5_ship_buffer));
    for(uint8_t i=0;i<19u;i++)
        d5_vertex(d5_ship_buffer,i,d3_asp_vertices[i].x,d3_asp_vertices[i].y,d3_asp_vertices[i].z);
    uint8_t *dst=d5_ship_buffer+19u*6u;uint16_t tris=0;
    for(uint8_t f=0;f<12u;f++){
        const d3_face_t *face=&d3_asp_faces[f];
        uint8_t color=d5.tvc?d3_asp_tvc[f]:d3_asp_ep[f];
        for(uint8_t k=1u;k+1u<face->count;k++){
            uint8_t edges=(uint8_t)(0x20u|(k==1u?0x10u:0u)|
                                    (k+2u==face->count?0x40u:0u));
            /* Two-sided: original Elite faces use per-face normals, not
             * necessarily the general API's winding. This exposes the
             * deliberately simpler public format's occlusion limitations. */
            d5_face(dst,tris++,face->vertex[0],face->vertex[k],face->vertex[k+1u],
                    (uint8_t)(color|edges|0x80u));
        }
    }
    twodfx_upload_raw(D5_ASP,d5_ship_buffer,19u*6u+(uint32_t)tris*4u);
    return tris;
}
static void d5_make_plume(void)
{
    /* A compact rear exhaust: public mesh, deliberately no special effects. */
    static const int16_t p[6][3]={
        {-13,0,0},{13,0,0},{0,-12,0},{0,12,0},{0,0,-54},{0,0,-83}
    };
    for(uint8_t i=0;i<6;i++)d5_vertex(d5_plume_buffer,i,p[i][0],p[i][1],p[i][2]);
    uint8_t *f=d5_plume_buffer+36u;
    for(uint8_t i=0;i<4;i++){
        uint8_t a=(i==0)?0:(i==1)?3:(i==2)?1:2;
        uint8_t b=(i==0)?3:(i==1)?1:(i==2)?2:0;
        d5_face(f,i,a,b,4u,(uint8_t)((d5.tvc?11u:6u)|0x80u));
        d5_face(f,i+4u,a,b,5u,(uint8_t)((d5.tvc?9u:3u)|0x80u));
    }
    twodfx_upload_raw(D5_PLUME,d5_plume_buffer,36u+8u*4u);
    twodfx_3d_define_mesh(2u,D5_PLUME,6u,8u);
}
static void d5_publish(void)
{
    uint8_t rings=(d5.mode%3u==0u)?4u:(d5.mode%3u==1u)?8u:12u;
    uint16_t faces=(uint16_t)(rings-1u)*D5_SIDES*2u;
    uint32_t base=(d5.next_bank==0u)?D5_TUNNEL0:D5_TUNNEL1;
    /* Single upload into the INACTIVE resource bank. The front bank remains
     * untouched until the renderer has finished using it. */
    twodfx_upload_raw(base,d5_buffer,D5_VERTS*6u+(uint32_t)faces*4u);
    twodfx_3d_define_mesh(0u,base,D5_VERTS,faces);
    uint16_t far=(rings==4u)?920u:(rings==8u)?2070u:2490u;
    twodfx_3d_set_projection(365u,300u,96u,far);
    twodfx_3d_set_camera(d5.camera);
    twodfx_reset_2dpt_background();
    twodfx_2d_set_clip_rect(TWODFX_LAYER_BACKGROUND,0,
        TWODFX_BASIC_SAFE_MIN_X,TWODFX_BASIC_SAFE_MIN_Y,
        TWODFX_BASIC_SAFE_MAX_X-TWODFX_BASIC_SAFE_MIN_X+1,
        TWODFX_BASIC_SAFE_MAX_Y-TWODFX_BASIC_SAFE_MIN_Y+1);
    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND,1,d5.tvc?15u:4u,0u);
    twodfx_mesh_mode_t mode=(d5.mode<3u)?TWODFX_MESH_WIREFRAME:TWODFX_MESH_FILLED;
    twodfx_3d_draw_mesh(0u,2u,0u,(twodfx_pose3d_t){0},mode);
    twodfx_2d_ret(TWODFX_LAYER_BACKGROUND,3u);

    twodfx_reset_2dpt_foreground();
    twodfx_2d_set_clip_rect(TWODFX_LAYER_FOREGROUND,0,
        TWODFX_BASIC_SAFE_MIN_X,TWODFX_BASIC_SAFE_MIN_Y,
        TWODFX_BASIC_SAFE_MAX_X-TWODFX_BASIC_SAFE_MIN_X+1,
        TWODFX_BASIC_SAFE_MAX_Y-TWODFX_BASIC_SAFE_MIN_Y+1);
    uint8_t slot=1u;
    if(d5.ship){
        /* Exhaust is positioned in ship local Z by its own mesh vertices. */
        twodfx_3d_draw_mesh(1u,slot++,2u,d5.ship_pose,TWODFX_MESH_FILLED);
        twodfx_2d_set_color(TWODFX_LAYER_FOREGROUND,slot++,d5.tvc?15u:4u,0u);
        twodfx_3d_draw_mesh(1u,slot++,1u,d5.ship_pose,TWODFX_MESH_FWE);
    }
    /* Same six phase/mode pips as D4, but all game logic lives in the egg. */
    twodfx_2d_set_color(TWODFX_LAYER_FOREGROUND,slot++,d5.tvc?15u:4u,0u);
    for(uint8_t k=0u;k<6u;k++){
        uint16_t x=TWODFX_BASIC_SAFE_MIN_X+8u+(uint16_t)k*16u;
        twodfx_2d_fill_rect(TWODFX_LAYER_FOREGROUND,slot++,x,
                  TWODFX_BASIC_SAFE_MIN_Y+8u,k==d5.mode?12u:5u,k==d5.mode?6u:3u);
    }
    twodfx_2d_ret(TWODFX_LAYER_FOREGROUND,slot);
    d5.bank=d5.next_bank;
    g_d5_public_bank=d5.bank;
    g_d5_public_mode=d5.mode;
    ++g_d5_public_frames;
    d5.ready=true;
}
/* Round signed Q8 fixed-point estimates to full public 10-bit angle units.
 * D5 never applies a screen-space compensation or hidden camera precision. */
static int32_t d5_round_angle_q8(int32_t q8)
{return (q8>=0 ? q8+128 : q8-128)/256;}
static void d5_prepare(void)
{
    uint16_t step=(d5.profile==0u)?15u:(d5.profile==1u)?25u:34u;
    d5.travel+=step;
    int32_t cx,cy,lookx,looky,sx,sy;
    d5_world_centre(d5.travel,&cx,&cy);
    d5_world_centre(d5.travel+550,&lookx,&looky);
    /* 1024/(2*pi) ~= 162.97 angle units per radian.
     * Both camera and ship use the same public 10-bit angle scale. */
    int32_t yaw_q8=(lookx-cx)*163*256/550;
    int32_t pitch_q8=-(looky-cy)*163*256/550;
    int32_t yaw=d5_round_angle_q8(yaw_q8);
    int32_t pitch=d5_round_angle_q8(pitch_q8);
    uint16_t spacing=(d5.mode%3u==0u)?900u:(d5.mode%3u==1u)?450u:300u;
    d5.camera=(twodfx_pose3d_t){
        (int16_t)((48*d5_sin((int32_t)d5.ticks,390,9000)/4096)*d5.profile/2),
        (int16_t)((30*d5_sin((int32_t)d5.ticks,510,26000)/4096)*d5.profile/2),
        (int16_t)(d5.travel%spacing),0,(uint16_t)yaw,(uint16_t)pitch};
    if(d5.zoom)d5.ship_z=445+(int32_t)(80*d5_sin((int32_t)d5.ticks,160,0)/4096);
    else d5.ship_z=410;
    d5_world_centre(d5.travel+d5.ship_z,&sx,&sy);
    d5.ship_pose=(twodfx_pose3d_t){d5_limit(sx-cx+d5.camera.x+(32*d5_sin((int32_t)d5.ticks,115,0)/4096)),
                       d5_limit(sy-cy+25+d5.camera.y),d5_limit(d5.ship_z+d5.camera.z),
                       (uint16_t)(d5_sin((int32_t)d5.ticks,200,0)/88),
                       (uint16_t)(yaw/2),0};
    d5.next_bank=(uint8_t)(d5.bank^1u);
    d5.ring=0u;d5.ready=false;
}
void egg_D5_public_mesh_start(void)
{
    memset(&d5,0,sizeof(d5));
    d5.tvc=twodfx_get_machine()==TWODFX_MACHINE_TVC;
    d5.active=true;d5.mode=2u;d5.profile=2u;d5.forced=true;d5.ship=true;d5.zoom=true;
    g_d5_public_frames=g_d5_public_faults=0u;
    twodfx_set_engine_mode(TWODFX_ENGINE_FORCED_ON);
    twodfx_set_fps(TWODFX_FPS_50);
    twodfx_reset_sprites();
    twodfx_3d_define_mesh(1u,D5_ASP,19u,d5_make_asp());
    d5_make_plume();
    d5_prepare();
    for(uint8_t r=0;r<D5_MAX_RINGS;r++)d5_make_ring(r);
    d5_make_faces(D5_MAX_RINGS);
    d5_publish();
}
void egg_D5_public_mesh_stop(void){d5.active=false;d5.ready=true;}
bool egg_D5_public_mesh_ready(void){return !d5.active || d5.ready;}
void egg_D5_public_mesh_tick(void)
{
    if(!d5.active)return;
    uint8_t v;
    while(twodfx_egg_read_f9(&v)){
        if(v<6u){d5.mode=v;d5.forced=true;d5.phase=0u;}
        else if(v==255u){d5.forced=false;d5.phase=0u;}
        else if(v>=240u&&v<=242u)d5.profile=v-240u;
        else if(v==243u)d5.ship=true;
        else if(v==244u)d5.ship=false;
        else if(v==245u)d5.zoom=false;
        else if(v==246u)d5.zoom=true;
    }
    if(!d5.forced && ++d5.phase>=400u){d5.phase=0u;d5.mode=(uint8_t)((d5.mode+1u)%6u);}
    ++d5.ticks;
    d5_prepare();
}
void egg_D5_public_mesh_step(void)
{
    if(!d5.active||d5.ready)return;
    if(d5.ring<D5_MAX_RINGS){d5_make_ring(d5.ring++);return;}
    if(d5.ring==D5_MAX_RINGS){
        uint8_t rings=(d5.mode%3u==0u)?4u:(d5.mode%3u==1u)?8u:12u;
        d5_make_faces(rings);d5.ring++;
        return;
    }
    d5_publish();
}
