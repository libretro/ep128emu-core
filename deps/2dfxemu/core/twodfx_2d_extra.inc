/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Károly Vaczkó
 */
/* 2dfx: independently authored portable reference raster operations.
 * Wire format and asset geometry follow the published v1.1.0 API contract.
 * These are straightforward reference algorithms, never MCU fast paths.
 * Edge pixels of curves remain subject to golden-hardware verification.
 */
#include <math.h>

static uint8_t raster_ext_packed(const twodfx_emu *d, uint32_t a, unsigned x) {
  uint8_t v = d->ram[(a + x / 2u) & (TWODFX_EMU_RESOURCE_BYTES - 1u)];
  return (uint8_t)((x & 1u) ? (v >> 4) : (v & 15u));
}
static uint8_t raster_ext_pattern_pixel(const twodfx_emu *d, unsigned bank, unsigned id,
                                int x, int y) {
  const twodfx_address_descriptor *desc = &d->active_patterns[bank & 63u];
  uint32_t a = desc->resource_address + (uint32_t)id * 64u +
               (uint32_t)(y & 7) * 8u;
  return raster_ext_packed(d, a, (unsigned)(x & 15));
}
static void raster_ext_text(twodfx_emu *d, twodfx_raster *r, int x, int y,
                     unsigned id, uint32_t str, uint8_t fg, uint8_t bg) {
  const twodfx_font_descriptor *f = &d->active_fonts[id & 63u];
  unsigned w, h, row_bytes, stride, ci;
  if (!f->valid || !f->width_units || !f->height_units) return;
  w = (unsigned)f->width_units * 8u;
  h = (unsigned)f->height_units * 8u;
  row_bytes = (unsigned)f->width_units;
  stride = row_bytes * h;
  for (ci = 0; ci < 255u; ++ci) {
    uint8_t ch = d->ram[(str + ci) & (TWODFX_EMU_RESOURCE_BYTES-1u)];
    int cx = x + (int)(ci * w);
    unsigned ry, bx;
    if (ch >= 128u) break;
    if (cx > r->x1 && w) break;
    if (bg != d->transparent)
      twodfx_raster_fill_rect(r, cx, y, cx+(int)w-1, y+(int)h-1, bg);
    for (ry = 0; ry < h; ++ry) {
      int py = y + (int)ry;
      if (py < r->y0 || py > r->y1) continue;
      for (bx = 0; bx < row_bytes; ++bx) {
        uint32_t adr = (f->resource_address + (uint32_t)ch*stride +
                        ry*row_bytes + bx) & (TWODFX_EMU_RESOURCE_BYTES-1u);
        uint8_t bits = d->ram[adr];
        unsigned bit;
        if (!bits) continue;
        for (bit = 0; bit < 8u; ++bit) {
          if (bits & (0x80u >> bit))
            twodfx_raster_plot(r,cx+(int)(bx*8u+bit),py,fg);
        }
      }
    }
  }
}
static void raster_ext_pattern_fill(twodfx_emu *d,twodfx_raster *r,
                             int rawx,int rawy,unsigned w,unsigned h,
                             unsigned bank,unsigned id,int origin_x,int origin_y) {
  int x,y,x0,x1,y0,y1;
  if (!d->active_patterns[bank & 63u].valid || !w || !h) return;
  /* 2dfx protocol: effective RAW X and width are rounded down to pairs. */
  rawx &= ~1; w &= ~1u;
  if (!w) return;
  x0=rawx-origin_x; y0=rawy-origin_y;
  x1=x0+(int)w-1; y1=y0+(int)h-1;
  if (x0<r->x0)x0=r->x0;
  if (y0<r->y0)y0=r->y0;
  if (x1>r->x1)x1=r->x1;
  if (y1>r->y1)y1=r->y1;
  for(y=y0;y<=y1;++y) for(x=x0;x<=x1;++x)
    twodfx_raster_plot(r,x,y,raster_ext_pattern_pixel(d,bank,id,
                      x+origin_x-rawx,y+origin_y-rawy));
}
static void raster_ext_tilemap(twodfx_emu *d,twodfx_raster *r,int rawx,int rawy,
                        unsigned map_id,unsigned srcx,unsigned srcy,
                        unsigned cols,unsigned rows,int origin_x,int origin_y) {
  const twodfx_tilemap_descriptor *m=&d->active_tilemaps[map_id & 31u];
  const twodfx_address_descriptor *t;
  unsigned cx,cy;
  int dx,dy;
  if (!m->valid || !cols || !rows || srcx>=m->width_cells || srcy>=m->height_cells) return;
  t=&d->active_tilesets[m->tileset & 63u];
  if(!t->valid) return;
  if(cols>m->width_cells-srcx)cols=m->width_cells-srcx;
  if(rows>m->height_cells-srcy)rows=m->height_cells-srcy;
  rawx &= ~1;
  dx=rawx-origin_x; dy=rawy-origin_y;
  for(cy=0;cy<rows;cy++) {
    int ybase=dy+(int)(cy*8u);
    if (ybase > r->y1) break;
    if (ybase+7 < r->y0) continue;
    for(cx=0;cx<cols;cx++) {
      int xbase=dx+(int)(cx*16u), x,y;
      uint32_t mapaddr,tileaddr;
      unsigned tid;
      if (xbase>r->x1) break;
      if (xbase+15<r->x0) continue;
      mapaddr=(m->resource_address+(srcy+cy)*m->width_cells+(srcx+cx)) &
              (TWODFX_EMU_RESOURCE_BYTES-1u);
      tid=d->ram[mapaddr];
      tileaddr=t->resource_address+tid*64u;
      for(y=ybase;y<ybase+8;y++) {
        if(y<r->y0||y>r->y1)continue;
        for(x=xbase;x<xbase+16;x++) {
          uint8_t pixel;
          if(x<r->x0||x>r->x1)continue;
          pixel=raster_ext_packed(d,tileaddr+(unsigned)(y-ybase)*8u,(unsigned)(x-xbase));
          twodfx_raster_plot(r,x,y,pixel);
        }
      }
    }
  }
}
static void raster_ext_pattern_bucket(twodfx_emu *d,twodfx_raster *r,
                               int sx,int sy,unsigned bank,unsigned pat,
                               int origin_x,int origin_y) {
  size_t total,pos,head=0,tail=0,*queue;
  uint8_t *seen, target;
  if (!d->active_patterns[bank & 63u].valid || sx<r->x0 || sx>r->x1 ||
      sy<r->y0 || sy>r->y1)return;
  total=(size_t)r->width*r->height;
  if(total>SIZE_MAX/sizeof(size_t))return;
  queue=(size_t*)malloc(total*sizeof(size_t));
  seen=(uint8_t*)calloc(total,1u);
  if(!queue||!seen){free(queue);free(seen);return;}
  pos=(size_t)sy*r->width+(unsigned)sx;
  target=r->colour[pos];
  queue[tail++]=pos;seen[pos]=1u;
  while(head<tail) {
    size_t cur=queue[head++];
    int x=(int)(cur%r->width), y=(int)(cur/r->width), n;
    size_t neighbours[4];unsigned count=0;
    /* Colour is compared before painting a neighbour, preserving traversal
       when the pattern itself produces the source colour. */
    twodfx_raster_plot(r,x,y,raster_ext_pattern_pixel(d,bank,pat,x+origin_x,y+origin_y));
    if(x>r->x0)neighbours[count++]=cur-1u;
    if(x<r->x1)neighbours[count++]=cur+1u;
    if(y>r->y0)neighbours[count++]=cur-r->width;
    if(y<r->y1)neighbours[count++]=cur+r->width;
    for(n=0;n<(int)count;++n){
      size_t at=neighbours[n];
      if(!seen[at]){
        seen[at]=1u;
        if(r->colour[at]==target)queue[tail++]=at;
      }
    }
  }
  free(queue);free(seen);
}
/* Curve raster is a compact mathematical reference, not an RP2354 pixel
   algorithm. Shape interiors are sampled at pixel centers. */
static int raster_ext_inside_ellipse(int x,int y,int cx,int cy,int rx,int ry) {
  double fx,fy;
  if(rx<=0||ry<=0)return (rx==0 && x==cx && abs(y-cy)<=ry) ||
                            (ry==0 && y==cy && abs(x-cx)<=rx);
  fx=((double)x-cx)/rx;fy=((double)y-cy)/ry;
  return fx*fx+fy*fy<=1.0;
}
static int raster_ext_inside_round(int x,int y,int left,int top,int w,int h,int rx,int ry) {
  int right=left+w-1,bottom=top+h-1,cx,cy;
  if(x<left||y<top||x>right||y>bottom)return 0;
  if(!rx||!ry)return 1;
  cx=x<left+rx ? left+rx : (x>right-rx ? right-rx : x);
  cy=y<top+ry ? top+ry : (y>bottom-ry ? bottom-ry : y);
  return raster_ext_inside_ellipse(x,y,cx,cy,rx,ry);
}
static void raster_ext_round(twodfx_raster *r,int x,int y,unsigned w,unsigned h,
                      unsigned rx,unsigned ry,unsigned stroke,uint8_t fg,
                      uint8_t bg,uint8_t key,int filled) {
  int xx,yy,x0=x,y0=y,x1,y1,ix,iy,iw,ih,irx,iry;
  if(!w||!h)return;
  if(rx>w/2u)rx=w/2u;
  if(ry>h/2u)ry=h/2u;
  x1=x+(int)w-1;y1=y+(int)h-1;
  if(x0<r->x0)x0=r->x0;
  if(y0<r->y0)y0=r->y0;
  if(x1>r->x1)x1=r->x1;
  if(y1>r->y1)y1=r->y1;
  ix=x+(int)(2u*stroke);iy=y+(int)stroke;
  iw=(int)w-(int)(4u*stroke);ih=(int)h-(int)(2u*stroke);
  irx=(int)rx-(int)(2u*stroke);iry=(int)ry-(int)stroke;
  if(irx<0)irx=0;
  if(iry<0)iry=0;
  for(yy=y0;yy<=y1;++yy)for(xx=x0;xx<=x1;++xx) {
    int outer=raster_ext_inside_round(xx,yy,x,y,(int)w,(int)h,(int)rx,(int)ry);
    if(!outer)continue;
    if(filled || iw<=0 || ih<=0 ||
       !raster_ext_inside_round(xx,yy,ix,iy,iw,ih,irx,iry))
      twodfx_raster_plot(r,xx,yy,fg);
    else if(bg!=key)twodfx_raster_plot(r,xx,yy,bg);
  }
}
static void raster_ext_ellipse(twodfx_raster *r,int cx,int cy,unsigned rx,unsigned ry,
                        unsigned stroke,uint8_t fg,uint8_t bg,uint8_t key,int filled) {
  int x,y,ix,iy,x0=cx-(int)rx,y0=cy-(int)ry,x1=cx+(int)rx,y1=cy+(int)ry;
  if(x0<r->x0)x0=r->x0;
  if(y0<r->y0)y0=r->y0;
  if(x1>r->x1)x1=r->x1;
  if(y1>r->y1)y1=r->y1;
  for(y=y0;y<=y1;y++)for(x=x0;x<=x1;x++){
    if(!raster_ext_inside_ellipse(x,y,cx,cy,(int)rx,(int)ry))continue;
    ix=(int)rx-(int)(2u*stroke);iy=(int)ry-(int)stroke;
    if(filled||ix<=0||iy<=0||!raster_ext_inside_ellipse(x,y,cx,cy,ix,iy))
      twodfx_raster_plot(r,x,y,fg);
    else if(bg!=key)twodfx_raster_plot(r,x,y,bg);
  }
}
static double raster_ext_angle(int x,int y,int cx,int cy) {
  double a=atan2((double)x-cx,(double)cy-y);
  if(a<0.0)a+=6.2831853071795864769;
  return a*256.0/6.2831853071795864769;
}
static int raster_ext_sweep(double a,unsigned start,unsigned end) {
  if(start==end)return 0;
  if(end>start)return a>=start && a<=end;
  return a>=start || a<=end;
}
static void raster_ext_arc_pie(twodfx_raster *r,int cx,int cy,unsigned rx,unsigned ry,
                        unsigned start,unsigned end,unsigned stroke,uint8_t fg,
                        int pie) {
  int x,y,x0=cx-(int)rx,y0=cy-(int)ry,x1=cx+(int)rx,y1=cy+(int)ry;
  if(start==end)return;
  if(x0<r->x0)x0=r->x0;
  if(y0<r->y0)y0=r->y0;
  if(x1>r->x1)x1=r->x1;
  if(y1>r->y1)y1=r->y1;
  for(y=y0;y<=y1;y++)for(x=x0;x<=x1;x++){
    int inside=raster_ext_inside_ellipse(x,y,cx,cy,(int)rx,(int)ry);
    int irx=(int)rx-(int)(2u*stroke),iry=(int)ry-(int)stroke;
    if(!inside || !raster_ext_sweep(raster_ext_angle(x,y,cx,cy),start,end))continue;
    if(pie || irx<=0 || iry<=0 ||
       !raster_ext_inside_ellipse(x,y,cx,cy,irx,iry))twodfx_raster_plot(r,x,y,fg);
  }
  if(pie) {
    int ex0=cx+(int)lround((double)rx*sin(start*6.2831853071795864769/256.0));
    int ey0=cy-(int)lround((double)ry*cos(start*6.2831853071795864769/256.0));
    int ex1=cx+(int)lround((double)rx*sin(end*6.2831853071795864769/256.0));
    int ey1=cy-(int)lround((double)ry*cos(end*6.2831853071795864769/256.0));
    twodfx_raster_line(r,cx,cy,ex0,ey0,fg);
    twodfx_raster_line(r,cx,cy,ex1,ey1,fg);
  }
}
