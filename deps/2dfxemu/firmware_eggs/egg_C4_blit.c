#include "egg_C4_blit.h"
#include "egg_C4_blit_assets.h"

#include <stdbool.h>
#include <stdint.h>

static bool g_is_tvc;
static egg_C4_asset_load_state_t g_asset_load_state;
static uint16_t g_tick;
static int16_t g_moving_x;
static int16_t g_moving_dx;

/* ------------------------------------------------------------------------- */
/* Active-profile geometry                                                   */
/* ------------------------------------------------------------------------- */

static uint16_t stage_x(void)
{
    return g_is_tvc ? EGG_C4_TVC_STAGE_X : EGG_C4_ENTERPRISE_STAGE_X;
}

static uint16_t stage_y(void)
{
    return g_is_tvc ? EGG_C4_TVC_STAGE_Y : EGG_C4_ENTERPRISE_STAGE_Y;
}

static uint16_t stage_width(void)
{
    return g_is_tvc ? EGG_C4_TVC_STAGE_W : EGG_C4_ENTERPRISE_STAGE_W;
}

static uint16_t stage_height(void)
{
    return g_is_tvc ? EGG_C4_TVC_STAGE_H : EGG_C4_ENTERPRISE_STAGE_H;
}

static uint8_t profile_colour(uint8_t enterprise_colour, uint8_t tvc_colour)
{
    return g_is_tvc ? tvc_colour : enterprise_colour;
}

/* ------------------------------------------------------------------------- */
/* Profile-specific offline asset installation                               */
/* ------------------------------------------------------------------------- */

static bool queue_object_decompression(egg_C4_asset_load_state_t state)
{
    const uint8_t *stream;
    uint32_t stream_size;
    uint32_t destination;

    switch (state) {
        case EGG_C4_ASSET_LOAD_OBJECT0:
            stream = g_is_tvc
                ? egg_C4_object0_tvc_animation_zx1
                : egg_C4_object0_enterprise_animation_zx1;
            stream_size = g_is_tvc
                ? sizeof(egg_C4_object0_tvc_animation_zx1)
                : sizeof(egg_C4_object0_enterprise_animation_zx1);
            destination = EGG_C4_OBJECT0_BANK_ADDRESS;
            break;

        case EGG_C4_ASSET_LOAD_OBJECT1:
            stream = g_is_tvc
                ? egg_C4_object1_tvc_animation_zx1
                : egg_C4_object1_enterprise_animation_zx1;
            stream_size = g_is_tvc
                ? sizeof(egg_C4_object1_tvc_animation_zx1)
                : sizeof(egg_C4_object1_enterprise_animation_zx1);
            destination = EGG_C4_OBJECT1_BANK_ADDRESS;
            break;

        case EGG_C4_ASSET_LOAD_OBJECT2:
            stream = g_is_tvc
                ? egg_C4_object2_tvc_animation_zx1
                : egg_C4_object2_enterprise_animation_zx1;
            stream_size = g_is_tvc
                ? sizeof(egg_C4_object2_tvc_animation_zx1)
                : sizeof(egg_C4_object2_enterprise_animation_zx1);
            destination = EGG_C4_OBJECT2_BANK_ADDRESS;
            break;

        case EGG_C4_ASSET_LOAD_OBJECT3:
            stream = g_is_tvc
                ? egg_C4_object3_tvc_animation_zx1
                : egg_C4_object3_enterprise_animation_zx1;
            stream_size = g_is_tvc
                ? sizeof(egg_C4_object3_tvc_animation_zx1)
                : sizeof(egg_C4_object3_enterprise_animation_zx1);
            destination = EGG_C4_OBJECT3_BANK_ADDRESS;
            break;

        default:
            return false;
    }

    twodfx_upload_raw(EGG_C4_ZX1_STAGING_ADDRESS,
                      stream,
                      stream_size);

    return twodfx_unzx1_raw(EGG_C4_ZX1_STAGING_ADDRESS,
                            destination);
}

/* ------------------------------------------------------------------------- */
/* Public-API scene builder                                                  */
/* ------------------------------------------------------------------------- */

static void build_scene(void)
{
    uint8_t slot = 0u;
    const uint16_t x = stage_x();
    const uint16_t y = stage_y();
    const uint16_t width = stage_width();
    const uint16_t height = stage_height();

    twodfx_reset_2dpt_all();

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                        profile_colour(EGG_C4_STAGE_FILL_COLOR, EGG_C4_TVC_STAGE_FILL_COLOR),
                        profile_colour(EGG_C4_STAGE_FILL_BACKGROUND, EGG_C4_TVC_STAGE_FILL_BACKGROUND));
    twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND, slot++,
                        x, y, width, height);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                        profile_colour(EGG_C4_STAGE_BORDER_COLOR, EGG_C4_TVC_STAGE_BORDER_COLOR),
                        EGG_C4_TRANSPARENT_COLOR);
    twodfx_2d_rect(TWODFX_LAYER_BACKGROUND, slot++,
                   x, y, width, height,
                   EGG_C4_STAGE_BORDER_STROKE);

    for (uint8_t line = 0u; line < EGG_C4_STAGE_LINE_COUNT; ++line) {
        uint16_t y0 = (uint16_t)(
            y + EGG_C4_STAGE_LINE_TOP_OFFSET
            + (uint16_t)line * EGG_C4_STAGE_LINE_TOP_STEP);
        uint16_t y1 = (uint16_t)(
            y + height - EGG_C4_STAGE_LINE_BOTTOM_INSET
            - (uint16_t)line * EGG_C4_STAGE_LINE_BOTTOM_STEP);

        /* Preserve the original command sequence: every line has its own
         * immediately preceding SET_COLOR slot. */
        twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                            profile_colour(EGG_C4_STAGE_LINE_COLOR, EGG_C4_TVC_STAGE_LINE_COLOR),
                            EGG_C4_TRANSPARENT_COLOR);
        twodfx_2d_line(TWODFX_LAYER_BACKGROUND, slot++,
                       (uint16_t)(x + EGG_C4_STAGE_LINE_LEFT_INSET), y0,
                       (uint16_t)(x + width - EGG_C4_STAGE_LINE_LEFT_INSET), y1,
                       EGG_C4_STAGE_LINE_STROKE);
    }

    if (g_is_tvc) {
        const uint16_t x0 = (uint16_t)(x + EGG_C4_TVC_COLUMN0_OFFSET);
        const uint16_t x1 = (uint16_t)(x + EGG_C4_TVC_COLUMN1_OFFSET);
        const uint16_t x2 = (uint16_t)(x + EGG_C4_TVC_COLUMN2_OFFSET);

        twodfx_2d_blit(TWODFX_LAYER_BACKGROUND, slot++,
                       x0, (uint16_t)(y + EGG_C4_TVC_EXACT_ROW_OFFSET),
                       EGG_C4_OBJECT0_BITMAP_ID, TWODFX_BLIT_EXACT);
        twodfx_2d_blit(TWODFX_LAYER_BACKGROUND, slot++,
                       x1, (uint16_t)(y + EGG_C4_TVC_EXACT_ROW_OFFSET),
                       EGG_C4_OBJECT1_BITMAP_ID, TWODFX_BLIT_EXACT);
        twodfx_2d_blit(TWODFX_LAYER_BACKGROUND, slot++,
                       x2, (uint16_t)(y + EGG_C4_TVC_EXACT_ROW_OFFSET),
                       EGG_C4_OBJECT2_BITMAP_ID, TWODFX_BLIT_EXACT);

        twodfx_2d_blit(TWODFX_LAYER_BACKGROUND, slot++,
                       x0, (uint16_t)(y + EGG_C4_TVC_TRANSPARENT_ROW_OFFSET),
                       EGG_C4_OBJECT0_BITMAP_ID, TWODFX_BLIT_TRANSPARENT);
        twodfx_2d_blit(TWODFX_LAYER_BACKGROUND, slot++,
                       x1, (uint16_t)(y + EGG_C4_TVC_TRANSPARENT_ROW_OFFSET),
                       EGG_C4_OBJECT1_BITMAP_ID, TWODFX_BLIT_TRANSPARENT);
        twodfx_2d_blit(TWODFX_LAYER_BACKGROUND, slot++,
                       x2, (uint16_t)(y + EGG_C4_TVC_TRANSPARENT_ROW_OFFSET),
                       EGG_C4_OBJECT2_BITMAP_ID, TWODFX_BLIT_TRANSPARENT);

        twodfx_2d_blit(TWODFX_LAYER_BACKGROUND, slot++,
                       (uint16_t)g_moving_x,
                       (uint16_t)(y + EGG_C4_TVC_MOVING_ROW_OFFSET),
                       EGG_C4_OBJECT3_BITMAP_ID, TWODFX_BLIT_TRANSPARENT);
    } else {
        twodfx_2d_blit(TWODFX_LAYER_BACKGROUND, slot++,
                       (uint16_t)(x + EGG_C4_ENTERPRISE_EXACT0_X_OFFSET),
                       (uint16_t)(y + EGG_C4_ENTERPRISE_EXACT_ROW_OFFSET),
                       EGG_C4_OBJECT0_BITMAP_ID, TWODFX_BLIT_EXACT);
        twodfx_2d_blit(TWODFX_LAYER_BACKGROUND, slot++,
                       (uint16_t)(x + EGG_C4_ENTERPRISE_EXACT1_X_OFFSET),
                       (uint16_t)(y + EGG_C4_ENTERPRISE_EXACT_ROW_OFFSET),
                       EGG_C4_OBJECT1_BITMAP_ID, TWODFX_BLIT_EXACT);
        twodfx_2d_blit(TWODFX_LAYER_BACKGROUND, slot++,
                       (uint16_t)(x + EGG_C4_ENTERPRISE_EXACT2_X_OFFSET),
                       (uint16_t)(y + EGG_C4_ENTERPRISE_EXACT_ROW_OFFSET),
                       EGG_C4_OBJECT2_BITMAP_ID, TWODFX_BLIT_EXACT);
        twodfx_2d_blit(TWODFX_LAYER_BACKGROUND, slot++,
                       (uint16_t)(x + EGG_C4_ENTERPRISE_EXACT3_X_OFFSET),
                       (uint16_t)(y + EGG_C4_ENTERPRISE_EXACT_ROW_OFFSET),
                       EGG_C4_OBJECT3_BITMAP_ID, TWODFX_BLIT_EXACT);

        twodfx_2d_blit(TWODFX_LAYER_BACKGROUND, slot++,
                       (uint16_t)(x + EGG_C4_ENTERPRISE_TRANS0_X_OFFSET),
                       (uint16_t)(y + EGG_C4_ENTERPRISE_TRANS_ROW_OFFSET),
                       EGG_C4_OBJECT0_BITMAP_ID, TWODFX_BLIT_TRANSPARENT);
        twodfx_2d_blit(TWODFX_LAYER_BACKGROUND, slot++,
                       (uint16_t)(x + EGG_C4_ENTERPRISE_TRANS1_X_OFFSET),
                       (uint16_t)(y + EGG_C4_ENTERPRISE_TRANS_ROW_OFFSET),
                       EGG_C4_OBJECT1_BITMAP_ID, TWODFX_BLIT_TRANSPARENT);
        twodfx_2d_blit(TWODFX_LAYER_BACKGROUND, slot++,
                       (uint16_t)(x + EGG_C4_ENTERPRISE_TRANS2_X_OFFSET),
                       (uint16_t)(y + EGG_C4_ENTERPRISE_TRANS_ROW_OFFSET),
                       EGG_C4_OBJECT2_BITMAP_ID, TWODFX_BLIT_TRANSPARENT);

        twodfx_2d_blit(TWODFX_LAYER_BACKGROUND, slot++,
                       (uint16_t)g_moving_x,
                       (uint16_t)(y + EGG_C4_ENTERPRISE_MOVING_ROW_OFFSET),
                       EGG_C4_OBJECT3_BITMAP_ID, TWODFX_BLIT_TRANSPARENT);
    }

    uint16_t marker_x = (uint16_t)(
        x + EGG_C4_MARKER_LEFT_INSET
        + ((g_tick * EGG_C4_MARKER_SPEED)
           % (width - EGG_C4_MARKER_TRAVEL_MARGIN)));

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                        profile_colour(EGG_C4_MARKER_COLOR, EGG_C4_TVC_MARKER_COLOR),
                        EGG_C4_TRANSPARENT_COLOR);
    twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND, slot++,
                        marker_x,
                        (uint16_t)(y + height - EGG_C4_MARKER_BOTTOM_INSET),
                        EGG_C4_MARKER_WIDTH,
                        EGG_C4_MARKER_HEIGHT);

    twodfx_2d_ret(TWODFX_LAYER_BACKGROUND, slot);
}

static uint32_t current_frame_address(uint32_t bank_address)
{
    const uint32_t frame =
        (g_tick >> EGG_C4_ANIMATION_TICK_SHIFT)
        & EGG_C4_ANIMATION_FRAME_MASK;

    return bank_address + frame * EGG_C4_BITMAP_BYTES;
}

/* Advance the animation by changing descriptor addresses only. All 64 frames
 * were generated offline and already reside in RRAM, so no bitmap bytes are
 * generated, uploaded or decompressed while the showcase is visible. */
static void define_current_animation_frames(void)
{
    twodfx_define_bitmap(EGG_C4_OBJECT0_BITMAP_ID,
                         current_frame_address(EGG_C4_OBJECT0_BANK_ADDRESS),
                         EGG_C4_BITMAP_WIDTH,
                         EGG_C4_BITMAP_HEIGHT);
    twodfx_define_bitmap(EGG_C4_OBJECT1_BITMAP_ID,
                         current_frame_address(EGG_C4_OBJECT1_BANK_ADDRESS),
                         EGG_C4_BITMAP_WIDTH,
                         EGG_C4_BITMAP_HEIGHT);
    twodfx_define_bitmap(EGG_C4_OBJECT2_BITMAP_ID,
                         current_frame_address(EGG_C4_OBJECT2_BANK_ADDRESS),
                         EGG_C4_BITMAP_WIDTH,
                         EGG_C4_BITMAP_HEIGHT);
    twodfx_define_bitmap(EGG_C4_OBJECT3_BITMAP_ID,
                         current_frame_address(EGG_C4_OBJECT3_BANK_ADDRESS),
                         EGG_C4_BITMAP_WIDTH,
                         EGG_C4_BITMAP_HEIGHT);
}

static void begin_visible_showcase(void)
{
    define_current_animation_frames();
    build_scene();
}

static void service_asset_loading(void)
{
    twodfx_resource_job_status_t status = twodfx_resource_job_status();

    if (status == TWODFX_RESOURCE_JOB_BUSY) {
        return;
    }

    if (status == TWODFX_RESOURCE_JOB_ERROR) {
        g_asset_load_state = EGG_C4_ASSET_ERROR;
        return;
    }

    if (g_asset_load_state < EGG_C4_ASSET_LOAD_OBJECT3) {
        g_asset_load_state =
            (egg_C4_asset_load_state_t)(g_asset_load_state + 1);

        if (!queue_object_decompression(g_asset_load_state)) {
            g_asset_load_state = EGG_C4_ASSET_ERROR;
        }
        return;
    }

    if (g_asset_load_state == EGG_C4_ASSET_LOAD_OBJECT3) {
        g_asset_load_state = EGG_C4_ASSET_READY;
        begin_visible_showcase();
    }
}

void egg_C4_blit_start(void)
{
    g_is_tvc = twodfx_get_machine() == TWODFX_MACHINE_TVC;

    twodfx_reset_sprites();
    twodfx_reset_2dpt_all();
    twodfx_set_transparent_color(EGG_C4_TRANSPARENT_COLOR);

    g_tick = 0u;
    g_moving_x = (int16_t)(stage_x() + EGG_C4_MOVING_MIN_INSET);
    g_moving_dx = EGG_C4_MOVING_INITIAL_DX;

    g_asset_load_state = EGG_C4_ASSET_LOAD_OBJECT0;
    if (!queue_object_decompression(g_asset_load_state)) {
        g_asset_load_state = EGG_C4_ASSET_ERROR;
    }
}

void egg_C4_blit_tick(void)
{
    if (g_asset_load_state != EGG_C4_ASSET_READY) {
        service_asset_loading();
        return;
    }

    ++g_tick;
    g_moving_x += g_moving_dx;

    const int16_t minimum_x =
        (int16_t)(stage_x() + EGG_C4_MOVING_MIN_INSET);
    const int16_t maximum_x = (int16_t)(
        stage_x() + stage_width()
        - EGG_C4_BITMAP_WIDTH
        - EGG_C4_MOVING_MIN_INSET);

    if (g_moving_x < minimum_x) {
        g_moving_x = minimum_x;
        g_moving_dx = EGG_C4_MOVING_BOUNCE_SPEED;
    }

    if (g_moving_x > maximum_x) {
        g_moving_x = maximum_x;
        g_moving_dx = -EGG_C4_MOVING_BOUNCE_SPEED;
    }

    define_current_animation_frames();
    build_scene();
}
