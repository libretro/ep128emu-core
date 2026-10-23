/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Károly Vaczkó
 */
#ifndef TWODFX_EMU_H
#define TWODFX_EMU_H
/* Independently implemented portable 2dfx host API (C99).
 * Copyright (c) 2026 Károly Vaczkó; MIT License.
 * The public v1.1.0 egg C source is supplied; private MCU fast paths are not.
 */
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define TWODFX_EMU_ABI_VERSION 1u
#define TWODFX_EMU_RESOURCE_BYTES 8388608u
#define TWODFX_EMU_SPRITE_COUNT 64u
#define TWODFX_EMU_2DPT_SLOTS 256u
#define TWODFX_EMU_VIDEO_W_MAX 744u
#define TWODFX_EMU_VIDEO_H_MAX 288u
#define TWODFX_EMU_TVC_VIDEO_W 704u
#define TWODFX_EMU_TVC_VIDEO_H 256u
/* Public built-in logo artwork, 206x79, four 2-bit pixels per byte,
 * first pixel in bits 1:0, 0=transparent, 1=main, 2=accent, 3=invalid.
 * The public v1.1.0 firmware artwork is EMBEDDED and loaded on create(). */
#define TWODFX_EMU_LOGO_W 206u
#define TWODFX_EMU_LOGO_H 79u
#define TWODFX_EMU_LOGO_STRIDE 52u
#define TWODFX_EMU_LOGO_BYTES (TWODFX_EMU_LOGO_H * TWODFX_EMU_LOGO_STRIDE)

typedef struct twodfx_emu twodfx_emu;
typedef enum {
  TWODFX_EMU_ENTERPRISE = 0,
  TWODFX_EMU_TVC = 1
} twodfx_machine;
typedef enum {
  TWODFX_EMU_ENGINE_OFF = 0,
  TWODFX_EMU_ENGINE_NORMAL = 1,
  TWODFX_EMU_ENGINE_FORCED = 2
} twodfx_engine_mode;

typedef struct {
  uint8_t opcode;            /* normalized opcode: 60h..7Fh */
  uint8_t nbytes;            /* number of payload bytes after slot */
  uint8_t payload[11];      /* byte-exact as received */
} twodfx_2dpt_entry;

/* Additive to ABI v1: host connector functions above are unchanged. */
typedef enum {
  TWODFX_RESOURCE_JOB_NONE = 0,
  TWODFX_RESOURCE_JOB_BUSY = 1,
  TWODFX_RESOURCE_JOB_SUCCESS = 2,
  TWODFX_RESOURCE_JOB_ERROR = 3
} twodfx_resource_job_status;
typedef struct {
  uint32_t resource_address;
  uint8_t width_units, height_units, valid;
} twodfx_font_descriptor;
typedef struct {
  uint32_t resource_address;
  uint16_t width, height;
  uint8_t valid;
} twodfx_bitmap_descriptor;
typedef struct {
  uint32_t resource_address;
  uint8_t valid;
} twodfx_address_descriptor;
typedef struct {
  uint32_t resource_address;
  uint16_t width_cells, height_cells;
  uint8_t tileset, valid;
} twodfx_tilemap_descriptor;

/* Optional built-in-logo artwork override, *not* a host framebuffer hook.
 * Copies validated 2bpp public-format pixels into this device instance;
 * survives SYS_RESET and physicalPowerCycle just like firmware ROM artwork.
 * Invalid length / reserved colour=3 is rejected atomically.
 * The public artwork ships with the module. clear_logo_art() restores the
 * immutable built-in artwork, discarding any host-supplied test override. */
int twodfx_emu_set_logo_art(twodfx_emu *d, const void *packed, size_t nbytes);
void twodfx_emu_clear_logo_art(twodfx_emu *d);
int twodfx_emu_logo_art_loaded(const twodfx_emu *d);

/* Easter-egg lifecycle. An unimplemented selection is not simulated as running.
 * Supported egg IDs are checked against the machine profile before start.
 * Input FIFO has 63 usable bytes; overflow discards newest bytes.
 * An interactive egg consumes idle F9h data at its accepted frame boundary;
 * host diagnostics may explicitly dequeue data when the egg has not done so.
 */
int twodfx_emu_egg_active(const twodfx_emu *d);
unsigned twodfx_emu_egg_id(const twodfx_emu *d);  /* 0..31 or 255 */
int twodfx_emu_egg_start_pending(const twodfx_emu *d);
int twodfx_emu_egg_pop_input(twodfx_emu *d, uint8_t *out);
int twodfx_emu_egg_input_overflow(const twodfx_emu *d);
void twodfx_emu_egg_clear_input_overflow(twodfx_emu *d);

/* 3D state inspection for module tests and host diagnostics. */
typedef struct {
  uint32_t resource_address;
  uint16_t vertices, triangles;
  uint8_t valid;
} twodfx_3d_mesh_descriptor;
typedef struct {
  int16_t x,y,z;
  uint16_t roll,yaw,pitch;
} twodfx_3d_camera;
typedef struct {
  uint16_t fx,fy,near_z,far_z;
} twodfx_3d_projection_info;
int twodfx_emu_get_mesh(const twodfx_emu *d,unsigned id,int active,
                        twodfx_3d_mesh_descriptor *out);
int twodfx_emu_get_camera(const twodfx_emu *d,int active,twodfx_3d_camera *out);
int twodfx_emu_get_projection(const twodfx_emu *d,int active,
                              twodfx_3d_projection_info *out);

/* Collision STATUS7 is active-low and delayed by one VSYNC: completed-frame
 * result is reported at the next VSYNC, including when 25fps frames skip.
 * Video rendering is enabled by default. */

/* Resource format conversion: 0=RAW, 1=NICK, 2=TVC; 3+ passes through. */
uint8_t twodfx_emu_convert_byte(uint8_t value, unsigned format);

/* Completed resource jobs remain SUCCESS/ERROR until another accepted job.
   UNZX1 runs on next vsync without modelling host CPU/MCU render duration. */
twodfx_resource_job_status twodfx_emu_resource_job_status(const twodfx_emu *d);
uint32_t twodfx_emu_resource_job_output_bytes(const twodfx_emu *d);

/* Descriptor introspection, internal state only: not RRAM bypass functions. */
int twodfx_emu_get_font(const twodfx_emu *d, unsigned id, twodfx_font_descriptor *out);
int twodfx_emu_get_bitmap(const twodfx_emu *d, unsigned id, twodfx_bitmap_descriptor *out);
int twodfx_emu_get_pattern(const twodfx_emu *d, unsigned id, twodfx_address_descriptor *out);
int twodfx_emu_get_tileset(const twodfx_emu *d, unsigned id, twodfx_address_descriptor *out);
int twodfx_emu_get_tilemap(const twodfx_emu *d, unsigned id, twodfx_tilemap_descriptor *out);

/* Allocates isolated per-instance memory, including the 8 MiB resource RAM. */
twodfx_emu *twodfx_emu_create(twodfx_machine machine);
void twodfx_emu_destroy(twodfx_emu *d);

/* Physical power cycle / reset: returns to cold F8h=50h wake gate. */
void twodfx_emu_power_cycle(twodfx_emu *d);

/* Host bus adapter: route writes on F8h and F9h; F8h reads only. */
void twodfx_emu_write_f8(twodfx_emu *d, uint8_t value);
void twodfx_emu_write_f9(twodfx_emu *d, uint8_t value);
uint8_t twodfx_emu_read_f8(const twodfx_emu *d);

/*
 * Host video/clock adapter: call VSYNC once per 50Hz video frame and HSYNC
 * once per host scanline, with the hardware-profile's raw scanline index.
 * 25fps mode only halves scene-update acceptance, not these 50Hz callbacks.
 * IRQ is level-triggered by pending raster latches; wire to host /INT logic.
 */
void twodfx_emu_vsync(twodfx_emu *d);
void twodfx_emu_hsync(twodfx_emu *d, uint16_t raw_scanline);
int twodfx_emu_irq_asserted(const twodfx_emu *d);
uint8_t twodfx_emu_raster_pending(const twodfx_emu *d);

/*
 * Video connector:
 * src_x,src_y are stored-framebuffer-local pixel coordinates, NOT guest
 * raw scanout coordinates: Enterprise local (0,0) = raw (88,16); TVC
 * local (0,0) = raw (0,41). Host owns final visible/palette mapping.
 * Populates colour indices
 * 0..15 and coverage[0..width-1] (0=host, 1=2dfx).
 * Host applies Enterprise NICK or TVC colour selection AFTER compositing.
 */
unsigned twodfx_emu_framebuffer_width(const twodfx_emu *d);
unsigned twodfx_emu_framebuffer_height(const twodfx_emu *d);
size_t twodfx_emu_overlay_line(const twodfx_emu *d,
                               unsigned src_x, unsigned src_y,
                               uint8_t *colours, uint8_t *coverage,
                               size_t width);

/* Device snapshot format v13. See docs/SNAPSHOT_FORMAT.md for the canonical
 * complete device-chunk contract. Atomic CRC/invariant checks reject invalid
 * blobs without changing the running device; older versions are rejected.
 */
/* Rendering defaults ON; this optional switch is for host diagnostics. */
void twodfx_emu_set_render_enabled(twodfx_emu *d, int enabled);
int twodfx_emu_render_enabled(const twodfx_emu *d);

size_t twodfx_emu_snapshot_size(void);
int twodfx_emu_save_snapshot(const twodfx_emu *d, void *output, size_t capacity);
int twodfx_emu_load_snapshot(twodfx_emu *d, const void *input, size_t length);

/* Introspection primarily for bridge tests. Not an RRAM write bypass API. */
/* Raw scanout-coordinate helper. Converts the caller's physical raw NICK/TVC
 * X/Y into the device's stored framebuffer, with safe left-edge clipping;
 * returns the number of 2dfx-selected pixels. Out-of-frame = uncovered.
 * NORMAL selects non-key pixels; FORCED selects all in-frame pixels,
 * including colour equal to transparent key. OFF selects none. */
size_t twodfx_emu_overlay_raw_line(const twodfx_emu *d,
                                   unsigned raw_x, unsigned raw_y,
                                   uint8_t *colours, uint8_t *coverage,
                                   size_t width);

twodfx_engine_mode twodfx_emu_engine_mode(const twodfx_emu *d);
twodfx_machine twodfx_emu_machine(const twodfx_emu *d);
uint8_t twodfx_emu_transparent_colour(const twodfx_emu *d);
uint8_t twodfx_emu_fps(const twodfx_emu *d);  /* 50 or 25 */
uint8_t twodfx_emu_resource_byte(const twodfx_emu *d, uint32_t addr);
uint8_t twodfx_emu_sprite_shadow_byte(const twodfx_emu *d, unsigned sprite, unsigned offset);
uint8_t twodfx_emu_sprite_active_byte(const twodfx_emu *d, unsigned sprite, unsigned offset);
int twodfx_emu_get_2dpt(const twodfx_emu *d, unsigned foreground,
                        unsigned slot, int active, twodfx_2dpt_entry *entry);
uint64_t twodfx_emu_video_frame_count(const twodfx_emu *d);
/* Current F8h collision readback as a boolean; snapshot-stable. */
int twodfx_emu_collision_detected(const twodfx_emu *d);

#ifdef __cplusplus
}
#endif
#endif
