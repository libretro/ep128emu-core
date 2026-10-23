#include "egg_CC_diag.h"

#include <stdbool.h>
#include <stdint.h>

#include "egg_CC_diag_assets.h"

/* ------------------------------------------------------------------------- */
/* Egg state                                                                 */
/* ------------------------------------------------------------------------- */

static bool g_is_tvc;
static uint16_t g_stage_tick;
static egg_CC_stage_t g_stage;
static bool g_zx1_queued;
static uint16_t g_frame_width;
static uint16_t g_frame_height;

static const uint32_t g_address_resources[8] = EGG_CC_ADDRESS_VALUES;
static const uint32_t g_cache_resources[4] = EGG_CC_CACHE_ADDRESS_VALUES;
static const uint8_t g_transform_flags[EGG_CC_TRANSFORM_COUNT] =
    EGG_CC_TRANSFORM_FLAG_VALUES;

static uint8_t background_colour(void)
{
    return g_is_tvc
        ? EGG_CC_TVC_BACKGROUND_COLOUR
        : EGG_CC_EP_BACKGROUND_COLOUR;
}

/* ------------------------------------------------------------------------- */
/* Separately named flash assets                                              */
/* ------------------------------------------------------------------------- */

static void upload_address_asset(uint8_t index, uint32_t address)
{
    switch (index) {
        case 0u:
            twodfx_upload_raw(address, egg_CC_address_sprite0_raw,
                              sizeof(egg_CC_address_sprite0_raw));
            break;
        case 1u:
            twodfx_upload_raw(address, egg_CC_address_sprite1_raw,
                              sizeof(egg_CC_address_sprite1_raw));
            break;
        case 2u:
            twodfx_upload_raw(address, egg_CC_address_sprite2_raw,
                              sizeof(egg_CC_address_sprite2_raw));
            break;
        case 3u:
            twodfx_upload_raw(address, egg_CC_address_sprite3_raw,
                              sizeof(egg_CC_address_sprite3_raw));
            break;
        case 4u:
            twodfx_upload_raw(address, egg_CC_address_sprite4_raw,
                              sizeof(egg_CC_address_sprite4_raw));
            break;
        case 5u:
            twodfx_upload_raw(address, egg_CC_address_sprite5_raw,
                              sizeof(egg_CC_address_sprite5_raw));
            break;
        case 6u:
            twodfx_upload_raw(address, egg_CC_address_sprite6_raw,
                              sizeof(egg_CC_address_sprite6_raw));
            break;
        default:
            twodfx_upload_raw(address, egg_CC_address_sprite7_raw,
                              sizeof(egg_CC_address_sprite7_raw));
            break;
    }
}

static void upload_stage0_cache_asset(uint8_t index, uint32_t address)
{
    switch (index) {
        case 0u:
            twodfx_upload_raw(address, egg_CC_stage0_cache_sprite0_raw,
                              sizeof(egg_CC_stage0_cache_sprite0_raw));
            break;
        case 1u:
            twodfx_upload_raw(address, egg_CC_stage0_cache_sprite1_raw,
                              sizeof(egg_CC_stage0_cache_sprite1_raw));
            break;
        case 2u:
            twodfx_upload_raw(address, egg_CC_stage0_cache_sprite2_raw,
                              sizeof(egg_CC_stage0_cache_sprite2_raw));
            break;
        default:
            twodfx_upload_raw(address, egg_CC_stage0_cache_sprite3_raw,
                              sizeof(egg_CC_stage0_cache_sprite3_raw));
            break;
    }
}

static void upload_stage2_cache_asset(uint8_t index, uint32_t address)
{
    switch (index) {
        case 0u:
            twodfx_upload_raw(
                address,
                g_is_tvc ? egg_CC_tvc_stage2_cache_sprite0_raw
                         : egg_CC_ep_stage2_cache_sprite0_raw,
                sizeof(egg_CC_ep_stage2_cache_sprite0_raw));
            break;
        case 1u:
            twodfx_upload_raw(
                address,
                g_is_tvc ? egg_CC_tvc_stage2_cache_sprite1_raw
                         : egg_CC_ep_stage2_cache_sprite1_raw,
                sizeof(egg_CC_ep_stage2_cache_sprite1_raw));
            break;
        case 2u:
            twodfx_upload_raw(
                address,
                g_is_tvc ? egg_CC_tvc_stage2_cache_sprite2_raw
                         : egg_CC_ep_stage2_cache_sprite2_raw,
                sizeof(egg_CC_ep_stage2_cache_sprite2_raw));
            break;
        default:
            twodfx_upload_raw(
                address,
                g_is_tvc ? egg_CC_tvc_stage2_cache_sprite3_raw
                         : egg_CC_ep_stage2_cache_sprite3_raw,
                sizeof(egg_CC_ep_stage2_cache_sprite3_raw));
            break;
    }
}

/* ------------------------------------------------------------------------- */
/* Common stage frame                                                        */
/* ------------------------------------------------------------------------- */

static void begin_stage(uint8_t stage_colour)
{
    uint8_t slot = 0u;

    twodfx_reset_sprites();
    twodfx_reset_2dpt_all();

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                        background_colour(),
                        EGG_CC_TRANSPARENT_COLOUR);
    twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND, slot++,
                        EGG_CC_FRAME_X,
                        EGG_CC_FRAME_Y,
                        g_frame_width,
                        g_frame_height);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                        stage_colour,
                        EGG_CC_TRANSPARENT_COLOUR);
    twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND, slot++,
                        EGG_CC_MARKER_X,
                        EGG_CC_MARKER_Y,
                        EGG_CC_MARKER_W,
                        EGG_CC_MARKER_H);
    twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND, slot++,
                        EGG_CC_PROGRESS_X,
                        EGG_CC_PROGRESS_Y,
                        (uint16_t)((g_stage + 1u) * EGG_CC_PROGRESS_STEP_W),
                        EGG_CC_PROGRESS_H);

    twodfx_2d_ret(TWODFX_LAYER_BACKGROUND, slot);
}

/* ------------------------------------------------------------------------- */
/* Stage 0: 23-bit addresses and cache tags                                  */
/* ------------------------------------------------------------------------- */

static void build_address_stage(void)
{
    begin_stage(EGG_CC_STAGE0_COLOUR);

    for (uint8_t i = 0u; i < 8u; ++i) {
        upload_address_asset(i, g_address_resources[i]);

        twodfx_sprite_set(i,
                          g_address_resources[i],
                          (uint16_t)(EGG_CC_ADDRESS_FIRST_X + i * EGG_CC_ADDRESS_STEP_X),
                          EGG_CC_ADDRESS_Y,
                          EGG_CC_ADDRESS_SPRITE_W,
                          EGG_CC_ADDRESS_SPRITE_H,
                          true,
                          false,
                          0u);
    }

    /* Same low address, different high bits: catches old low-address tags. */
    for (uint8_t i = 0u; i < 4u; ++i) {
        upload_stage0_cache_asset(i, g_cache_resources[i]);

        twodfx_sprite_set((uint8_t)(8u + i),
                          g_cache_resources[i],
                          (uint16_t)(EGG_CC_CACHE_FIRST_X + i * EGG_CC_CACHE_STEP_X),
                          EGG_CC_CACHE_Y,
                          EGG_CC_CACHE_SPRITE_W,
                          EGG_CC_CACHE_SPRITE_H,
                          true,
                          false,
                          TWODFX_SPRITE_X_DOUBLE | TWODFX_SPRITE_Y_DOUBLE);
    }
}

/* ------------------------------------------------------------------------- */
/* Stage 1: transform matrix                                                 */
/* ------------------------------------------------------------------------- */

static void build_transform_stage(void)
{
    begin_stage(EGG_CC_STAGE1_COLOUR);
    twodfx_upload_raw(EGG_CC_ARROW_ADDR,
                      egg_CC_transform_arrow_raw,
                      sizeof(egg_CC_transform_arrow_raw));

    for (uint8_t i = 0u; i < EGG_CC_TRANSFORM_COUNT; ++i) {
        uint16_t x = (uint16_t)(EGG_CC_TRANSFORM_FIRST_X +
                                (i % EGG_CC_TRANSFORM_COLUMNS) *
                                    EGG_CC_TRANSFORM_STEP_X);
        uint16_t y = (uint16_t)(EGG_CC_TRANSFORM_FIRST_Y +
                                (i / EGG_CC_TRANSFORM_COLUMNS) *
                                    EGG_CC_TRANSFORM_STEP_Y);

        twodfx_sprite_set(i,
                          EGG_CC_ARROW_ADDR,
                          x,
                          y,
                          EGG_CC_ARROW_W,
                          EGG_CC_ARROW_H,
                          true,
                          false,
                          g_transform_flags[i]);
    }
}

/* ------------------------------------------------------------------------- */
/* Stage 2: clipping                                                         */
/* ------------------------------------------------------------------------- */

static void build_clipping_stage(void)
{
    begin_stage(EGG_CC_STAGE2_COLOUR);
    twodfx_upload_raw(EGG_CC_CLIP_ADDR,
                      egg_CC_clipping_sprite_raw,
                      sizeof(egg_CC_clipping_sprite_raw));

    twodfx_sprite_set(0u, EGG_CC_CLIP_ADDR,
                      EGG_CC_CLIP_LEFT_X, EGG_CC_CLIP_LEFT_Y,
                      EGG_CC_CLIP_SPRITE_W, EGG_CC_CLIP_SPRITE_H,
                      true, false, 0u);
    twodfx_sprite_set(1u, EGG_CC_CLIP_ADDR,
                      EGG_CC_CLIP_RIGHT_X, EGG_CC_CLIP_RIGHT_Y,
                      EGG_CC_CLIP_SPRITE_W, EGG_CC_CLIP_SPRITE_H,
                      true, false, 0u);
    twodfx_sprite_set(2u, EGG_CC_CLIP_ADDR,
                      EGG_CC_CLIP_TOP_X, EGG_CC_CLIP_TOP_Y,
                      EGG_CC_CLIP_SPRITE_W, EGG_CC_CLIP_SPRITE_H,
                      true, false, 0u);
    twodfx_sprite_set(3u, EGG_CC_CLIP_ADDR,
                      EGG_CC_CLIP_BOTTOM_X, EGG_CC_CLIP_BOTTOM_Y,
                      EGG_CC_CLIP_SPRITE_W, EGG_CC_CLIP_SPRITE_H,
                      true, false, 0u);
    twodfx_sprite_set(4u, EGG_CC_CLIP_ADDR,
                      EGG_CC_CLIP_DOUBLE_X, EGG_CC_CLIP_DOUBLE_Y,
                      EGG_CC_CLIP_SPRITE_W, EGG_CC_CLIP_SPRITE_H,
                      true, false, TWODFX_SPRITE_X_DOUBLE);

    for (uint8_t i = 0u; i < 4u; ++i) {
        upload_stage2_cache_asset(i, g_cache_resources[i]);

        twodfx_sprite_set((uint8_t)(8u + i),
                          g_cache_resources[i],
                          (uint16_t)(EGG_CC_CLIP_CACHE_FIRST_X +
                                     i * EGG_CC_CLIP_CACHE_STEP_X),
                          EGG_CC_CLIP_CACHE_Y,
                          EGG_CC_CLIP_CACHE_W,
                          EGG_CC_CLIP_CACHE_H,
                          true,
                          false,
                          TWODFX_SPRITE_X_DOUBLE | TWODFX_SPRITE_Y_DOUBLE);
    }
}

/* ------------------------------------------------------------------------- */
/* Stage 3: mixed resources                                                  */
/* ------------------------------------------------------------------------- */

static void build_mixed_stage(void)
{
    uint8_t slot = 0u;

    twodfx_reset_sprites();
    twodfx_reset_2dpt_all();
    twodfx_upload_raw(EGG_CC_BLIT_ADDR,
                      egg_CC_blit_bitmap_raw,
                      sizeof(egg_CC_blit_bitmap_raw));
    twodfx_upload_raw(EGG_CC_PATTERN_ADDR,
                      egg_CC_pattern_bank_raw,
                      sizeof(egg_CC_pattern_bank_raw));
    twodfx_upload_raw(EGG_CC_TILESET_ADDR,
                      egg_CC_tileset_raw,
                      sizeof(egg_CC_tileset_raw));
    twodfx_upload_raw(EGG_CC_TILEMAP_ADDR,
                      egg_CC_tilemap_raw,
                      sizeof(egg_CC_tilemap_raw));
    twodfx_upload_raw(EGG_CC_MIX_SPR_ADDR,
                      egg_CC_mixed_sprite_raw,
                      sizeof(egg_CC_mixed_sprite_raw));

    twodfx_define_bitmap(EGG_CC_BITMAP_ID,
                         EGG_CC_BLIT_ADDR,
                         EGG_CC_BLIT_W,
                         EGG_CC_BLIT_H);
    twodfx_define_pattern(EGG_CC_PATTERN_ID,
                          EGG_CC_PATTERN_ADDR);
    twodfx_define_tileset(EGG_CC_TILESET_ID,
                          EGG_CC_TILESET_ADDR);
    twodfx_define_tilemap(EGG_CC_TILEMAP_ID,
                          EGG_CC_TILEMAP_ADDR,
                          EGG_CC_TILEMAP_COLUMNS,
                          EGG_CC_TILEMAP_ROWS,
                          EGG_CC_TILESET_ID);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                        background_colour(),
                        EGG_CC_TRANSPARENT_COLOUR);
    twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND, slot++,
                        EGG_CC_FRAME_X,
                        EGG_CC_FRAME_Y,
                        g_frame_width,
                        g_frame_height);

    twodfx_2d_pattern_fill(TWODFX_LAYER_BACKGROUND, slot++,
                           EGG_CC_PATTERN0_X,
                           EGG_CC_PATTERN0_Y,
                           EGG_CC_PATTERN0_W,
                           EGG_CC_PATTERN0_H,
                           EGG_CC_PATTERN_ID,
                           0u);
    twodfx_2d_pattern_fill(TWODFX_LAYER_BACKGROUND, slot++,
                           EGG_CC_PATTERN1_X,
                           EGG_CC_PATTERN1_Y,
                           EGG_CC_PATTERN1_W,
                           EGG_CC_PATTERN1_H,
                           EGG_CC_PATTERN_ID,
                           1u);

    twodfx_2d_tilemap(TWODFX_LAYER_BACKGROUND, slot++,
                      EGG_CC_TILEMAP_X,
                      EGG_CC_TILEMAP_Y,
                      EGG_CC_TILEMAP_ID,
                      0u,
                      0u,
                      EGG_CC_TILEMAP_COLUMNS,
                      EGG_CC_TILEMAP_ROWS);

    twodfx_2d_blit(TWODFX_LAYER_BACKGROUND, slot++,
                   EGG_CC_BLIT0_X,
                   EGG_CC_BLIT0_Y,
                   EGG_CC_BITMAP_ID,
                   TWODFX_BLIT_EXACT);
    twodfx_2d_blit(TWODFX_LAYER_BACKGROUND, slot++,
                   EGG_CC_BLIT1_X,
                   EGG_CC_BLIT1_Y,
                   EGG_CC_BITMAP_ID,
                   TWODFX_BLIT_EXACT);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                        EGG_CC_FRAME_COLOUR,
                        EGG_CC_TRANSPARENT_COLOUR);
    twodfx_2d_rect(TWODFX_LAYER_BACKGROUND, slot++,
                   EGG_CC_FRAME_X,
                   EGG_CC_FRAME_Y,
                   g_frame_width,
                   g_frame_height,
                   2u);
    twodfx_2d_ret(TWODFX_LAYER_BACKGROUND, slot);

    twodfx_sprite_set(0u,
                      EGG_CC_MIX_SPR_ADDR,
                      EGG_CC_MIX_SPRITE0_X,
                      EGG_CC_MIX_SPRITE0_Y,
                      EGG_CC_MIX_SPRITE_W,
                      EGG_CC_MIX_SPRITE_H,
                      true,
                      false,
                      0u);
    twodfx_sprite_set(1u,
                      EGG_CC_MIX_SPR_ADDR,
                      EGG_CC_MIX_SPRITE1_X,
                      EGG_CC_MIX_SPRITE1_Y,
                      EGG_CC_MIX_SPRITE_W,
                      EGG_CC_MIX_SPRITE_H,
                      true,
                      false,
                      TWODFX_SPRITE_X_DOUBLE | TWODFX_SPRITE_Y_DOUBLE);
}

/* ------------------------------------------------------------------------- */
/* Stage 4: asynchronous raw ZX1 smoke test                                  */
/* ------------------------------------------------------------------------- */

static void build_zx1_stage(void)
{
    begin_stage(EGG_CC_STAGE4_COLOUR);

    twodfx_upload_raw(EGG_CC_ZX1_SRC_ADDR,
                      egg_CC_diag_zx1_sprite_stream,
                      sizeof(egg_CC_diag_zx1_sprite_stream));
    twodfx_rram_fill(EGG_CC_ZX1_DST_ADDR,
                     0u,
                     EGG_CC_DIAG_ZX1_UNPACKED_BYTES);

    if (!g_zx1_queued) {
        (void)twodfx_unzx1_raw(EGG_CC_ZX1_SRC_ADDR,
                               EGG_CC_ZX1_DST_ADDR);
        g_zx1_queued = true;
    }

    /* The destination sprite becomes visible after the job completes. */
    twodfx_sprite_set(0u,
                      EGG_CC_ZX1_DST_ADDR,
                      EGG_CC_ZX1_SPRITE_X,
                      EGG_CC_ZX1_SPRITE_Y,
                      EGG_CC_DIAG_ZX1_SPRITE_W,
                      EGG_CC_DIAG_ZX1_SPRITE_H,
                      true,
                      false,
                      TWODFX_SPRITE_X_DOUBLE | TWODFX_SPRITE_Y_DOUBLE);
}

/* ------------------------------------------------------------------------- */
/* Stage selection and lifecycle                                             */
/* ------------------------------------------------------------------------- */

static void build_stage(void)
{
    switch (g_stage) {
        case EGG_CC_STAGE_ADDRESS_AND_CACHE:
            build_address_stage();
            break;

        case EGG_CC_STAGE_TRANSFORMS:
            build_transform_stage();
            break;

        case EGG_CC_STAGE_CLIPPING:
            build_clipping_stage();
            break;

        case EGG_CC_STAGE_MIXED_RESOURCES:
            build_mixed_stage();
            break;

        default:
            build_zx1_stage();
            break;
    }
}

void egg_CC_diag_start(void)
{
    g_is_tvc = twodfx_get_machine() == TWODFX_MACHINE_TVC;

    if (g_is_tvc) {
        g_frame_width = EGG_CC_TVC_FRAME_WIDTH;
        g_frame_height = EGG_CC_TVC_FRAME_HEIGHT;
    } else {
        g_frame_width = EGG_CC_EP_FRAME_WIDTH;
        g_frame_height = EGG_CC_EP_FRAME_HEIGHT;
    }

    g_stage = EGG_CC_STAGE_ADDRESS_AND_CACHE;
    g_stage_tick = 0u;
    g_zx1_queued = false;
    build_stage();
}

void egg_CC_diag_tick(void)
{
    ++g_stage_tick;

    if (g_stage_tick >= EGG_CC_STAGE_TICKS) {
        g_stage_tick = 0u;
        g_stage = (egg_CC_stage_t)(((uint8_t)g_stage + 1u) % EGG_CC_STAGE_COUNT);
        g_zx1_queued = false;
        build_stage();
    }
}
