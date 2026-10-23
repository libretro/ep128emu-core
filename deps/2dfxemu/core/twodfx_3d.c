/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Károly Vaczkó
 */
/* Portable C99 behavioral 3D, independent mathematical implementation.
 * Not a translation of the RP2354B firmware render algorithm. */
#include "twodfx_3d.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define DFX_PI 3.14159265358979323846
typedef struct { double x, y, z; } v3;
typedef struct { double x,y; } p2;
typedef struct { uint16_t index; double z; } sorted_face;
static uint16_t u16(const uint8_t *p) {
  return (uint16_t)((unsigned)p[0] | ((unsigned)p[1] << 8));
}
static int16_t i16(const uint8_t *p) {
  uint16_t v=u16(p);
  return (int16_t)(v<=32767u ? (int)v : (int)v-65536);
}
void twodfx_3d_unpack_angles(const uint8_t p[4],
                            uint16_t *roll,uint16_t *yaw,uint16_t *pitch) {
  uint32_t b=(uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
  if(roll)*roll=(uint16_t)(b&1023u);
  if(yaw)*yaw=(uint16_t)((b>>10)&1023u);
  if(pitch)*pitch=(uint16_t)((b>>20)&1023u);
}
void twodfx_3d_reset(twodfx_3d_scene *s) {
  if(!s)return;
  memset(s,0,sizeof *s);
  s->projection_shadow.fx=365u;s->projection_shadow.fy=300u;
  s->projection_shadow.near_z=96u;s->projection_shadow.far_z=2490u;
  s->projection_active=s->projection_shadow;
}
void twodfx_3d_latch(twodfx_3d_scene *s) {
  if(!s)return;
  memcpy(s->mesh_active,s->mesh_shadow,sizeof s->mesh_active);
  s->camera_active=s->camera_shadow;
  s->projection_active=s->projection_shadow;
}
void twodfx_3d_define(twodfx_3d_scene *s,unsigned id,uint32_t address,
                       unsigned vertices,unsigned faces) {
  uint64_t bytes=(uint64_t)vertices*6u+(uint64_t)faces*4u;
  if(!s||id>=256u)return;
  s->mesh_shadow[id].address=address;
  s->mesh_shadow[id].vertices=(uint16_t)vertices;
  s->mesh_shadow[id].faces=(uint16_t)(vertices>0u && vertices<=256u &&
    faces>0u && faces<=512u && address<8388608u && bytes<=8388608u-address?faces:0u);
}
void twodfx_3d_scene_set_projection(twodfx_3d_scene *s,uint16_t fx,
                              uint16_t fy,uint16_t near_z,uint16_t far_z) {
  if(!s||!fx||!fy||!near_z||far_z<=near_z)return;
  s->projection_shadow.fx=fx;s->projection_shadow.fy=fy;
  s->projection_shadow.near_z=near_z;s->projection_shadow.far_z=far_z;
}
void twodfx_3d_scene_set_camera(twodfx_3d_scene *s,const uint8_t p[10]) {
  if(!s||!p)return;
  s->camera_shadow.x=i16(p);
  s->camera_shadow.y=i16(p+2);
  s->camera_shadow.z=i16(p+4);
  twodfx_3d_unpack_angles(p+6,&s->camera_shadow.roll,
    &s->camera_shadow.yaw,&s->camera_shadow.pitch);
}
static v3 rotate(v3 v,unsigned roll,unsigned yaw,unsigned pitch) {
  double a=(double)roll*(2.0*DFX_PI/1024.0);
  double b=(double)yaw*(2.0*DFX_PI/1024.0);
  double c=(double)pitch*(2.0*DFX_PI/1024.0);
  double sn=sin(a),cs=cos(a),x=cs*v.x-sn*v.y,y=sn*v.x+cs*v.y;
  v.x=x;v.y=y;
  sn=sin(b);cs=cos(b);
  x=cs*v.x+sn*v.z;v.z=-sn*v.x+cs*v.z;v.x=x;
  sn=sin(c);cs=cos(c);
  y=cs*v.y-sn*v.z;v.z=sn*v.y+cs*v.z;v.y=y;
  return v;
}
static v3 inverse_rotate(v3 v,const twodfx_3d_pose *p) {
  double a=(double)p->pitch*(2.0*DFX_PI/1024.0);
  double sn=sin(a),cs=cos(a),y=cs*v.y+sn*v.z;
  v.z=-sn*v.y+cs*v.z;v.y=y;
  a=(double)p->yaw*(2.0*DFX_PI/1024.0);sn=sin(a);cs=cos(a);
  {double x=cs*v.x-sn*v.z;v.z=sn*v.x+cs*v.z;v.x=x;}
  a=(double)p->roll*(2.0*DFX_PI/1024.0);sn=sin(a);cs=cos(a);
  {double x=cs*v.x+sn*v.y;v.y=-sn*v.x+cs*v.y;v.x=x;}
  return v;
}
static v3 interp(v3 a,v3 b,double t) {
  v3 p={a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t};return p;
}
/* Sutherland-Hodgman in camera space: clip near then far, at most 5 vertices. */
static unsigned zclip(const v3 *in,unsigned n,v3 *out,double plane,int minimum) {
  unsigned i,m=0;
  v3 a;int ai;
  if(!n)return 0;
  a=in[n-1];ai=minimum?a.z>=plane:a.z<=plane;
  for(i=0;i<n;i++) {
    v3 b=in[i];int bi=minimum?b.z>=plane:b.z<=plane;
    if(ai!=bi && m<8) {
      double t=(plane-a.z)/(b.z-a.z);
      out[m++]=interp(a,b,t);
    }
    if(bi && m<8)out[m++]=b;
    a=b;ai=bi;
  }
  return m;
}
static p2 project(v3 a,const twodfx_3d_projection *p,double cx,double cy) {
  p2 r={cx+a.x*(double)p->fx/a.z,cy-a.y*(double)p->fy/a.z};return r;
}
/* Clip a segment to raster bounds before invoking integer Bresenham: huge
 * projected coordinates cannot overflow the generic raster line routine. */
static void clipped_line(twodfx_raster *r,p2 a,p2 b,uint8_t colour) {
  double dx=b.x-a.x,dy=b.y-a.y,t0=0.0,t1=1.0;
  double p[4]={-dx,dx,-dy,dy};
  double q[4]={a.x-r->x0,r->x1-a.x,a.y-r->y0,r->y1-a.y};
  unsigned i;
  for(i=0;i<4u;i++) {
    if(fabs(p[i])<1e-12) {if(q[i]<0)return;}
    else {
      double t=q[i]/p[i];
      if(p[i]<0){if(t>t0)t0=t;}
      else if(t<t1)t1=t;
      if(t0>t1)return;
    }
  }
  {p2 c={a.x+dx*t0,a.y+dy*t0},d={a.x+dx*t1,a.y+dy*t1};
   twodfx_raster_line(r,(int)lround(c.x),(int)lround(c.y),
                      (int)lround(d.x),(int)lround(d.y),colour);}
}
/* Polygon clipping in projected space bounds the fill to the framebuffer,
 * preventing giant off-screen polygons from creating integer overflow. */
static int inside(p2 p,unsigned side,double limit) {
  if(side==0)return p.x>=limit;
  if(side==1)return p.x<=limit;
  if(side==2)return p.y>=limit;
  return p.y<=limit;
}
static p2 intersect(p2 a,p2 b,unsigned side,double limit) {
  double u=(side<2)?(limit-a.x)/(b.x-a.x):(limit-a.y)/(b.y-a.y);
  p2 p={a.x+(b.x-a.x)*u,a.y+(b.y-a.y)*u};return p;
}
static unsigned plane_clip(const p2 *in,unsigned n,p2 *out,unsigned side,double limit) {
  unsigned i,k=0; p2 a;int ai;
  if(!n)return 0;
  a=in[n-1];ai=inside(a,side,limit);
  for(i=0;i<n;i++) {
    p2 b=in[i];int bi=inside(b,side,limit);
    if(ai!=bi && k<16)out[k++]=intersect(a,b,side,limit);
    if(bi && k<16)out[k++]=b;
    a=b;ai=bi;
  }
  return k;
}
static void fill_clipped(twodfx_raster *r,const v3 *in,unsigned count,
                         const twodfx_3d_projection *p,double cx,double cy,
                         uint8_t colour) {
  p2 a[16],b[16];unsigned i,k=count;
  if(count<3)return;
  for(i=0;i<count;i++)a[i]=project(in[i],p,cx,cy);
  k=plane_clip(a,k,b,0,(double)r->x0);
  k=plane_clip(b,k,a,1,(double)r->x1);
  k=plane_clip(a,k,b,2,(double)r->y0);
  k=plane_clip(b,k,a,3,(double)r->y1);
  for(i=1;i+1<k;i++)
    twodfx_raster_fill_triangle(r,(int)lround(a[0].x),(int)lround(a[0].y),
      (int)lround(a[i].x),(int)lround(a[i].y),
      (int)lround(a[i+1].x),(int)lround(a[i+1].y),colour);
}
static void edge(twodfx_raster *r,v3 a,v3 b,const twodfx_3d_projection *p,
                 double cx,double cy,uint8_t colour) {
  double near=(double)p->near_z,far=(double)p->far_z;
  if ((a.z<near&&b.z<near)||(a.z>far&&b.z>far))return;
  if(a.z<near)a=interp(a,b,(near-a.z)/(b.z-a.z));
  else if(b.z<near)b=interp(a,b,(near-a.z)/(b.z-a.z));
  if(a.z>far)a=interp(a,b,(far-a.z)/(b.z-a.z));
  else if(b.z>far)b=interp(a,b,(far-a.z)/(b.z-a.z));
  clipped_line(r,project(a,p,cx,cy),project(b,p,cx,cy),colour);
}
static int front_face(v3 a,v3 b,v3 c) {
  v3 u={b.x-a.x,b.y-a.y,b.z-a.z},v={c.x-a.x,c.y-a.y,c.z-a.z};
  v3 n={u.y*v.z-u.z*v.y,u.z*v.x-u.x*v.z,u.x*v.y-u.y*v.x};
  return n.x*a.x+n.y*a.y+n.z*a.z<0.0;
}
void twodfx_3d_render(const twodfx_3d_scene *s,const uint8_t *ram,
                      twodfx_raster *r,const uint8_t entry[11],
                      unsigned mode,uint8_t edge_colour,int ox,int oy,
                      int origin_x,int origin_y,int optical_x,int optical_y) {
  v3 points[256];sorted_face order[512];
  const twodfx_3d_mesh *m;
  unsigned i,j,n=0,roll,yaw,pitch;
  uint32_t table;
  double cx=(double)(optical_x+ox-origin_x);
  double cy=(double)(optical_y+oy-origin_y);
  if(!s||!ram||!r||!entry||mode>2u)return;
  m=&s->mesh_active[entry[0]];
  if(!m->vertices||!m->faces||m->vertices>256||m->faces>512||
     m->address>=8388608u ||
     (uint64_t)m->address+(uint64_t)m->vertices*6u+(uint64_t)m->faces*4u>8388608u)return;
  {uint16_t rr,yy,pp;twodfx_3d_unpack_angles(entry+7,&rr,&yy,&pp);
   roll=rr;yaw=yy;pitch=pp;}
  for(i=0;i<m->vertices;i++) {
    const uint8_t *a=ram+m->address+i*6u;
    v3 v={(double)i16(a),(double)i16(a+2),(double)i16(a+4)};
    v=rotate(v,roll,yaw,pitch);
    v.x+=(double)i16(entry+1)-(double)s->camera_active.x;
    v.y+=(double)i16(entry+3)-(double)s->camera_active.y;
    v.z+=(double)i16(entry+5)-(double)s->camera_active.z;
    points[i]=inverse_rotate(v,&s->camera_active);
  }
  table=m->address+(uint32_t)m->vertices*6u;
  for(i=0;i<m->faces;i++) {
    const uint8_t *t=ram+table+i*4u;
    v3 a,b,c;double depth;
    unsigned k;
    if(t[0]>=m->vertices||t[1]>=m->vertices||t[2]>=m->vertices)continue;
    a=points[t[0]];b=points[t[1]];c=points[t[2]];
    if(!(t[3]&0x80u)&&!front_face(a,b,c))continue;
    if((a.z<s->projection_active.near_z&&b.z<s->projection_active.near_z&&c.z<s->projection_active.near_z)||
       (a.z>s->projection_active.far_z&&b.z>s->projection_active.far_z&&c.z>s->projection_active.far_z))continue;
    depth=(a.z+b.z+c.z)/3.0;k=n++;
    while(k>0u && order[k-1u].z<depth){order[k]=order[k-1u];k--;}
    order[k].z=depth;order[k].index=(uint16_t)i;
  }
  for(j=0;j<n;j++) {
    const uint8_t *t=ram+table+(uint32_t)order[j].index*4u;
    v3 a=points[t[0]],b=points[t[1]],c=points[t[2]];
    if(mode!=0u) {
      v3 aa[8],bb[8];unsigned count;
      aa[0]=a;aa[1]=b;aa[2]=c;
      count=zclip(aa,3u,bb,(double)s->projection_active.near_z,1);
      count=zclip(bb,count,aa,(double)s->projection_active.far_z,0);
      fill_clipped(r,aa,count,&s->projection_active,cx,cy,(uint8_t)(t[3]&15u));
    }
    if(mode!=1u){
      if(t[3]&0x10u)edge(r,a,b,&s->projection_active,cx,cy,edge_colour);
      if(t[3]&0x20u)edge(r,b,c,&s->projection_active,cx,cy,edge_colour);
      if(t[3]&0x40u)edge(r,c,a,&s->projection_active,cx,cy,edge_colour);
    }
  }
}
