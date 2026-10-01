#ifndef TWODFX_API_H
#define TWODFX_API_H

/*
 * 2dfx firmware-extension C API
 *
 * This is the supported source-level interface for firmware-resident eggs and
 * third-party firmware extensions. It exposes typed operations over the same
 * renderer state used by the F8h/F9h host protocol without exposing protocol
 * opcodes, GPIO/PIO/DMA details, raw SPB/2DPB layouts or renderer-cache
 * internals.
 *
 * Calling model
 * -------------
 * - Call these functions from core0 egg start/tick code unless a function
 *   explicitly states that interrupt use is supported.
 * - Sprite, descriptor and 2DPT calls update shadow scene state. The normal
 *   frame-synchronous publication path presents that state to core1.
 * - Resource-RAM writes are immediate. Do not overwrite a resource that an
 *   in-progress render may still be reading.
 * - Host F8h/F9h commands and this C API operate on the same shadow scene.
 *   The later write wins. State rewritten by an egg on every tick will replace
 *   a host-side interactive change on the next tick.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------- */
/* Boot-selected machine and practical geometry                              */
/* ------------------------------------------------------------------------- */

/** Machine profile selected from the GPIO20 strap during firmware boot. */
typedef enum {
    TWODFX_MACHINE_ENTERPRISE = 0,
    TWODFX_MACHINE_TVC        = 1
} twodfx_machine_t;

/**
 * Returns the machine profile selected when 2dfx started.
 *
 * @return
 *     TWODFX_MACHINE_ENTERPRISE or TWODFX_MACHINE_TVC.
 *
 * An egg may use the result to choose machine-appropriate colours, resources,
 * layout or presentation behavior. The selected profile cannot change until
 * the RP2354 is reset.
 */
twodfx_machine_t twodfx_get_machine(void);

/*
 * TVC practical display areas measured and verified on real hardware.
 * Minimum and maximum coordinates are inclusive.
 */
#define TWODFX_TVC_CRTC_VISIBLE_MIN_X  79u
#define TWODFX_TVC_CRTC_VISIBLE_MAX_X 686u
#define TWODFX_TVC_CRTC_VISIBLE_MIN_Y  41u
#define TWODFX_TVC_CRTC_VISIBLE_MAX_Y 296u

#define TWODFX_TVC_BASIC_SAFE_MIN_X    126u
#define TWODFX_TVC_BASIC_SAFE_MAX_X    637u
#define TWODFX_TVC_BASIC_SAFE_MIN_Y     49u
#define TWODFX_TVC_BASIC_SAFE_MAX_Y    288u

/*
 * Enterprise practical display areas measured and verified on real hardware.
 * Minimum and maximum coordinates are inclusive.
 */
#define TWODFX_ENTERPRISE_NICK_VISIBLE_MIN_X  88u
#define TWODFX_ENTERPRISE_NICK_VISIBLE_MAX_X 811u
#define TWODFX_ENTERPRISE_NICK_VISIBLE_MIN_Y  18u
#define TWODFX_ENTERPRISE_NICK_VISIBLE_MAX_Y 305u

#define TWODFX_ENTERPRISE_BASIC_SAFE_MIN_X   124u
#define TWODFX_ENTERPRISE_BASIC_SAFE_MAX_X   763u
#define TWODFX_ENTERPRISE_BASIC_SAFE_MIN_Y    38u
#define TWODFX_ENTERPRISE_BASIC_SAFE_MAX_Y   262u

/* Retained for source compatibility with pre-measurement API clients. */
#define TWODFX_GEOMETRY_UNMEASURED UINT16_MAX

/*
 * Default EXOS/IS-BASIC TEXT-MODE palette entries.
 *
 * These values are intentionally labelled as text-mode defaults: Enterprise
 * graphics modes may program the lower palette entries differently.  Entries
 * 8..15 are not named here because their displayed RGB values are selected by
 * the active NICK BIAS value.
 */
#define TWODFX_ENTERPRISE_COLOUR_BLACK      0u
#define TWODFX_ENTERPRISE_COLOUR_GREEN      1u
#define TWODFX_ENTERPRISE_COLOUR_BLACK_ALT  2u
#define TWODFX_ENTERPRISE_COLOUR_RED        3u
#define TWODFX_ENTERPRISE_COLOUR_WHITE      4u
#define TWODFX_ENTERPRISE_COLOUR_BLUE       5u
#define TWODFX_ENTERPRISE_COLOUR_MAGENTA    6u
#define TWODFX_ENTERPRISE_COLOUR_CYAN       7u

/* Active maximum-visible area for the boot-selected machine. */
#define TWODFX_MAX_VISIBLE_MIN_X \
    ((twodfx_get_machine() == TWODFX_MACHINE_TVC) \
        ? TWODFX_TVC_CRTC_VISIBLE_MIN_X \
        : TWODFX_ENTERPRISE_NICK_VISIBLE_MIN_X)
#define TWODFX_MAX_VISIBLE_MAX_X \
    ((twodfx_get_machine() == TWODFX_MACHINE_TVC) \
        ? TWODFX_TVC_CRTC_VISIBLE_MAX_X \
        : TWODFX_ENTERPRISE_NICK_VISIBLE_MAX_X)
#define TWODFX_MAX_VISIBLE_MIN_Y \
    ((twodfx_get_machine() == TWODFX_MACHINE_TVC) \
        ? TWODFX_TVC_CRTC_VISIBLE_MIN_Y \
        : TWODFX_ENTERPRISE_NICK_VISIBLE_MIN_Y)
#define TWODFX_MAX_VISIBLE_MAX_Y \
    ((twodfx_get_machine() == TWODFX_MACHINE_TVC) \
        ? TWODFX_TVC_CRTC_VISIBLE_MAX_Y \
        : TWODFX_ENTERPRISE_NICK_VISIBLE_MAX_Y)

/* Active IS-BASIC/VT-BASIC-safe area for the boot-selected machine. */
#define TWODFX_BASIC_SAFE_MIN_X \
    ((twodfx_get_machine() == TWODFX_MACHINE_TVC) \
        ? TWODFX_TVC_BASIC_SAFE_MIN_X \
        : TWODFX_ENTERPRISE_BASIC_SAFE_MIN_X)
#define TWODFX_BASIC_SAFE_MAX_X \
    ((twodfx_get_machine() == TWODFX_MACHINE_TVC) \
        ? TWODFX_TVC_BASIC_SAFE_MAX_X \
        : TWODFX_ENTERPRISE_BASIC_SAFE_MAX_X)
#define TWODFX_BASIC_SAFE_MIN_Y \
    ((twodfx_get_machine() == TWODFX_MACHINE_TVC) \
        ? TWODFX_TVC_BASIC_SAFE_MIN_Y \
        : TWODFX_ENTERPRISE_BASIC_SAFE_MIN_Y)
#define TWODFX_BASIC_SAFE_MAX_Y \
    ((twodfx_get_machine() == TWODFX_MACHINE_TVC) \
        ? TWODFX_TVC_BASIC_SAFE_MAX_Y \
        : TWODFX_ENTERPRISE_BASIC_SAFE_MAX_Y)

/* Active inclusive-range dimensions for the boot-selected machine. */
#define TWODFX_MAX_VISIBLE_WIDTH \
    (((TWODFX_MAX_VISIBLE_MIN_X == TWODFX_GEOMETRY_UNMEASURED) || \
      (TWODFX_MAX_VISIBLE_MAX_X == TWODFX_GEOMETRY_UNMEASURED)) \
        ? 0u \
        : (uint16_t)(TWODFX_MAX_VISIBLE_MAX_X - \
                     TWODFX_MAX_VISIBLE_MIN_X + 1u))
#define TWODFX_MAX_VISIBLE_HEIGHT \
    (((TWODFX_MAX_VISIBLE_MIN_Y == TWODFX_GEOMETRY_UNMEASURED) || \
      (TWODFX_MAX_VISIBLE_MAX_Y == TWODFX_GEOMETRY_UNMEASURED)) \
        ? 0u \
        : (uint16_t)(TWODFX_MAX_VISIBLE_MAX_Y - \
                     TWODFX_MAX_VISIBLE_MIN_Y + 1u))
#define TWODFX_BASIC_SAFE_WIDTH \
    (((TWODFX_BASIC_SAFE_MIN_X == TWODFX_GEOMETRY_UNMEASURED) || \
      (TWODFX_BASIC_SAFE_MAX_X == TWODFX_GEOMETRY_UNMEASURED)) \
        ? 0u \
        : (uint16_t)(TWODFX_BASIC_SAFE_MAX_X - \
                     TWODFX_BASIC_SAFE_MIN_X + 1u))
#define TWODFX_BASIC_SAFE_HEIGHT \
    (((TWODFX_BASIC_SAFE_MIN_Y == TWODFX_GEOMETRY_UNMEASURED) || \
      (TWODFX_BASIC_SAFE_MAX_Y == TWODFX_GEOMETRY_UNMEASURED)) \
        ? 0u \
        : (uint16_t)(TWODFX_BASIC_SAFE_MAX_Y - \
                     TWODFX_BASIC_SAFE_MIN_Y + 1u))

/* ------------------------------------------------------------------------- */
/* Global controls and selective reset helpers                               */
/* ------------------------------------------------------------------------- */

/** Selects how 2dfx controls the external-colour enable signal. */
typedef enum {
    /** Disable rendering, force /EXTC inactive and release EC0..EC3. */
    TWODFX_ENGINE_OFF = 0,

    /** Enable normal comparator-controlled external-colour operation. */
    TWODFX_ENGINE_NORMAL,

    /** Enable rendering and force /EXTC active for the complete scanout. */
    TWODFX_ENGINE_FORCED_ON
} twodfx_engine_mode_t;

/**
 * Changes the 2dfx engine mode.
 *
 * @param mode
 *     TWODFX_ENGINE_OFF, TWODFX_ENGINE_NORMAL or
 *     TWODFX_ENGINE_FORCED_ON.
 *
 * OFF leaves the computer's normal display visible, electrically releases
 * EC0..EC3 and prevents new render jobs. Scanout PIO/DMA remain running
 * internally so a later enable is immediate. A render already running is
 * allowed to finish. Because normal egg
 * ticks follow accepted scene-update opportunities, an egg that turns the
 * engine off cannot rely on receiving another tick to turn it back on; another
 * code path or a host F8h/F9h command must re-enable it.
 */
void twodfx_set_engine_mode(twodfx_engine_mode_t mode);

/** Scene-update cadence; physical video scanout remains 50 Hz. */
typedef enum {
    TWODFX_FPS_50 = 0,
    TWODFX_FPS_25 = 1
} twodfx_fps_t;

/**
 * Selects the scene-update cadence used by the renderer and active egg.
 *
 * @param fps
 *     TWODFX_FPS_50 or TWODFX_FPS_25.
 *
 * In 25 fps mode a new scene is rendered on every second video frame. Changing
 * the mode resets the cadence phase, so the next VSYNC is an update
 * opportunity rather than an automatically skipped frame.
 */
void twodfx_set_fps(twodfx_fps_t fps);

/**
 * Selects the current transparent colour index.
 *
 * @param color_index
 *     Packed-pixel colour index 0..15. Only the low four bits are used.
 *
 * Every newly rendered framebuffer begins filled with this colour. Sprite
 * pixels matching it are transparent, and transparency-aware BLIT preserves
 * the destination where its source pixel matches it. The selected index is
 * also presented to the external transparency hardware.
 */
void twodfx_set_transparent_color(uint8_t color_index);

/**
 * Enables or disables the flashing on-screen render-overrun indicator.
 *
 * @param enabled
 *     true enables the indicator; false disables it.
 *
 * This does not clear an already-set sticky render-overrun status.
 */
void twodfx_set_render_overrun_visual(bool enabled);

/**
 * Clears the sticky render-overrun status.
 *
 * This also releases the corresponding host status indication. It does not
 * change whether the flashing on-screen indicator is enabled.
 */
void twodfx_clear_render_overrun(void);

/**
 * Programs one of the four raster interrupt comparators.
 *
 * @param interrupt_number
 *     Raster interrupt number 1..4. Other values are ignored.
 *
 * @param line
 *     0 disables the selected comparator. Values 1..311 arm it for the
 *     corresponding absolute scanline. Values above 311 are treated as
 *     disabled.
 *
 * Reprogramming a comparator also clears that comparator's pending latch.
 */
void twodfx_set_raster_interrupt(uint8_t interrupt_number, uint16_t line);

/**
 * Clears all four pending raster interrupt latches.
 *
 * Programmed comparator lines remain unchanged and can trigger again on later
 * matching scanlines.
 */
void twodfx_clear_raster_pending(void);

/**
 * Shows the built-in 206x79-pixel 2dfx logo overlay.
 *
 * @param x
 *     Raw X coordinate of the logo's top-left corner, 0..1023.
 *
 * @param y
 *     Raw Y coordinate of the logo's top-left corner, 0..511.
 *
 * @param main_color
 *     Enterprise colour index for the main white/text portion.
 *
 * @param accent_color
 *     Enterprise colour index for the curved red X portion.
 *
 * White background pixels in the stored artwork are transparent. Enterprise
 * uses the supplied colour nibbles. TVC deliberately ignores them and always
 * uses white index 15 and red index 3. The logo renders above both 2DPT layers
 * and all sprites.
 */
void twodfx_show_builtin_logo(uint16_t x, uint16_t y,
                              uint8_t main_color,
                              uint8_t accent_color);

/**
 * Hides the built-in logo overlay.
 *
 * Its last position and colour selection may remain stored internally, but the
 * logo is no longer drawn.
 */
void twodfx_hide_builtin_logo(void);

/**
 * Clears all 64 shadow sprite parameter blocks.
 *
 * Every sprite becomes disabled when the shadow scene is next published.
 * Resource RAM is not modified. The collision indication continues to
 * represent the previous completed frame until a newer frame result is
 * published.
 */
void twodfx_reset_sprites(void);

/**
 * Replaces every background 2DPT slot with RET.
 *
 * Background replay therefore stops at slot 0 when the shadow table is next
 * published. The foreground 2DPT is unchanged.
 */
void twodfx_reset_2dpt_background(void);

/**
 * Replaces every foreground 2DPT slot with RET.
 *
 * Foreground replay therefore stops at slot 0 when the shadow table is next
 * published. The background 2DPT is unchanged.
 */
void twodfx_reset_2dpt_foreground(void);

/** Replaces every slot of both background and foreground 2DPTs with RET. */
void twodfx_reset_2dpt_all(void);

/**
 * Clears every font, bitmap, pattern, tileset and tilemap descriptor.
 *
 * Resource RAM contents are not modified. Renderer font/tile caches associated
 * with previous descriptor state are invalidated.
 */
void twodfx_reset_resource_descriptors(void);

/**
 * Disables all four raster interrupt comparators and clears every pending
 * raster interrupt latch.
 */
void twodfx_reset_raster_interrupts(void);

/* ------------------------------------------------------------------------- */
/* Resource RAM, uploads and asynchronous resource jobs                      */
/* ------------------------------------------------------------------------- */

/** Total size of the 2dfx resource RAM in bytes. */
#define TWODFX_RRAM_SIZE_BYTES   0x800000u

/** Highest valid 23-bit resource RAM address. */
#define TWODFX_RRAM_LAST_ADDRESS 0x7FFFFFu

/**
 * Copies byte-exact data into 2dfx resource RAM.
 *
 * @param destination_address
 *     First destination byte. Only address bits 22..0 are used.
 *
 * @param source
 *     Pointer to source bytes in RP2354 flash or SRAM. NULL performs no
 *     operation.
 *
 * @param length
 *     Number of bytes to copy. Zero performs no operation.
 *
 * A range crossing the end of the 8 MiB resource space wraps to address zero.
 * Unlike the Z80 UPLOAD_RAW transport command, zero does not mean 65536 bytes.
 * The operation is synchronous and does not stop rendering; do not overwrite
 * a resource that the current render may still be reading.
 */
void twodfx_upload_raw(uint32_t destination_address,
                       const void *source,
                       uint32_t length);

/**
 * Converts Enterprise/NICK 16-colour bytes while copying them into RRAM.
 *
 * @param destination_address
 *     First destination byte. Only address bits 22..0 are used.
 *
 * @param source
 *     Pointer to uncompressed NICK-format source bytes. NULL performs no
 *     operation.
 *
 * @param length
 *     Number of source bytes to convert and copy. Zero performs no operation.
 *
 * In each source byte, bits 7,5,3,1 form the left pixel and bits 6,4,2,0 form
 * the right pixel. The converted left pixel is stored in the internal low
 * nibble and the right pixel in the high nibble. This is a source-format
 * operation available on both machine profiles. It is synchronous and uses
 * the same authoritative conversion LUT as the host UPLOAD_NICK command.
 */
void twodfx_upload_nick(uint32_t destination_address,
                        const void *source,
                        uint32_t length);

/**
 * Converts TVC Graphics-16 bytes while copying them into resource RAM.
 *
 * @param destination_address
 *     First destination byte. Only address bits 22..0 are used.
 *
 * @param source
 *     Pointer to uncompressed interleaved-IIGGRRBB TVC bytes. NULL performs no
 *     operation.
 *
 * @param length
 *     Number of source bytes to convert and copy. Zero performs no operation.
 *
 * In each source byte, bits 7,5,3,1 form the left IGRB pixel and bits 6,4,2,0
 * form the right IGRB pixel. Each pixel is converted to TVC external-colour
 * numeric order BGRI; left is stored in the internal low nibble and right in
 * the high nibble. This source-format operation is available on either machine
 * and uses the same LUT as the host UPLOAD_TVC command.
 */
void twodfx_upload_tvc(uint32_t destination_address,
                       const void *source,
                       uint32_t length);

/**
 * Fills a resource-RAM range with one byte value.
 *
 * @param destination_address
 *     First destination byte. Only address bits 22..0 are used.
 *
 * @param value
 *     Byte written to every destination position.
 *
 * @param length
 *     Number of bytes to fill. Zero performs no operation.
 *
 * The range wraps at the end of the 8 MiB resource space. The operation is
 * synchronous and does not stop rendering.
 */
void twodfx_rram_fill(uint32_t destination_address,
                      uint8_t value,
                      uint32_t length);

/** State of the current or most recently completed resource job. */
typedef enum {
    /** No asynchronous resource job has run since initialization/reset. */
    TWODFX_RESOURCE_JOB_NONE = 0,

    /** A resource job is queued or running on core1. */
    TWODFX_RESOURCE_JOB_BUSY,

    /** The most recent resource job completed successfully. */
    TWODFX_RESOURCE_JOB_SUCCESS,

    /** The most recent UNZX1 job rejected an invalid or malformed stream. */
    TWODFX_RESOURCE_JOB_ERROR
} twodfx_resource_job_status_t;

/**
 * Returns the current or most recent asynchronous resource-job state.
 *
 * @return
 *     TWODFX_RESOURCE_JOB_NONE, BUSY, SUCCESS or ERROR.
 *
 * Starting an accepted job changes the state to BUSY. After completion the
 * state remains SUCCESS or ERROR until another job starts or full firmware
 * reset restores NONE.
 */
twodfx_resource_job_status_t twodfx_resource_job_status(void);

/**
 * Starts an asynchronous clear of the complete 8 MiB resource RAM.
 *
 * @return
 *     true if the job was accepted; false if another asynchronous resource job
 *     is already pending/running or terminal/reset state prevents it.
 *
 * A render already running is allowed to finish. A render that is queued but
 * has not begun is discarded. No new render starts while the clear is pending
 * or running, and the current front framebuffer remains displayed.
 *
 * This clears only RRAM. It does not stop the active egg, clear sprites or
 * 2DPTs, remove descriptors or change engine state. Existing descriptors keep
 * their addresses but refer to zero-filled data after completion.
 */
bool twodfx_reset_rram(void);

/**
 * Queues asynchronous ZX1 decompression with byte-exact output.
 *
 * @param source_address
 *     Address of a ZX1-compressed stream previously stored byte-exact in RRAM.
 *
 * @param destination_address
 *     First destination address for decompressed output.
 *
 * @return
 *     true if accepted; false if another resource job is pending/running.
 *
 * The job runs on core1 after an already-running render completes. No new
 * render starts while it is pending/running; the current front framebuffer
 * remains displayed. Poll twodfx_resource_job_status() for completion.
 */
bool twodfx_unzx1_raw(uint32_t source_address,
                      uint32_t destination_address);

/**
 * Queues asynchronous ZX1 decompression of NICK-format graphics.
 *
 * @param source_address
 *     Address of a ZX1-compressed NICK-format stream stored byte-exact in
 *     RRAM. Only address bits 22..0 are used.
 *
 * @param destination_address
 *     First destination address for the converted decompressed output. Only
 *     address bits 22..0 are used.
 *
 * @return
 *     true if the job was accepted; false if another resource job is already
 *     pending/running or terminal/reset state prevents it.
 *
 * Literal output bytes use the same conversion as twodfx_upload_nick(); ZX1
 * match copies reproduce already-converted destination bytes. Store the
 * compressed stream byte-exact, normally with twodfx_upload_raw(). Scheduling
 * behavior matches twodfx_unzx1_raw().
 */
bool twodfx_unzx1_nick(uint32_t source_address,
                       uint32_t destination_address);

/**
 * Queues asynchronous ZX1 decompression of TVC IIGGRRBB graphics.
 *
 * @param source_address
 *     Address of a ZX1-compressed TVC-format stream stored byte-exact in RRAM.
 *     Only address bits 22..0 are used.
 *
 * @param destination_address
 *     First destination address for the converted decompressed output. Only
 *     address bits 22..0 are used.
 *
 * @return
 *     true if the job was accepted; false if another resource job is already
 *     pending/running or terminal/reset state prevents it.
 *
 * Literal output bytes use the same conversion as twodfx_upload_tvc(); ZX1
 * match copies reproduce already-converted destination bytes. Store the
 * compressed stream byte-exact, normally with twodfx_upload_raw(). Scheduling
 * behavior matches twodfx_unzx1_raw().
 */
bool twodfx_unzx1_tvc(uint32_t source_address,
                      uint32_t destination_address);

/* ------------------------------------------------------------------------- */
/* Resource descriptors                                                      */
/* ------------------------------------------------------------------------- */

#define TWODFX_FONT_COUNT      64u
#define TWODFX_BITMAP_COUNT   256u
#define TWODFX_PATTERN_COUNT   64u
#define TWODFX_TILESET_COUNT   64u
#define TWODFX_TILEMAP_COUNT   32u

#define TWODFX_FONT_GLYPHS    128u

#define TWODFX_PATTERN_WIDTH   16u
#define TWODFX_PATTERN_HEIGHT   8u
#define TWODFX_PATTERN_BYTES   64u

#define TWODFX_TILE_WIDTH      16u
#define TWODFX_TILE_HEIGHT      8u
#define TWODFX_TILE_BYTES      64u

/**
 * Defines one fixed-cell 1-bpp font bank in resource RAM.
 *
 * @param font
 *     Font descriptor number 0..63.
 *
 * @param resource_address
 *     First byte of glyph 0. Only address bits 22..0 are used.
 *
 * @param width_units
 *     Glyph width in units of eight pixels, 1..16 (8..128 pixels).
 *
 * @param height_units
 *     Glyph height in units of eight pixels, 1..16 (8..128 pixels).
 *
 * A font contains exactly 128 sequential glyphs numbered 0..127. Each row
 * occupies width_units bytes and bit 7 of its first byte is the leftmost pixel.
 * Bytes per glyph are width_units * (height_units * 8). The function defines
 * only the descriptor; font bytes must already be present in RRAM.
 */
void twodfx_define_font(uint8_t font,
                        uint32_t resource_address,
                        uint8_t width_units,
                        uint8_t height_units);

/**
 * Defines one packed 4-bpp bitmap for BLIT.
 *
 * @param bitmap
 *     Bitmap descriptor number 0..255.
 *
 * @param resource_address
 *     Address of the first packed byte. Only bits 22..0 are used.
 *
 * @param width
 *     Source width in pixels, 0..1023. Odd width is rounded down to the
 *     previous even value.
 *
 * @param height
 *     Source height in pixels, 0..511.
 *
 * Rows are consecutive without padding or separate stride; row length is
 * effective_even_width / 2 bytes. Pixel X=0 is in the low nibble and X=1 in
 * the high nibble. Zero effective width or zero height invalidates the
 * descriptor. The source bytes must already exist in RRAM.
 */
void twodfx_define_bitmap(uint8_t bitmap,
                          uint32_t resource_address,
                          uint16_t width,
                          uint16_t height);

/**
 * Defines one bank of fixed 16x8-pixel patterns for PATTERN_FILL.
 *
 * @param pattern
 *     Pattern-bank descriptor number 0..63.
 *
 * @param resource_address
 *     Address of pattern 0. Only bits 22..0 are used.
 *
 * A bank contains up to 256 sequential patterns. Each pattern occupies exactly
 * 64 bytes in internal packed 4-bpp format. This function defines only the
 * bank address; graphics must already be present in RRAM.
 */
void twodfx_define_pattern(uint8_t pattern,
                           uint32_t resource_address);

/**
 * Defines one bank of fixed 16x8-pixel tiles for TILEMAP.
 *
 * @param tileset
 *     Tileset descriptor number 0..63.
 *
 * @param resource_address
 *     Address of tile 0. Only bits 22..0 are used.
 *
 * A bank contains up to 256 sequential tiles. Each tile occupies exactly 64
 * bytes in internal packed 4-bpp format. This function defines only the bank
 * address; tile graphics must already be present in RRAM.
 */
void twodfx_define_tileset(uint8_t tileset,
                           uint32_t resource_address);

/**
 * Defines one complete tilemap and associates it with a tileset.
 *
 * @param tilemap
 *     Tilemap descriptor number 0..31.
 *
 * @param resource_address
 *     Address of the first tilemap cell. Only bits 22..0 are used.
 *
 * @param width_cells
 *     Complete map width in cells, 1..256.
 *
 * @param height_cells
 *     Complete map height in cells, 1..256.
 *
 * @param tileset
 *     Tileset descriptor number 0..63 used to draw the map.
 *
 * The map is a row-major array with one tile-number byte per cell. Cell (x,y)
 * is at resource_address + y*width_cells + x. This function defines only the
 * descriptor; cells and tile graphics must already exist in RRAM.
 */
void twodfx_define_tilemap(uint8_t tilemap,
                           uint32_t resource_address,
                           uint16_t width_cells,
                           uint16_t height_cells,
                           uint8_t tileset);

/* ------------------------------------------------------------------------- */
/* Sprites                                                                   */
/* ------------------------------------------------------------------------- */

/** Number of sprite parameter blocks; valid sprite numbers are 0..63. */
#define TWODFX_SPRITE_COUNT 64u

/* Transform flags map directly to SPB BYTE6 bits 7..4 and may be ORed. */
#define TWODFX_SPRITE_X_DOUBLE 0x80u
#define TWODFX_SPRITE_Y_DOUBLE 0x40u
#define TWODFX_SPRITE_X_FLIP   0x20u
#define TWODFX_SPRITE_Y_FLIP   0x10u

/**
 * Writes one complete nine-byte sprite parameter block.
 *
 * Every call replaces the complete shadow definition. The C API deliberately
 * does not expose partial SPB updates; that optimization remains specific to
 * the host ALTER_SPB command.
 *
 * @param sprite
 *     Sprite number 0..63.
 *
 * @param resource_address
 *     Address of the first packed source byte. Only bits 22..0 are used.
 *
 * @param x
 *     Absolute left-edge coordinate 0..1023. Off-screen positions within this
 *     field are valid and clip normally.
 *
 * @param y
 *     Absolute top-edge coordinate 0..511. Off-screen positions within this
 *     field are valid and clip normally.
 *
 * @param width
 *     Original source width 0..1023 before optional X doubling.
 *
 * @param height
 *     Original source height 0..511 before optional Y doubling.
 *
 * @param enabled
 *     true enables the sprite; false stores the definition disabled.
 *
 * @param collision_enabled
 *     true includes it in global sprite collision detection while enabled.
 *
 * @param transform_flags
 *     Zero or a bitwise OR of TWODFX_SPRITE_X_DOUBLE,
 *     TWODFX_SPRITE_Y_DOUBLE, TWODFX_SPRITE_X_FLIP and
 *     TWODFX_SPRITE_Y_FLIP. Bits 3..0 must remain zero.
 *
 * Source format is internal packed 4-bpp: even X is in the low nibble, odd X
 * in the high nibble, rows are consecutive and row length is (width+1)/2.
 * Pixels matching the current transparent index are skipped. A sprite is
 * disabled when enabled is false or width/height is zero.
 *
 * Sprites draw in numerical order 0..63, so higher-numbered sprites appear
 * above lower-numbered ones. Complete renderer order is background 2DPT,
 * sprites 0..63, foreground 2DPT, built-in logo and optional overrun marker.
 * Collision is one global result with two-pixel horizontal granularity and is
 * evaluated over the complete stored framebuffer.
 */
void twodfx_sprite_set(uint8_t sprite,
                       uint32_t resource_address,
                       uint16_t x,
                       uint16_t y,
                       uint16_t width,
                       uint16_t height,
                       bool enabled,
                       bool collision_enabled,
                       uint8_t transform_flags);

/* ------------------------------------------------------------------------- */
/* 2DPT fundamentals                                                         */
/* ------------------------------------------------------------------------- */

/** Number of command slots in each background or foreground 2DPT. */
#define TWODFX_2DPT_SLOT_COUNT 256u

/** Selects the destination 2D Parameter Table. */
typedef enum {
    /** Replayed before sprites. */
    TWODFX_LAYER_BACKGROUND = 0,

    /** Replayed after sprites. */
    TWODFX_LAYER_FOREGROUND = 1
} twodfx_layer_t;

/*
 * Each layer contains slots 0..255. Replay begins at slot 0 and advances one
 * slot at a time unless SKIP_NEXT changes the next slot. RET ends that layer's
 * replay. Every function below replaces one complete shadow slot.
 *
 * Background and foreground independently maintain current foreground colour,
 * background colour and current X/Y offset. Every replay starts with foreground
 * 15, background equal to the current transparent index, and offset 0,0.
 */

/**
 * Writes a NOP command into one 2DPT slot.
 *
 * @param layer
 *     Background or foreground layer.
 *
 * @param slot
 *     Slot 0..255.
 *
 * NOP draws nothing, changes no replay state and continues with the next slot.
 */
void twodfx_2d_nop(twodfx_layer_t layer, uint8_t slot);

/**
 * Writes a RET command into one 2DPT slot.
 *
 * @param layer
 *     Background or foreground layer.
 *
 * @param slot
 *     Slot 0..255.
 *
 * RET immediately ends replay of the selected layer for the current frame.
 */
void twodfx_2d_ret(twodfx_layer_t layer, uint8_t slot);

/**
 * Sets persistent foreground and background colours for following commands.
 *
 * @param layer
 *     Background or foreground layer.
 *
 * @param slot
 *     Slot 0..255.
 *
 * @param foreground_color
 *     Foreground index 0..15; only the low nibble is stored.
 *
 * @param background_color
 *     Background index 0..15; only the low nibble is stored.
 *
 * Both remain active in the same playlist until another SET_COLOR or replay
 * end. The two layers maintain independent colour state.
 */
void twodfx_2d_set_color(twodfx_layer_t layer,
                         uint8_t slot,
                         uint8_t foreground_color,
                         uint8_t background_color);

/**
 * Sets a persistent signed destination-coordinate offset.
 *
 * @param layer
 *     Background or foreground layer.
 *
 * @param slot
 *     Slot 0..255.
 *
 * @param offset_x
 *     Signed horizontal offset -128..127.
 *
 * @param offset_y
 *     Signed vertical offset -128..127.
 *
 * SET_XY_OFFSET sets rather than accumulates. SET_XY_OFFSET 0,0 restores original
 * coordinates. It affects destination positions of all positional commands and
 * both LINE endpoints, but not widths, heights, radii, angles, resource IDs,
 * tilemap source coordinates or tile counts. Signed translated coordinates
 * clip normally and never wrap through an unsigned range.
 */
void twodfx_2d_set_xy_offset(twodfx_layer_t layer,
                        uint8_t slot,
                        int8_t offset_x,
                        int8_t offset_y);

/**
 * Skips a specified number of following slots.
 *
 * @param layer
 *     Background or foreground layer.
 *
 * @param slot
 *     Slot 0..255.
 *
 * @param count
 *     Number of immediately following slots to skip, 0..255.
 *
 * Skipped commands are not interpreted and have no effects, including skipped
 * SET_COLOR, SET_XY_OFFSET, SKIP_NEXT and RET. Crossing slot 255 safely ends replay;
 * execution cannot branch backward or loop.
 */
void twodfx_2d_skip_next(twodfx_layer_t layer,
                         uint8_t slot,
                         uint8_t count);

/* ------------------------------------------------------------------------- */
/* 2DPT primitives                                                           */
/* ------------------------------------------------------------------------- */

/**
 * Writes one pixel using the current foreground colour.
 *
 * @param layer
 *     Background or foreground layer.
 *
 * @param slot
 *     Slot 0..255.
 *
 * @param x
 *     Absolute X coordinate 0..1023.
 *
 * @param y
 *     Absolute Y coordinate 0..511.
 *
 * SET_XY_OFFSET is applied before framebuffer clipping.
 *
 * Enterprise/NICK caveat: in NORMAL engine mode, an isolated single visible
 * pixel following at least one transparent pixel may not be displayed because
 * of NICK external-colour behavior. FORCED_ON displays it correctly because
 * 2dfx supplies the complete scanline. This limitation does not apply to TVC.
 */
void twodfx_2d_plot(twodfx_layer_t layer,
                    uint8_t slot,
                    uint16_t x,
                    uint16_t y);

/**
 * Draws a line between two inclusive endpoints in foreground colour.
 *
 * @param layer
 *     Background or foreground layer.
 *
 * @param slot
 *     Slot 0..255.
 *
 * @param x0
 *     First endpoint X coordinate, 0..1023.
 *
 * @param y0
 *     First endpoint Y coordinate, 0..511.
 *
 * @param x1
 *     Second endpoint X coordinate, 0..1023.
 *
 * @param y1
 *     Second endpoint Y coordinate, 0..511.
 *
 * @param stroke
 *     Thickness 1..4.
 *
 * SET_XY_OFFSET affects both endpoints. A horizontal line is stroke scanlines high;
 * a vertical line is stroke*2 framebuffer pixels wide; a diagonal thick line
 * uses the corresponding aspect-compensated brush. Drawing clips normally.
 */
void twodfx_2d_line(twodfx_layer_t layer,
                    uint8_t slot,
                    uint16_t x0,
                    uint16_t y0,
                    uint16_t x1,
                    uint16_t y1,
                    uint8_t stroke);

/**
 * Draws a line using the current background colour.
 *
 * @param layer
 *     Background or foreground layer.
 *
 * @param slot
 *     Slot 0..255.
 *
 * @param x0
 *     First endpoint X coordinate, 0..1023.
 *
 * @param y0
 *     First endpoint Y coordinate, 0..511.
 *
 * @param x1
 *     Second endpoint X coordinate, 0..1023.
 *
 * @param y1
 *     Second endpoint Y coordinate, 0..511.
 *
 * @param stroke
 *     Thickness 1..4.
 *
 * Endpoint handling, SET_XY_OFFSET, clipping and aspect-compensated stroke behavior
 * are identical to twodfx_2d_line(). ERASE_LINE explicitly writes the current
 * background colour; it does not restore an older framebuffer value.
 */
void twodfx_2d_erase_line(twodfx_layer_t layer,
                          uint8_t slot,
                          uint16_t x0,
                          uint16_t y0,
                          uint16_t x1,
                          uint16_t y1,
                          uint8_t stroke);

/**
 * Flood-fills one four-directionally connected region in foreground colour.
 *
 * @param layer
 *     Background or foreground layer.
 *
 * @param slot
 *     Slot 0..255.
 *
 * @param x
 *     Seed X coordinate, 0..1023.
 *
 * @param y
 *     Seed Y coordinate, 0..511.
 *
 * SET_XY_OFFSET is applied to the seed. Out-of-bounds seeds and seeds already equal
 * to foreground do nothing. BUCKET_FILL is an expensive primitive with bounded
 * internal work storage; pathological regions can remain partially filled.
 * For repeated or complex shapes, preparing a bitmap and using BLIT can be
 * substantially faster.
 */
void twodfx_2d_bucket_fill(twodfx_layer_t layer,
                           uint8_t slot,
                           uint16_t x,
                           uint16_t y);

/**
 * Draws a filled rectangle using the current foreground colour.
 *
 * @param layer
 *     Background or foreground layer.
 *
 * @param slot
 *     Slot 0..255.
 *
 * @param x
 *     Top-left X coordinate, 0..1023.
 *
 * @param y
 *     Top-left Y coordinate, 0..511.
 *
 * @param width
 *     Rectangle width, 0..1023.
 *
 * @param height
 *     Rectangle height, 0..511.
 *
 * Zero width or height draws nothing. SET_XY_OFFSET affects x/y only; drawing clips
 * normally against the active framebuffer.
 */
void twodfx_2d_fill_rect(twodfx_layer_t layer,
                         uint8_t slot,
                         uint16_t x,
                         uint16_t y,
                         uint16_t width,
                         uint16_t height);

/**
 * Draws a stroked rectangle.
 *
 * @param layer
 *     Background or foreground layer.
 *
 * @param slot
 *     Slot 0..255.
 *
 * @param x
 *     Top-left X coordinate, 0..1023.
 *
 * @param y
 *     Top-left Y coordinate, 0..511.
 *
 * @param width
 *     Complete outer width, 0..1023.
 *
 * @param height
 *     Complete outer height, 0..511.
 *
 * @param stroke
 *     Border thickness 1..4.
 *
 * The border uses foreground. If background differs from the transparent
 * index, the interior is filled with background; otherwise existing interior
 * pixels are preserved. Horizontal borders are stroke rows high and vertical
 * borders are stroke*2 pixels wide. Zero dimensions draw nothing. SET_XY_OFFSET
 * affects x/y only.
 */
void twodfx_2d_rect(twodfx_layer_t layer,
                    uint8_t slot,
                    uint16_t x,
                    uint16_t y,
                    uint16_t width,
                    uint16_t height,
                    uint8_t stroke);

/**
 * Draws a filled rounded rectangle using the current foreground colour.
 *
 * @param layer
 *     Background or foreground layer.
 *
 * @param slot
 *     Slot 0..255.
 *
 * @param x
 *     Top-left X coordinate, 0..1023.
 *
 * @param y
 *     Top-left Y coordinate, 0..511.
 *
 * @param width
 *     Complete outer width, 0..1023.
 *
 * @param height
 *     Complete outer height, 0..511.
 *
 * @param radius_x
 *     Horizontal corner radius, 0..255.
 *
 * @param radius_y
 *     Vertical corner radius, 0..255.
 *
 * Radii too large for the rectangle are reduced automatically. A zero radius
 * on either axis produces an ordinary filled rectangle. Zero dimensions draw
 * nothing. SET_XY_OFFSET affects x/y only.
 */
void twodfx_2d_fill_round_rect(twodfx_layer_t layer,
                               uint8_t slot,
                               uint16_t x,
                               uint16_t y,
                               uint16_t width,
                               uint16_t height,
                               uint8_t radius_x,
                               uint8_t radius_y);

/**
 * Draws a stroked rounded rectangle.
 *
 * @param layer
 *     Background or foreground layer.
 *
 * @param slot
 *     Slot 0..255.
 *
 * @param x
 *     Top-left X coordinate, 0..1023.
 *
 * @param y
 *     Top-left Y coordinate, 0..511.
 *
 * @param width
 *     Complete outer width, 0..1023.
 *
 * @param height
 *     Complete outer height, 0..511.
 *
 * @param stroke
 *     Border thickness 1..4.
 *
 * @param radius_x
 *     Horizontal corner radius, 0..255.
 *
 * @param radius_y
 *     Vertical corner radius, 0..255.
 *
 * Radius normalization matches twodfx_2d_fill_round_rect(). Foreground and
 * background interior behavior matches RECT. A zero radius on either axis
 * produces an ordinary RECT with the same dimensions and stroke. SET_XY_OFFSET
 * affects x/y only.
 */
void twodfx_2d_round_rect(twodfx_layer_t layer,
                          uint8_t slot,
                          uint16_t x,
                          uint16_t y,
                          uint16_t width,
                          uint16_t height,
                          uint8_t stroke,
                          uint8_t radius_x,
                          uint8_t radius_y);

/**
 * Draws a filled ellipse using centre/radius semantics and foreground colour.
 *
 * @param layer
 *     Background or foreground layer.
 *
 * @param slot
 *     Slot 0..255.
 *
 * @param center_x
 *     Centre X coordinate, 0..1023.
 *
 * @param center_y
 *     Centre Y coordinate, 0..511.
 *
 * @param radius_x
 *     Horizontal radius, 0..1023.
 *
 * @param radius_y
 *     Vertical radius, 0..511.
 *
 * Nominal dimensions are 2*radius_x+1 by 2*radius_y+1. Zero radius is valid
 * and produces a one-pixel diameter on that axis. SET_XY_OFFSET affects the centre
 * only; drawing clips normally.
 */
void twodfx_2d_fill_ellipse(twodfx_layer_t layer,
                            uint8_t slot,
                            uint16_t center_x,
                            uint16_t center_y,
                            uint16_t radius_x,
                            uint16_t radius_y);

/**
 * Draws a stroked ellipse using centre/radius semantics.
 *
 * @param layer
 *     Background or foreground layer.
 *
 * @param slot
 *     Slot 0..255.
 *
 * @param center_x
 *     Centre X coordinate, 0..1023.
 *
 * @param center_y
 *     Centre Y coordinate, 0..511.
 *
 * @param radius_x
 *     Horizontal radius, 0..1023.
 *
 * @param radius_y
 *     Vertical radius, 0..511.
 *
 * @param stroke
 *     Ring thickness 1..4.
 *
 * The ring uses foreground. If background differs from transparent, the
 * interior is filled with background; otherwise it is preserved. Horizontal
 * thickness uses stroke*2 pixels and vertical thickness uses stroke scanlines.
 * SET_XY_OFFSET affects the centre only.
 */
void twodfx_2d_ellipse(twodfx_layer_t layer,
                       uint8_t slot,
                       uint16_t center_x,
                       uint16_t center_y,
                       uint16_t radius_x,
                       uint16_t radius_y,
                       uint8_t stroke);

/* Useful positions in the eight-bit clockwise angle system. */
#define TWODFX_ANGLE_TOP      0u
#define TWODFX_ANGLE_RIGHT   64u
#define TWODFX_ANGLE_BOTTOM 128u
#define TWODFX_ANGLE_LEFT   192u

/**
 * Draws a stroked clockwise section of an ellipse.
 *
 * @param layer
 *     Background or foreground layer.
 *
 * @param slot
 *     Slot 0..255.
 *
 * @param center_x
 *     Centre X coordinate, 0..1023.
 *
 * @param center_y
 *     Centre Y coordinate, 0..511.
 *
 * @param radius_x
 *     Horizontal radius, 0..1023.
 *
 * @param radius_y
 *     Vertical radius, 0..511.
 *
 * @param start_angle
 *     First angle in the eight-bit clockwise system.
 *
 * @param end_angle
 *     Final angle in the eight-bit clockwise system.
 *
 * @param stroke
 *     Arc thickness 1..4.
 *
 * Angles use 0 top, 64 right, 128 bottom and 192 left; the sweep wraps through
 * 255 to 0. Equal start/end angles are an empty sweep and draw nothing. ARC
 * uses foreground only, ignores background, and applies SET_XY_OFFSET to the centre
 * only.
 *
 * ARC is expensive. For repeated or complex shapes, a prepared bitmap and
 * BLIT may be faster.
 */
void twodfx_2d_arc(twodfx_layer_t layer,
                   uint8_t slot,
                   uint16_t center_x,
                   uint16_t center_y,
                   uint16_t radius_x,
                   uint16_t radius_y,
                   uint8_t start_angle,
                   uint8_t end_angle,
                   uint8_t stroke);

/**
 * Draws a filled clockwise elliptical sector in foreground colour.
 *
 * @param layer
 *     Background or foreground layer.
 *
 * @param slot
 *     Slot 0..255.
 *
 * @param center_x
 *     Centre X coordinate, 0..1023.
 *
 * @param center_y
 *     Centre Y coordinate, 0..511.
 *
 * @param radius_x
 *     Horizontal radius, 0..1023.
 *
 * @param radius_y
 *     Vertical radius, 0..511.
 *
 * @param start_angle
 *     First angle in the eight-bit clockwise system.
 *
 * @param end_angle
 *     Final angle in the eight-bit clockwise system.
 *
 * Angles use 0 top, 64 right, 128 bottom and 192 left; the sweep wraps through
 * 255 to 0. The sector includes radial sides from the centre to both ellipse-
 * edge endpoints. Equal start/end angles draw nothing. Background is ignored
 * and SET_XY_OFFSET affects the centre only.
 *
 * PIE is expensive. For repeated or complex shapes, a prepared bitmap and
 * BLIT may be faster.
 */
void twodfx_2d_pie(twodfx_layer_t layer,
                   uint8_t slot,
                   uint16_t center_x,
                   uint16_t center_y,
                   uint16_t radius_x,
                   uint16_t radius_y,
                   uint8_t start_angle,
                   uint8_t end_angle);

/* ------------------------------------------------------------------------- */
/* Resource-based 2DPT commands                                              */
/* ------------------------------------------------------------------------- */

/** Terminates a DRAW_TEXT string stored in RRAM. */
#define TWODFX_TEXT_TERMINATOR 0x80u

/** Maximum number of glyphs processed by one DRAW_TEXT command. */
#define TWODFX_TEXT_MAX_CHARS 255u

/**
 * Draws a horizontal string using a defined font descriptor.
 *
 * @param layer
 *     Background or foreground layer.
 *
 * @param slot
 *     Slot 0..255.
 *
 * @param x
 *     Top-left X coordinate of the first character cell, 0..1023.
 *
 * @param y
 *     Top-left Y coordinate of the first character cell, 0..511.
 *
 * @param font
 *     Font descriptor number 0..63.
 *
 * @param string_address
 *     Address of the first raw string byte; only bits 22..0 are used.
 *
 * Bytes 00h..7Fh select glyphs directly. 80h terminates; reserved 81h..FFh
 * also stop safely. At most 255 glyphs are processed. Text is horizontal with
 * no newline, wrap, kerning or variable spacing.
 *
 * Set font bits use foreground. If background differs from transparent, each
 * complete cell is filled with background before the glyph is drawn; otherwise
 * clear bits preserve existing pixels. SET_XY_OFFSET affects x/y only. The font and
 * raw string bytes must already exist in RRAM.
 */
void twodfx_2d_draw_text(twodfx_layer_t layer,
                         uint8_t slot,
                         uint16_t x,
                         uint16_t y,
                         uint8_t font,
                         uint32_t string_address);

/** Selects exact or transparency-aware bitmap copying. */
typedef enum {
    /** Copy every source pixel exactly; fastest mode. */
    TWODFX_BLIT_EXACT = 0,

    /** Preserve destination pixels where source equals transparent index. */
    TWODFX_BLIT_TRANSPARENT = 1
} twodfx_blit_mode_t;

/**
 * Draws one complete bitmap described by a bitmap descriptor.
 *
 * @param layer
 *     Background or foreground layer.
 *
 * @param slot
 *     Slot 0..255.
 *
 * @param x
 *     Destination top-left X coordinate, 0..1023.
 *
 * @param y
 *     Destination top-left Y coordinate, 0..511.
 *
 * @param bitmap
 *     Bitmap descriptor number 0..255.
 *
 * @param mode
 *     TWODFX_BLIT_EXACT or TWODFX_BLIT_TRANSPARENT.
 *
 * BLIT has no source subrectangle, scaling, flipping or custom stride. SET_XY_OFFSET
 * is applied first, then effective destination X is aligned down to even.
 * EXACT copies every source pixel, including the transparent index.
 * TRANSPARENT skips each individual matching source pixel and preserves the
 * destination underneath. Current foreground/background colours are unused.
 */
void twodfx_2d_blit(twodfx_layer_t layer,
                    uint8_t slot,
                    uint16_t x,
                    uint16_t y,
                    uint8_t bitmap,
                    twodfx_blit_mode_t mode);

/**
 * Fills a rectangle by repeating one fixed 16x8 pattern.
 *
 * @param layer
 *     Background or foreground layer.
 *
 * @param slot
 *     Slot 0..255.
 *
 * @param x
 *     Requested top-left X coordinate, 0..1023.
 *
 * @param y
 *     Requested top-left Y coordinate, 0..511.
 *
 * @param width
 *     Requested width, 0..1023.
 *
 * @param height
 *     Requested height, 0..511.
 *
 * @param pattern
 *     Pattern-bank descriptor number 0..63.
 *
 * @param pattern_number
 *     Pattern number 0..255 within the bank.
 *
 * Repetition phase is anchored to the requested top-left even when clipped.
 * Copying is exact: transparent-index pixels are still written and current
 * colours are unused. After SET_XY_OFFSET, effective X and width are aligned down
 * to even. Zero effective width or zero height draws nothing.
 */
void twodfx_2d_pattern_fill(twodfx_layer_t layer,
                            uint8_t slot,
                            uint16_t x,
                            uint16_t y,
                            uint16_t width,
                            uint16_t height,
                            uint8_t pattern,
                            uint8_t pattern_number);

/**
 * Draws a rectangular viewport from a defined tilemap.
 *
 * @param layer
 *     Background or foreground layer.
 *
 * @param slot
 *     Slot 0..255.
 *
 * @param x
 *     Destination X coordinate of the viewport's top-left tile, 0..1023.
 *
 * @param y
 *     Destination Y coordinate of the viewport's top-left tile, 0..511.
 *
 * @param tilemap
 *     Tilemap descriptor number 0..31.
 *
 * @param source_tile_x
 *     X coordinate of the first source cell inside the complete map.
 *
 * @param source_tile_y
 *     Y coordinate of the first source cell inside the complete map.
 *
 * @param columns
 *     Requested tile columns, 0..255. Zero draws nothing.
 *
 * @param rows
 *     Requested tile rows, 0..255. Zero draws nothing.
 *
 * A source start outside the map draws nothing. Right/bottom overflow clips to
 * remaining valid cells; it never wraps or repeats and missing destination
 * cells remain untouched. SET_XY_OFFSET affects destination only, then effective X
 * is aligned down to even. Tile pixels copy exactly, including transparent-
 * index pixels; foreground/background colours are unused.
 */
void twodfx_2d_tilemap(twodfx_layer_t layer,
                       uint8_t slot,
                       uint16_t x,
                       uint16_t y,
                       uint8_t tilemap,
                       uint8_t source_tile_x,
                       uint8_t source_tile_y,
                       uint8_t columns,
                       uint8_t rows);

/* ------------------------------------------------------------------------- */
/* Status and readback                                                       */
/* ------------------------------------------------------------------------- */

/**
 * Returns the collision result from the most recently published completed
 * frame.
 *
 * @return
 *     true if at least one pair of enabled, collision-enabled sprites
 *     collided; false otherwise.
 *
 * The result is global and does not identify sprites or position. It remains
 * stable during skipped 25 fps frames or long renders until a newer completed
 * frame publishes another result. Precision and participation rules are
 * documented with twodfx_sprite_set().
 */
bool twodfx_collision_detected(void);

/**
 * Returns the sticky render-budget overrun status.
 *
 * @return
 *     true if a render exceeded the available update budget since the status
 *     was last cleared; false otherwise.
 *
 * The approximate budget is 20 ms in 50 fps mode and 40 ms in 25 fps mode.
 * Once set, it remains true until twodfx_clear_render_overrun(). This status is
 * independent of the optional flashing visual indicator.
 */
bool twodfx_render_overrun_detected(void);

/**
 * Returns whether one raster interrupt comparator has a pending event.
 *
 * @param interrupt_number
 *     Comparator 1..4. Other values return false.
 *
 * @return
 *     true if the selected comparator has a pending event; false otherwise.
 *
 * Pending remains set until all latches are cleared, that comparator is
 * reprogrammed, or all raster interrupts are reset.
 */
bool twodfx_raster_interrupt_pending(uint8_t interrupt_number);

#ifdef __cplusplus
}
#endif

#endif /* TWODFX_API_H */
