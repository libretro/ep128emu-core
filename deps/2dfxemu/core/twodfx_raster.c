/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Károly Vaczkó
 */
#include "twodfx_raster.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int twodfx_raster_bind(twodfx_raster *r, uint8_t *colour, uint8_t *coverage,
                       unsigned width, unsigned height) {
  if (!r || !colour || !coverage || !width || !height ||
      width > 65535u || height > 65535u ||
      (size_t)width > SIZE_MAX / height) return 0;
  r->colour = colour; r->cover = coverage;
  r->width = width; r->height = height;
  r->x0 = r->y0 = 0;
  r->x1 = (int)width-1; r->y1 = (int)height-1;
  return 1;
}
void twodfx_raster_clear(twodfx_raster *r) {
  if (!r || !r->colour || !r->cover) return;
  memset(r->colour, 0, (size_t)r->width*r->height);
  memset(r->cover, 0, (size_t)r->width*r->height);
}
void twodfx_raster_clip(twodfx_raster *r, int x0, int y0, int x1, int y1) {
  if (!r) return;
  if (x0 < 0) x0=0;
  if (y0 < 0) y0=0;
  if (x1 >= (int)r->width) x1=(int)r->width-1;
  if (y1 >= (int)r->height) y1=(int)r->height-1;
  r->x0=x0; r->y0=y0; r->x1=x1; r->y1=y1;
}
static int inside(const twodfx_raster *r, int x, int y) {
  return r && r->colour && r->cover &&
         x >= r->x0 && x <= r->x1 && y >= r->y0 && y <= r->y1;
}
void twodfx_raster_plot(twodfx_raster *r, int x, int y, uint8_t colour) {
  if (inside(r,x,y)) {
    size_t p=(size_t)y*r->width+(unsigned)x;
    r->colour[p]=(uint8_t)(colour&15u);
    r->cover[p]=1;
  }
}
static unsigned outcode(const twodfx_raster *r,int x,int y) {
  return (x<r->x0?1u:0u)|(x>r->x1?2u:0u)|
         (y<r->y0?4u:0u)|(y>r->y1?8u:0u);
}
void twodfx_raster_line(twodfx_raster *r,int x0,int y0,int x1,int y1,uint8_t colour) {
  unsigned a,b,tries=0;
  int64_t ax=x0,ay=y0,bx=x1,by=y1;
  int dx,dy,sx,sy,err;
  if (!r || r->x1<r->x0 || r->y1<r->y0) return;
  /* Valid 2dfx geometry is many orders smaller. Reject extreme integers
     so clipping arithmetic cannot overflow on hostile input. */
  if (ax < -1000000 || ax > 1000000 || ay < -1000000 || ay > 1000000 ||
      bx < -1000000 || bx > 1000000 || by < -1000000 || by > 1000000) return;
  /* Cohen-Sutherland line clipping: avoids unbounded Bresenham loops. */
  for (;;) {
    int64_t nx,ny;
    unsigned c;
    a=outcode(r,(int)ax,(int)ay); b=outcode(r,(int)bx,(int)by);
    if (!(a|b)) break;
    if ((a&b) || ++tries>8) return;
    c=a?a:b;
    nx=a?ax:bx; ny=a?ay:by;
    if (c&4u) { if (by==ay) return;
      nx=ax+(bx-ax)*((int64_t)r->y0-ay)/(by-ay); ny=r->y0;
    } else if (c&8u) { if (by==ay) return;
      nx=ax+(bx-ax)*((int64_t)r->y1-ay)/(by-ay); ny=r->y1;
    } else if (c&1u) { if (bx==ax) return;
      ny=ay+(by-ay)*((int64_t)r->x0-ax)/(bx-ax); nx=r->x0;
    } else { if (bx==ax) return;
      ny=ay+(by-ay)*((int64_t)r->x1-ax)/(bx-ax); nx=r->x1;
    }
    if (a) { ax=nx; ay=ny; } else { bx=nx; by=ny; }
  }
  x0=(int)ax;y0=(int)ay;x1=(int)bx;y1=(int)by;
  dx=abs(x1-x0); dy=abs(y1-y0);
  sx=x0<x1?1:-1; sy=y0<y1?1:-1; err=dx-dy;
  for (;;) {
    int e;
    twodfx_raster_plot(r,x0,y0,colour);
    if (x0==x1 && y0==y1) break;
    e=err*2;
    if (e>-dy) { err-=dy; x0+=sx; }
    if (e<dx) { err+=dx; y0+=sy; }
  }
}
void twodfx_raster_fill_rect(twodfx_raster *r,int x0,int y0,int x1,int y1,uint8_t colour) {
  int y,x,t;
  if (!r) return;
  if (x0>x1){t=x0;x0=x1;x1=t;}
  if (y0>y1){t=y0;y0=y1;y1=t;}
  if (x0<r->x0)x0=r->x0;
  if (x1>r->x1)x1=r->x1;
  if (y0<r->y0)y0=r->y0;
  if (y1>r->y1)y1=r->y1;
  for (y=y0;y<=y1;y++) for (x=x0;x<=x1;x++) twodfx_raster_plot(r,x,y,colour);
}
void twodfx_raster_rect(twodfx_raster *r,int x0,int y0,int x1,int y1,uint8_t colour) {
  twodfx_raster_line(r,x0,y0,x1,y0,colour);
  twodfx_raster_line(r,x1,y0,x1,y1,colour);
  twodfx_raster_line(r,x1,y1,x0,y1,colour);
  twodfx_raster_line(r,x0,y1,x0,y0,colour);
}
void twodfx_raster_triangle(twodfx_raster *r,int x0,int y0,int x1,int y1,
                            int x2,int y2,uint8_t colour) {
  twodfx_raster_line(r,x0,y0,x1,y1,colour);
  twodfx_raster_line(r,x1,y1,x2,y2,colour);
  twodfx_raster_line(r,x2,y2,x0,y0,colour);
}
static int64_t edge(int64_t ax,int64_t ay,int64_t bx,int64_t by,int64_t px,int64_t py) {
  return (px-ax)*(by-ay)-(py-ay)*(bx-ax);
}
void twodfx_raster_fill_triangle(twodfx_raster *r,int x0,int y0,int x1,int y1,
                                 int x2,int y2,uint8_t colour) {
  int x,y,minx,maxx,miny,maxy;
  int64_t area;
  if (!r) return;
  if (x0 < -1000000 || x0 > 1000000 || y0 < -1000000 || y0 > 1000000 ||
      x1 < -1000000 || x1 > 1000000 || y1 < -1000000 || y1 > 1000000 ||
      x2 < -1000000 || x2 > 1000000 || y2 < -1000000 || y2 > 1000000) return;
  area=edge(x0,y0,x1,y1,x2,y2);
  if (!area) return; /* v1.1.0: zero-area filled triangle is empty */
  minx=x0<x1?x0:x1; if (x2<minx)minx=x2;
  maxx=x0>x1?x0:x1; if (x2>maxx)maxx=x2;
  miny=y0<y1?y0:y1; if (y2<miny)miny=y2;
  maxy=y0>y1?y0:y1; if (y2>maxy)maxy=y2;
  if(minx<r->x0)minx=r->x0;
  if(maxx>r->x1)maxx=r->x1;
  if(miny<r->y0)miny=r->y0;
  if(maxy>r->y1)maxy=r->y1;
  for(y=miny;y<=maxy;y++) for(x=minx;x<=maxx;x++) {
    int64_t e0=edge(x0,y0,x1,y1,x,y);
    int64_t e1=edge(x1,y1,x2,y2,x,y);
    int64_t e2=edge(x2,y2,x0,y0,x,y);
    if (area>0 ? (e0>=0&&e1>=0&&e2>=0) : (e0<=0&&e1<=0&&e2<=0))
      twodfx_raster_plot(r,x,y,colour);
  }
}
size_t twodfx_raster_flood(twodfx_raster *r,int x,int y,uint8_t colour) {
  size_t *queue,head=0,tail=0,count=0,p,total;
  uint8_t old_c,old_cov,new_c=(uint8_t)(colour&15u);
  if (!inside(r,x,y)) return 0;
  total=(size_t)r->width*r->height;
  p=(size_t)y*r->width+(unsigned)x;
  old_c=r->colour[p];old_cov=r->cover[p];
  if (old_c==new_c && old_cov==1) return 0;
  if (total>SIZE_MAX/sizeof(size_t))return 0;
  queue=(size_t*)malloc(total*sizeof(size_t));
  if(!queue)return 0;
  queue[tail++]=p;r->colour[p]=new_c;r->cover[p]=1;count=1;
  while (head<tail) {
    size_t q=queue[head++];
    unsigned cx=(unsigned)(q%r->width),cy=(unsigned)(q/r->width);
    size_t candidates[4];unsigned n=0,i;
    if(cx>(unsigned)r->x0)candidates[n++]=q-1;
    if(cx<(unsigned)r->x1)candidates[n++]=q+1;
    if(cy>(unsigned)r->y0)candidates[n++]=q-r->width;
    if(cy<(unsigned)r->y1)candidates[n++]=q+r->width;
    for(i=0;i<n;i++) {
      size_t v=candidates[i];
      if(r->colour[v]==old_c && r->cover[v]==old_cov) {
        r->colour[v]=new_c;r->cover[v]=1;queue[tail++]=v;count++;
      }
    }
  }
  free(queue);return count;
}
int twodfx_raster_blit4(twodfx_raster *r,int x,int y,unsigned width,unsigned height,
                        const uint8_t *src,size_t src_bytes,size_t stride,uint8_t transparent) {
  unsigned row,col;
  size_t rowbytes=(size_t)(width/2u)+(width&1u);
  if(!r || !src || !width || !height || width>4096u || height>4096u || stride<rowbytes ||
     (size_t)(height-1)>SIZE_MAX/stride ||
     (size_t)(height-1)*stride>src_bytes ||
     rowbytes>src_bytes-(size_t)(height-1)*stride)return 0;
  for(row=0;row<height;row++) {
    int64_t dy=(int64_t)y+row;
    if (dy<r->y0 || dy>r->y1) continue;
    for(col=0;col<width;col++) {
      int64_t dx=(int64_t)x+col;
      uint8_t b,c;
      if (dx<r->x0 || dx>r->x1) continue;
      b=src[(size_t)row*stride+col/2u];
      c=(uint8_t)((col&1u)?(b>>4):(b&15u));
      if(c!=(transparent&15u)) twodfx_raster_plot(r,(int)dx,(int)dy,c);
    }
  }
  return 1;
}
