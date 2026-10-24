/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Károly Vaczkó
 */
#ifndef TWODFX_3D_H
#define TWODFX_3D_H
/* Standalone, independently written C99 reference geometry module.
 * The published v1.1.0 resource/port format is the behavioral contract.
 * Deliberately uses standard floating-point trigonometry, NOT MCU tables,
 * fixed point shortcuts, render-duration, or other firmware internals. */
#include <stddef.h>
#include <stdint.h>
#include "twodfx_raster.h"
#ifdef __cplusplus
extern "C" {
#endif
#define TWODFX_3D_MESH_IDS 256u
#define TWODFX_3D_MAX_VERTICES 256u
#define TWODFX_3D_MAX_FACES 512u
typedef struct {
  uint32_t address;
  uint16_t vertices, faces;
} twodfx_3d_mesh;
typedef struct {
  int16_t x, y, z;
  uint16_t roll, yaw, pitch;
} twodfx_3d_pose;
typedef struct {
  uint16_t fx, fy, near_z, far_z;
} twodfx_3d_projection;
typedef struct {
  twodfx_3d_mesh mesh_shadow[256], mesh_active[256];
  twodfx_3d_pose camera_shadow, camera_active;
  twodfx_3d_projection projection_shadow, projection_active;
} twodfx_3d_scene;
void twodfx_3d_reset(twodfx_3d_scene *s);
void twodfx_3d_latch(twodfx_3d_scene *s);
void twodfx_3d_define(twodfx_3d_scene *s, unsigned id, uint32_t address,
                       unsigned vertices, unsigned faces);
void twodfx_3d_scene_set_projection(twodfx_3d_scene *s, uint16_t fx,
                              uint16_t fy, uint16_t near_z, uint16_t far_z);
void twodfx_3d_scene_set_camera(twodfx_3d_scene *s, const uint8_t params[10]);
void twodfx_3d_unpack_angles(const uint8_t wire[4],
                            uint16_t *roll, uint16_t *yaw, uint16_t *pitch);
/* DRAW_MESH entries contain 11 bytes after the 2DPT slot: mesh ID,
 * signed xyz, and little endian packed 10-bit roll/yaw/pitch.
 * mode: 0 wireframe, 1 filled, 2 filled with edges.
 * raw origin is the stored framebuffer offset, not optical centre. */
void twodfx_3d_render(const twodfx_3d_scene *s, const uint8_t *ram,
                      twodfx_raster *r, const uint8_t entry[11],
                      unsigned mode, uint8_t edge_colour,
                      int xy_offset_x, int xy_offset_y,
                      int origin_x, int origin_y, int optical_x, int optical_y);
#ifdef __cplusplus
}
#endif
#endif
