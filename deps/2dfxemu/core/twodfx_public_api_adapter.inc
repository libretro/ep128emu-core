/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Károly Vaczkó
 *
 * Portable implementation of the public v1.1.0 twodfx_api.h contract.
 * This is independently written against the portable emulator model;
 * NO MCU twodfx_api.c, 2dfx.h, PIO/DMA, IRQ or scanout code is copied.
 *
 * Bound for the duration of one start/tick/SHOW_FW callback. The original
 * egg source uses file-static state: just as in the source firmware, only one
 * instance can run eggs at a time. Host API operations use the same retained
 * shadow structures and renderer as F8/F9 commands.
 */
#define TWODFX_RESOURCE_JOB_NONE TWODFX_FIRMWARE_JOB_NONE
#define TWODFX_RESOURCE_JOB_BUSY TWODFX_FIRMWARE_JOB_BUSY
#define TWODFX_RESOURCE_JOB_SUCCESS TWODFX_FIRMWARE_JOB_SUCCESS
#define TWODFX_RESOURCE_JOB_ERROR TWODFX_FIRMWARE_JOB_ERROR
#include "../firmware_eggs/twodfx_api.h"
#undef TWODFX_RESOURCE_JOB_NONE
#undef TWODFX_RESOURCE_JOB_BUSY
#undef TWODFX_RESOURCE_JOB_SUCCESS
#undef TWODFX_RESOURCE_JOB_ERROR
#include "../firmware_eggs/twodfx_egg.h"

static twodfx_emu *egg_api_device;
static void egg_api_bind(twodfx_emu *d) { egg_api_device=d; }

static void egg_api_put(unsigned layer,uint8_t slot,uint8_t op,const uint8_t *p,unsigned n){
  twodfx_emu *d=egg_api_device;
  twodfx_2dpt_entry *e;
  if(!d || layer>1u || n>sizeof(d->shadow_2dpt[0][0].payload))return;
  e=&d->shadow_2dpt[layer][slot];
  memset(e,0,sizeof *e);e->opcode=op;e->nbytes=(uint8_t)n;
  if(n)memcpy(e->payload,p,n);
}
static uint8_t egg_api_hi(unsigned x,unsigned y,unsigned w,unsigned h,unsigned stroke) {
  return (uint8_t)((((stroke-1u)&3u)<<6)|(((x>>8)&3u)<<4)|
                 (((y>>8)&1u)<<3)|(((w>>8)&3u)<<1)|((h>>8)&1u));
}
static void egg_api_rectcmd(twodfx_layer_t layer,uint8_t slot,uint8_t op,
       uint16_t x,uint16_t y,uint16_t w,uint16_t h,uint8_t stroke,
       uint8_t b6,uint8_t b7,unsigned n){
  uint8_t p[7]={egg_api_hi(x,y,w,h,stroke),(uint8_t)x,(uint8_t)y,
                (uint8_t)w,(uint8_t)h,b6,b7};
  egg_api_put(layer,slot,op,p,n);
}
static void egg_api_linecmd(twodfx_layer_t layer,uint8_t slot,uint8_t op,
      uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1,uint8_t stroke){
  egg_api_rectcmd(layer,slot,op,x0,y0,x1,y1,stroke,0,0,5);
}
static void egg_api_tri(twodfx_layer_t layer,uint8_t slot,uint8_t op,
      uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2){
  uint8_t p[8]={egg_api_hi(x0,y0,x1,y1,1),egg_api_hi(x2,y2,0,0,1),
                (uint8_t)x0,(uint8_t)y0,(uint8_t)x1,(uint8_t)y1,
                (uint8_t)x2,(uint8_t)y2};
  egg_api_put(layer,slot,op,p,8);
}
static void egg_api_upload(uint32_t dst,const void *source,uint32_t len,unsigned format){
  twodfx_emu *d=egg_api_device;
  const uint8_t *p=(const uint8_t*)source;
  uint32_t i;
  if(!d||(!p && len))return;
  for(i=0;i<len;i++)d->ram[(dst+i)&(TWODFX_EMU_RESOURCE_BYTES-1u)]=source_to_internal(p[i],(uint8_t)format);
}

twodfx_machine_t twodfx_get_machine(void){
  return egg_api_device&&egg_api_device->machine==TWODFX_EMU_TVC?TWODFX_MACHINE_TVC:TWODFX_MACHINE_ENTERPRISE;
}
void twodfx_set_engine_mode(twodfx_engine_mode_t mode){
  if(egg_api_device)egg_api_device->engine=(uint8_t)mode;
}
void twodfx_set_fps(twodfx_fps_t fps){if(egg_api_device)egg_api_device->fps=fps==TWODFX_FPS_25?25:50;}
void twodfx_set_transparent_color(uint8_t c){if(egg_api_device)egg_api_device->transparent=c&15u;}
void twodfx_set_render_overrun_visual(bool enabled){if(egg_api_device)egg_api_device->overrun_visual=enabled;}
void twodfx_clear_render_overrun(void){}
void twodfx_set_raster_interrupt(uint8_t n,uint16_t line){
  if(egg_api_device&&n>=1&&n<=4){egg_api_device->raster_lines[n-1u]=line;
    egg_api_device->raster_pending&=(uint8_t)~(1u<<(n-1u));}
}
void twodfx_clear_raster_pending(void){if(egg_api_device)egg_api_device->raster_pending=0;}
void twodfx_show_builtin_logo(uint16_t x,uint16_t y,uint8_t main,uint8_t accent){
  twodfx_emu *d=egg_api_device;if(!d)return;
  d->logo_visible=1;d->logo_x=x;d->logo_y=y;d->logo_main=main&15u;d->logo_accent=accent&15u;
}
void twodfx_hide_builtin_logo(void){if(egg_api_device)egg_api_device->logo_visible=0;}
void twodfx_reset_sprites(void){if(egg_api_device)memset(egg_api_device->shadow_spt,0,sizeof egg_api_device->shadow_spt);}
static void egg_api_reset_layer(unsigned layer){
  twodfx_emu *d=egg_api_device;
  if(!d||layer>1u)return;
  memset(d->shadow_2dpt[layer],0,sizeof d->shadow_2dpt[layer]);
  d->shadow_2dpt[layer][0].opcode=0x7f;
}
void twodfx_reset_2dpt_background(void){egg_api_reset_layer(0);}
void twodfx_reset_2dpt_foreground(void){egg_api_reset_layer(1);}
void twodfx_reset_2dpt_all(void){egg_api_reset_layer(0);egg_api_reset_layer(1);}
void twodfx_reset_resource_descriptors(void){
  twodfx_emu *d=egg_api_device;if(!d)return;
  memset(d->fonts,0,sizeof d->fonts);memset(d->bitmaps,0,sizeof d->bitmaps);
  memset(d->patterns,0,sizeof d->patterns);memset(d->tilesets,0,sizeof d->tilesets);
  memset(d->tilemaps,0,sizeof d->tilemaps);
}
void twodfx_reset_raster_interrupts(void){
  if(egg_api_device){memset(egg_api_device->raster_lines,0,sizeof egg_api_device->raster_lines);
    egg_api_device->raster_pending=0;}
}
void twodfx_upload_raw(uint32_t addr,const void *src,uint32_t n){egg_api_upload(addr,src,n,0);}
void twodfx_upload_nick(uint32_t addr,const void *src,uint32_t n){egg_api_upload(addr,src,n,1);}
void twodfx_upload_tvc(uint32_t addr,const void *src,uint32_t n){egg_api_upload(addr,src,n,2);}
void twodfx_rram_fill(uint32_t addr,uint8_t value,uint32_t n){
  twodfx_emu *d=egg_api_device;uint32_t i;if(!d)return;
  for(i=0;i<n;i++)d->ram[(addr+i)&(TWODFX_EMU_RESOURCE_BYTES-1u)]=value;
}
twodfx_resource_job_status_t twodfx_resource_job_status(void){
  twodfx_emu *d=egg_api_device;
  return d?(twodfx_resource_job_status_t)d->resource_job:TWODFX_FIRMWARE_JOB_ERROR;
}
bool twodfx_reset_rram(void){
  twodfx_emu *d=egg_api_device;
  if(!d||(int)d->resource_job==(int)TWODFX_FIRMWARE_JOB_BUSY)return false;
  memset(d->ram,0,TWODFX_EMU_RESOURCE_BYTES);d->resource_job=TWODFX_FIRMWARE_JOB_SUCCESS;
  return true;
}
static bool egg_api_zx1(uint32_t src,uint32_t dst,uint8_t format){
  twodfx_emu *d=egg_api_device;
  if(!d||(int)d->resource_job==(int)TWODFX_FIRMWARE_JOB_BUSY)return false;
  d->job_source=src;d->job_destination=dst;d->job_format=format;
  d->resource_job=TWODFX_FIRMWARE_JOB_BUSY;
  return true;
}
bool twodfx_unzx1_raw(uint32_t a,uint32_t b){return egg_api_zx1(a,b,0);}
bool twodfx_unzx1_nick(uint32_t a,uint32_t b){return egg_api_zx1(a,b,1);}
bool twodfx_unzx1_tvc(uint32_t a,uint32_t b){return egg_api_zx1(a,b,2);}
void twodfx_define_font(uint8_t id,uint32_t addr,uint8_t wu,uint8_t hu){
  twodfx_emu *d=egg_api_device;twodfx_font_descriptor *f;
  if(!d||id>=64||wu<1||wu>16||hu<1||hu>16)return;
  f=&d->fonts[id];f->resource_address=addr&0x7fffffu;f->width_units=wu;f->height_units=hu;f->valid=1;
}
void twodfx_define_bitmap(uint8_t id,uint32_t addr,uint16_t w,uint16_t h){
  twodfx_emu *d=egg_api_device;twodfx_bitmap_descriptor *b;
  if(!d||w>1023||h>511)return;
  b=&d->bitmaps[id];b->resource_address=addr&0x7fffffu;b->width=w&~1u;b->height=h;
  b->valid=b->width&&h;
}
void twodfx_define_pattern(uint8_t id,uint32_t addr){
  if(egg_api_device&&id<64){egg_api_device->patterns[id].resource_address=addr&0x7fffffu;egg_api_device->patterns[id].valid=1;}
}
void twodfx_define_tileset(uint8_t id,uint32_t addr){
  if(egg_api_device&&id<64){egg_api_device->tilesets[id].resource_address=addr&0x7fffffu;egg_api_device->tilesets[id].valid=1;}
}
void twodfx_define_tilemap(uint8_t id,uint32_t addr,uint16_t w,uint16_t h,uint8_t tileset){
  twodfx_emu *d=egg_api_device;twodfx_tilemap_descriptor *t;
  if(!d||id>=32||w==0||w>256||h==0||h>256||tileset>=64)return;
  t=&d->tilemaps[id];t->resource_address=addr&0x7fffffu;t->width_cells=w;
  t->height_cells=h;t->tileset=tileset;t->valid=1;
}
void twodfx_sprite_set(uint8_t id,uint32_t addr,uint16_t x,uint16_t y,
      uint16_t w,uint16_t h,bool enabled,bool collision,uint8_t transforms){
  twodfx_emu *d=egg_api_device;uint8_t *p;
  if(!d||id>=64)return;
  p=d->shadow_spt[id];memset(p,0,9);
  p[0]=(uint8_t)((enabled?0x80u:0u)|(collision?0x40u:0u)|
                 (((x>>8)&3u)<<4)|(((y>>8)&1u)<<3)|(((w>>8)&3u)<<1)|((h>>8)&1u));
  p[1]=(uint8_t)x;p[2]=(uint8_t)y;
  p[3]=(uint8_t)addr;p[4]=(uint8_t)(addr>>8);p[5]=(uint8_t)(addr>>16);
  p[6]=transforms&0xf0u;p[7]=(uint8_t)w;p[8]=(uint8_t)h;
}
void twodfx_2d_nop(twodfx_layer_t l,uint8_t s){egg_api_put(l,s,0x60,0,0);}
void twodfx_2d_ret(twodfx_layer_t l,uint8_t s){egg_api_put(l,s,0x7f,0,0);}
void twodfx_2d_set_color(twodfx_layer_t l,uint8_t s,uint8_t fg,uint8_t bg){
  uint8_t p=(uint8_t)(((fg&15u)<<4)|(bg&15u));egg_api_put(l,s,0x61,&p,1);
}
void twodfx_2d_set_xy_offset(twodfx_layer_t l,uint8_t s,int8_t x,int8_t y){
  uint8_t p[2]={(uint8_t)x,(uint8_t)y};egg_api_put(l,s,0x6e,p,2);
}
void twodfx_2d_set_clip_rect(twodfx_layer_t l,uint8_t s,uint16_t x,uint16_t y,uint16_t w,uint16_t h){
  egg_api_rectcmd(l,s,0x79,x,y,w,h,1,0,0,5);
}
void twodfx_2d_skip_next(twodfx_layer_t l,uint8_t s,uint8_t count){egg_api_put(l,s,0x6f,&count,1);}
void twodfx_2d_plot(twodfx_layer_t l,uint8_t s,uint16_t x,uint16_t y){
  uint8_t p[3]={egg_api_hi(x,y,0,0,1),(uint8_t)x,(uint8_t)y};egg_api_put(l,s,0x62,p,3);
}
void twodfx_2d_line(twodfx_layer_t l,uint8_t s,uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1,uint8_t stroke){
  egg_api_linecmd(l,s,0x63,x0,y0,x1,y1,stroke);
}
void twodfx_2d_erase_line(twodfx_layer_t l,uint8_t s,uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1,uint8_t stroke){
  egg_api_linecmd(l,s,0x64,x0,y0,x1,y1,stroke);
}
void twodfx_2d_bucket_fill(twodfx_layer_t l,uint8_t s,uint16_t x,uint16_t y){
  uint8_t p[3]={egg_api_hi(x,y,0,0,1),(uint8_t)x,(uint8_t)y};egg_api_put(l,s,0x65,p,3);
}
void twodfx_2d_fill_rect(twodfx_layer_t l,uint8_t s,uint16_t x,uint16_t y,uint16_t w,uint16_t h){
  egg_api_rectcmd(l,s,0x67,x,y,w,h,1,0,0,5);
}
void twodfx_2d_rect(twodfx_layer_t l,uint8_t s,uint16_t x,uint16_t y,uint16_t w,uint16_t h,uint8_t stroke){
  egg_api_rectcmd(l,s,0x66,x,y,w,h,stroke,0,0,5);
}
void twodfx_2d_fill_round_rect(twodfx_layer_t l,uint8_t s,uint16_t x,uint16_t y,uint16_t w,uint16_t h,uint8_t rx,uint8_t ry){
  egg_api_rectcmd(l,s,0x69,x,y,w,h,1,rx,ry,7);
}
void twodfx_2d_round_rect(twodfx_layer_t l,uint8_t s,uint16_t x,uint16_t y,uint16_t w,uint16_t h,uint8_t st,uint8_t rx,uint8_t ry){
  egg_api_rectcmd(l,s,0x68,x,y,w,h,st,rx,ry,7);
}
void twodfx_2d_fill_ellipse(twodfx_layer_t l,uint8_t s,uint16_t x,uint16_t y,uint16_t rx,uint16_t ry){
  egg_api_rectcmd(l,s,0x6b,x,y,rx,ry,1,0,0,5);
}
void twodfx_2d_ellipse(twodfx_layer_t l,uint8_t s,uint16_t x,uint16_t y,uint16_t rx,uint16_t ry,uint8_t st){
  egg_api_rectcmd(l,s,0x6a,x,y,rx,ry,st,0,0,5);
}
void twodfx_2d_triangle(twodfx_layer_t l,uint8_t s,uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2){
  egg_api_tri(l,s,0x75,x0,y0,x1,y1,x2,y2);
}
void twodfx_2d_fill_triangle(twodfx_layer_t l,uint8_t s,uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2){
  egg_api_tri(l,s,0x76,x0,y0,x1,y1,x2,y2);
}
void twodfx_2d_arc(twodfx_layer_t l,uint8_t s,uint16_t x,uint16_t y,uint16_t rx,uint16_t ry,uint8_t a,uint8_t b,uint8_t st){
  egg_api_rectcmd(l,s,0x6c,x,y,rx,ry,st,a,b,7);
}
void twodfx_2d_pie(twodfx_layer_t l,uint8_t s,uint16_t x,uint16_t y,uint16_t rx,uint16_t ry,uint8_t a,uint8_t b){
  egg_api_rectcmd(l,s,0x6d,x,y,rx,ry,1,a,b,7);
}
void twodfx_2d_draw_text(twodfx_layer_t l,uint8_t s,uint16_t x,uint16_t y,uint8_t font,uint32_t addr){
  uint8_t p[7]={egg_api_hi(x,y,0,0,1),(uint8_t)x,(uint8_t)y,font,
                (uint8_t)addr,(uint8_t)(addr>>8),(uint8_t)(addr>>16)};
  egg_api_put(l,s,0x70,p,7);
}
void twodfx_2d_blit(twodfx_layer_t l,uint8_t s,uint16_t x,uint16_t y,uint8_t id,twodfx_blit_mode_t mode){
  uint8_t p[4]={(uint8_t)(egg_api_hi(x,y,0,0,1)|(mode==TWODFX_BLIT_TRANSPARENT?0x80u:0)),
                (uint8_t)x,(uint8_t)y,id};egg_api_put(l,s,0x71,p,4);
}
static void egg_api_subblit(twodfx_layer_t l,uint8_t slot,uint8_t id,twodfx_blit_mode_t mode,
           uint16_t sx,uint16_t sy,uint16_t sw,uint16_t sh,uint16_t dx,uint16_t dy,uint16_t dw,uint16_t dh,int stretch){
  uint8_t p[11]={0};
  p[0]=(uint8_t)(egg_api_hi(sx,sy,sw,sh,1)|(mode==TWODFX_BLIT_TRANSPARENT?0x80u:0));
  p[1]=(uint8_t)sx;p[2]=(uint8_t)sy;p[3]=(uint8_t)sw;p[4]=(uint8_t)sh;
  p[5]=egg_api_hi(dx,dy,stretch?dw:0,stretch?dh:0,1);
  p[6]=(uint8_t)dx;p[7]=(uint8_t)dy;
  if(stretch){p[8]=(uint8_t)dw;p[9]=(uint8_t)dh;p[10]=id;}
  else p[8]=id;
  egg_api_put(l,slot,stretch?0x78:0x77,p,stretch?11:9);
}
void twodfx_2d_crop_blit(twodfx_layer_t l,uint8_t s,uint8_t id,twodfx_blit_mode_t m,
 uint16_t sx,uint16_t sy,uint16_t sw,uint16_t sh,uint16_t dx,uint16_t dy){
  egg_api_subblit(l,s,id,m,sx,sy,sw,sh,dx,dy,sw,sh,0);
}
void twodfx_2d_stretch_blit(twodfx_layer_t l,uint8_t s,uint8_t id,twodfx_blit_mode_t m,
 uint16_t sx,uint16_t sy,uint16_t sw,uint16_t sh,uint16_t dx,uint16_t dy,uint16_t dw,uint16_t dh){
  egg_api_subblit(l,s,id,m,sx,sy,sw,sh,dx,dy,dw,dh,1);
}
void twodfx_2d_pattern_fill(twodfx_layer_t l,uint8_t s,uint16_t x,uint16_t y,
 uint16_t w,uint16_t h,uint8_t pat,uint8_t n){egg_api_rectcmd(l,s,0x72,x,y,w,h,1,pat,n,7);}
void twodfx_2d_pattern_bucket_fill(twodfx_layer_t l,uint8_t s,uint16_t x,uint16_t y,uint8_t pat,uint8_t n){
  uint8_t p[5]={egg_api_hi(x,y,0,0,1),(uint8_t)x,(uint8_t)y,pat,n};
  egg_api_put(l,s,0x74,p,5);
}
void twodfx_2d_tilemap(twodfx_layer_t l,uint8_t s,uint16_t x,uint16_t y,uint8_t map,
 uint8_t srcx,uint8_t srcy,uint8_t cols,uint8_t rows){
  uint8_t p[7]={(uint8_t)(((map&31u)<<3)|(((x>>8)&3u)<<1)|((y>>8)&1u)),
                (uint8_t)x,(uint8_t)y,srcx,srcy,cols,rows};
  egg_api_put(l,s,0x73,p,7);
}
bool twodfx_collision_detected(void){return egg_api_device && egg_api_device->collision_reported;}
bool twodfx_render_overrun_detected(void){return false;}
bool twodfx_raster_interrupt_pending(uint8_t n){return egg_api_device && n>0&&n<=4&&((egg_api_device->raster_pending>>(n-1u))&1u);}
void twodfx_3d_define_mesh(uint8_t id,uint32_t addr,uint16_t nv,uint16_t nt){
  if(egg_api_device)twodfx_3d_define(&egg_api_device->scene3d,id,addr,nv,nt);
}
void twodfx_3d_set_projection(uint16_t fx,uint16_t fy,uint16_t nearz,uint16_t farz){
  if(egg_api_device)twodfx_3d_scene_set_projection(&egg_api_device->scene3d,fx,fy,nearz,farz);
}
void twodfx_3d_set_camera(twodfx_pose3d_t pose){
  uint8_t p[10]={(uint8_t)pose.x,(uint8_t)(pose.x>>8),
                 (uint8_t)pose.y,(uint8_t)(pose.y>>8),
                 (uint8_t)pose.z,(uint8_t)(pose.z>>8)};
  if(!egg_api_device)return;
  twodfx_pack_angles10(p+6,pose.roll,pose.yaw,pose.pitch);
  twodfx_3d_scene_set_camera(&egg_api_device->scene3d,p);
}
void twodfx_3d_draw_mesh(uint8_t layer,uint8_t slot,uint8_t id,twodfx_pose3d_t pose,twodfx_mesh_mode_t mode){
  uint8_t p[11]={id,(uint8_t)pose.x,(uint8_t)(pose.x>>8),
                 (uint8_t)pose.y,(uint8_t)(pose.y>>8),
                 (uint8_t)pose.z,(uint8_t)(pose.z>>8)};
  twodfx_pack_angles10(p+7,pose.roll,pose.yaw,pose.pitch);
  egg_api_put(layer,slot,(uint8_t)(0x7a+mode),p,11);
}
bool twodfx_egg_read_f9(uint8_t *value){return egg_api_device&&twodfx_emu_egg_pop_input(egg_api_device,value);}
bool twodfx_egg_f9_overflowed(void){return egg_api_device&&egg_api_device->egg_overflow;}
void twodfx_egg_clear_f9_overflow(void){if(egg_api_device)egg_api_device->egg_overflow=0;}
