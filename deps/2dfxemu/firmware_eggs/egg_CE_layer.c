#include "egg_CE_layer.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* ------------------------------------------------------------------------- */
/* Runtime state                                                             */
/* ------------------------------------------------------------------------- */

static bool g_is_tvc;

static uint8_t background_fill_colour(void)
{
    return g_is_tvc
        ? EGG_CE_TVC_BACKGROUND_FILL_COLOUR
        : EGG_CE_EP_BACKGROUND_FILL_COLOUR;
}

static uint8_t outer_frame_colour(void)
{
    return g_is_tvc
        ? EGG_CE_TVC_OUTER_FRAME_COLOUR
        : EGG_CE_EP_OUTER_FRAME_COLOUR;
}

static uint16_t stage_x(void)
{
    return g_is_tvc ? EGG_CE_TVC_STAGE_X : EGG_CE_EP_STAGE_X;
}

static uint16_t stage_y(void)
{
    return g_is_tvc ? EGG_CE_TVC_STAGE_Y : EGG_CE_EP_STAGE_Y;
}

static uint16_t stage_width(void)
{
    return g_is_tvc ? EGG_CE_TVC_STAGE_WIDTH : EGG_CE_EP_STAGE_WIDTH;
}

static uint16_t stage_height(void)
{
    return g_is_tvc ? EGG_CE_TVC_STAGE_HEIGHT : EGG_CE_EP_STAGE_HEIGHT;
}
static uint16_t g_tick;
static int16_t g_x[EGG_CE_SPRITE_COUNT];
static int16_t g_y[EGG_CE_SPRITE_COUNT];
static int16_t g_velocity_x[EGG_CE_SPRITE_COUNT];
static int16_t g_velocity_y[EGG_CE_SPRITE_COUNT];
static uint8_t g_sprite_pixels[EGG_CE_SPRITE_BYTES];

static uint8_t sprite_body_colour(uint8_t sprite)
{
    if (g_is_tvc) {
        switch (sprite) {
            case 0u: return EGG_CE_TVC_SPRITE0_BODY_COLOUR;
            case 1u: return EGG_CE_TVC_SPRITE1_BODY_COLOUR;
            case 2u: return EGG_CE_TVC_SPRITE2_BODY_COLOUR;
            default: return EGG_CE_TVC_SPRITE3_BODY_COLOUR;
        }
    }

    switch (sprite) {
        case 0u: return EGG_CE_EP_SPRITE0_BODY_COLOUR;
        case 1u: return EGG_CE_EP_SPRITE1_BODY_COLOUR;
        case 2u: return EGG_CE_EP_SPRITE2_BODY_COLOUR;
        default: return EGG_CE_EP_SPRITE3_BODY_COLOUR;
    }
}

static uint8_t sprite_accent_colour(uint8_t sprite)
{
    if (g_is_tvc) {
        switch (sprite) {
            case 0u: return EGG_CE_TVC_SPRITE0_ACCENT_COLOUR;
            case 1u: return EGG_CE_TVC_SPRITE1_ACCENT_COLOUR;
            case 2u: return EGG_CE_TVC_SPRITE2_ACCENT_COLOUR;
            default: return EGG_CE_TVC_SPRITE3_ACCENT_COLOUR;
        }
    }

    switch (sprite) {
        case 0u: return EGG_CE_EP_SPRITE0_ACCENT_COLOUR;
        case 1u: return EGG_CE_EP_SPRITE1_ACCENT_COLOUR;
        case 2u: return EGG_CE_EP_SPRITE2_ACCENT_COLOUR;
        default: return EGG_CE_EP_SPRITE3_ACCENT_COLOUR;
    }
}

/* ------------------------------------------------------------------------- */
/* Procedural sprite resources                                               */
/* ------------------------------------------------------------------------- */

static void write_sprite_pixel(uint8_t x, uint8_t y, uint8_t colour)
{
    uint16_t address =
        (uint16_t)y * EGG_CE_SPRITE_ROW_BYTES + (uint16_t)(x >> 1);
    uint8_t packed = g_sprite_pixels[address];

    if ((x & 1u) == 0u) {
        packed = (uint8_t)((packed & 0xF0u) | (colour & 0x0Fu));
    } else {
        packed = (uint8_t)((packed & 0x0Fu) | ((colour & 0x0Fu) << 4));
    }

    g_sprite_pixels[address] = packed;
}

static void generate_sprite(uint8_t sprite)
{
    uint8_t body_colour = sprite_body_colour(sprite);
    uint8_t accent_colour = sprite_accent_colour(sprite);

    memset(g_sprite_pixels, 0, sizeof(g_sprite_pixels));

    for (uint8_t y = 0u; y < EGG_CE_SPRITE_HEIGHT; ++y) {
        for (uint8_t x = 0u; x < EGG_CE_SPRITE_WIDTH; ++x) {
            uint8_t colour = EGG_CE_TRANSPARENT_COLOUR;

            /* Rounded-ish body and accent outline. */
            if (x >= EGG_CE_FACE_MIN_X && x <= EGG_CE_FACE_MAX_X &&
                y >= EGG_CE_FACE_MIN_Y && y <= EGG_CE_FACE_MAX_Y) {
                colour = body_colour;

                if (x == EGG_CE_FACE_MIN_X || x == EGG_CE_FACE_MAX_X ||
                    y == EGG_CE_FACE_MIN_Y || y == EGG_CE_FACE_MAX_Y) {
                    colour = accent_colour;
                }

                if (((x + y + sprite * EGG_CE_ACCENT_PHASE_STEP) &
                     EGG_CE_ACCENT_PATTERN_MASK) == 0u) {
                    colour = accent_colour;
                }
            }

            /* Transparent facial details make sprite movement easy to see. */
            if ((x == EGG_CE_LEFT_EYE_X || x == EGG_CE_RIGHT_EYE_X) &&
                y >= EGG_CE_EYE_MIN_Y && y <= EGG_CE_EYE_MAX_Y) {
                colour = EGG_CE_TRANSPARENT_COLOUR;
            }

            if (x >= EGG_CE_MOUTH_MIN_X && x <= EGG_CE_MOUTH_MAX_X &&
                y == EGG_CE_MOUTH_Y) {
                colour = EGG_CE_TRANSPARENT_COLOUR;
            }

            if ((x == EGG_CE_MOUTH_CORNER0_X ||
                 x == EGG_CE_MOUTH_CORNER1_X) &&
                y == EGG_CE_MOUTH_CORNER_Y) {
                colour = EGG_CE_TRANSPARENT_COLOUR;
            }

            write_sprite_pixel(x, y, colour);
        }
    }
}

static void upload_sprite_assets(void)
{
    for (uint8_t sprite = 0u; sprite < EGG_CE_SPRITE_COUNT; ++sprite) {
        uint32_t address =
            EGG_CE_SPRITE_ADDRESS_BASE +
            (uint32_t)sprite * EGG_CE_SPRITE_BYTES;

        generate_sprite(sprite);
        twodfx_upload_raw(address, g_sprite_pixels, sizeof(g_sprite_pixels));
    }
}

/* ------------------------------------------------------------------------- */
/* Public-API scene playlists                                                */
/* ------------------------------------------------------------------------- */

static void build_background_2dpt(void)
{
    uint8_t slot = 0u;
    const uint16_t sx = stage_x();
    const uint16_t sy = stage_y();
    const uint16_t sw = stage_width();
    const uint16_t sh = stage_height();
    uint8_t horizontal_colour = g_is_tvc
        ? EGG_CE_TVC_HORIZONTAL_GRID_COLOUR
        : EGG_CE_EP_HORIZONTAL_GRID_COLOUR;
    uint8_t vertical_colour = g_is_tvc
        ? EGG_CE_TVC_VERTICAL_GRID_COLOUR
        : EGG_CE_EP_VERTICAL_GRID_COLOUR;
    uint8_t border_colour = g_is_tvc
        ? EGG_CE_TVC_STAGE_BORDER_COLOUR
        : EGG_CE_EP_STAGE_BORDER_COLOUR;

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                        background_fill_colour(),
                        EGG_CE_TRANSPARENT_COLOUR);
    twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND, slot++,
                        sx, sy,
                        sw, sh);

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                        horizontal_colour,
                        EGG_CE_TRANSPARENT_COLOUR);
    for (uint16_t y = sy + EGG_CE_HORIZONTAL_GRID_FIRST_OFFSET_Y;
         y < sy + sh;
         y = (uint16_t)(y + EGG_CE_HORIZONTAL_GRID_STEP_Y)) {
        twodfx_2d_line(TWODFX_LAYER_BACKGROUND, slot++,
                       sx, y,
                       sx + sw - 1u, y,
                       EGG_CE_HORIZONTAL_GRID_STROKE);
    }

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                        vertical_colour,
                        EGG_CE_TRANSPARENT_COLOUR);
    for (uint16_t x = sx + EGG_CE_VERTICAL_GRID_FIRST_OFFSET_X;
         x < sx + sw;
         x = (uint16_t)(x + EGG_CE_VERTICAL_GRID_STEP_X)) {
        twodfx_2d_line(TWODFX_LAYER_BACKGROUND, slot++,
                       x, sy,
                       x, sy + sh - 1u,
                       EGG_CE_VERTICAL_GRID_STROKE);
    }

    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                        border_colour,
                        EGG_CE_TRANSPARENT_COLOUR);
    twodfx_2d_rect(TWODFX_LAYER_BACKGROUND, slot++,
                   sx, sy,
                   sw, sh,
                   EGG_CE_STAGE_BORDER_STROKE);

    twodfx_2d_ret(TWODFX_LAYER_BACKGROUND, slot);
}

static void build_foreground_2dpt(void)
{
    uint8_t slot = 0u;
    const uint16_t sx = stage_x();
    const uint16_t sy = stage_y();
    const uint16_t sw = stage_width();
    const uint16_t sh = stage_height();
    uint16_t phase =
        (uint16_t)((g_tick >> EGG_CE_CAGE_PHASE_SHIFT) &
                   EGG_CE_CAGE_PHASE_MASK);
    uint8_t cage_colour = g_is_tvc
        ? EGG_CE_TVC_CAGE_COLOUR
        : EGG_CE_EP_CAGE_COLOUR;
    uint8_t top_hud_colour = g_is_tvc
        ? EGG_CE_TVC_TOP_HUD_COLOUR
        : EGG_CE_EP_TOP_HUD_COLOUR;
    uint8_t bottom_hud_colour = g_is_tvc
        ? EGG_CE_TVC_BOTTOM_HUD_COLOUR
        : EGG_CE_EP_BOTTOM_HUD_COLOUR;

    /*
     * Moving cage bars prove that foreground commands cover real sprites.
     *
     * Start one lattice position before the old first bar and clip every bar
     * to the stage.  This makes the leftmost bar enter progressively through
     * the stage edge when the 0..63 phase wraps, instead of popping into
     * existence at EGG_CE_CAGE_FIRST_OFFSET_X.  Clipping the right edge at
     * the same time also makes the exit symmetrical.
     */
    twodfx_2d_set_color(TWODFX_LAYER_FOREGROUND, slot++,
                        cage_colour,
                        EGG_CE_TRANSPARENT_COLOUR);

    const int32_t cage_min_x = (int32_t)sx;
    const int32_t cage_max_x = (int32_t)sx + (int32_t)sw;
    const int32_t cage_first_x =
        (int32_t)sx + (int32_t)EGG_CE_CAGE_FIRST_OFFSET_X +
        (int32_t)phase - (int32_t)EGG_CE_CAGE_STEP_X;

    for (int32_t x = cage_first_x;
         x < cage_max_x;
         x += (int32_t)EGG_CE_CAGE_STEP_X) {
        int32_t clipped_left = x;
        int32_t clipped_right = x + (int32_t)EGG_CE_CAGE_BAR_WIDTH;

        if (clipped_left < cage_min_x) {
            clipped_left = cage_min_x;
        }
        if (clipped_right > cage_max_x) {
            clipped_right = cage_max_x;
        }

        if (clipped_right > clipped_left) {
            twodfx_2d_fill_rect(
                TWODFX_LAYER_FOREGROUND, slot++,
                (uint16_t)clipped_left,
                sy + EGG_CE_CAGE_VERTICAL_MARGIN,
                (uint16_t)(clipped_right - clipped_left),
                sh - 2u * EGG_CE_CAGE_VERTICAL_MARGIN);
        }
    }

    twodfx_2d_set_color(TWODFX_LAYER_FOREGROUND, slot++,
                        outer_frame_colour(),
                        EGG_CE_TRANSPARENT_COLOUR);
    twodfx_2d_rect(
        TWODFX_LAYER_FOREGROUND, slot++,
        sx - EGG_CE_OUTER_FRAME_MARGIN,
        sy - EGG_CE_OUTER_FRAME_MARGIN,
        sw + 2u * EGG_CE_OUTER_FRAME_MARGIN,
        sh + 2u * EGG_CE_OUTER_FRAME_MARGIN,
        EGG_CE_OUTER_FRAME_STROKE);

    /* HUD strips are also on the foreground layer. */
    twodfx_2d_set_color(TWODFX_LAYER_FOREGROUND, slot++,
                        top_hud_colour,
                        EGG_CE_TRANSPARENT_COLOUR);
    twodfx_2d_fill_rect(
        TWODFX_LAYER_FOREGROUND, slot++,
        sx + EGG_CE_TOP_HUD_OFFSET_X,
        (uint16_t)((int32_t)sy + EGG_CE_TOP_HUD_OFFSET_Y),
        EGG_CE_HUD_WIDTH, EGG_CE_HUD_HEIGHT);

    if (bottom_hud_colour != top_hud_colour) {
        twodfx_2d_set_color(TWODFX_LAYER_FOREGROUND, slot++,
                            bottom_hud_colour,
                            EGG_CE_TRANSPARENT_COLOUR);
    }
    twodfx_2d_fill_rect(
        TWODFX_LAYER_FOREGROUND, slot++,
        sx + sw -
            EGG_CE_HUD_WIDTH - EGG_CE_BOTTOM_HUD_RIGHT_MARGIN,
        sy + sh + EGG_CE_BOTTOM_HUD_OFFSET_Y,
        EGG_CE_HUD_WIDTH, EGG_CE_HUD_HEIGHT);

    twodfx_2d_ret(TWODFX_LAYER_FOREGROUND, slot);
}

/* ------------------------------------------------------------------------- */
/* Sprite motion                                                             */
/* ------------------------------------------------------------------------- */

static void update_sprites(void)
{
    const int16_t min_x = (int16_t)stage_x();
    const int16_t min_y = (int16_t)stage_y();
    const int16_t max_x =
        (int16_t)(stage_x() + stage_width() - EGG_CE_SPRITE_WIDTH);
    const int16_t max_y =
        (int16_t)(stage_y() + stage_height() - EGG_CE_SPRITE_HEIGHT);

    for (uint8_t sprite = 0u; sprite < EGG_CE_SPRITE_COUNT; ++sprite) {
        g_x[sprite] = (int16_t)(g_x[sprite] + g_velocity_x[sprite]);
        g_y[sprite] = (int16_t)(g_y[sprite] + g_velocity_y[sprite]);

        if (g_x[sprite] < min_x) {
            g_x[sprite] = min_x;
            g_velocity_x[sprite] = (int16_t)-g_velocity_x[sprite];
        }
        if (g_y[sprite] < min_y) {
            g_y[sprite] = min_y;
            g_velocity_y[sprite] = (int16_t)-g_velocity_y[sprite];
        }
        if (g_x[sprite] > max_x) {
            g_x[sprite] = max_x;
            g_velocity_x[sprite] = (int16_t)-g_velocity_x[sprite];
        }
        if (g_y[sprite] > max_y) {
            g_y[sprite] = max_y;
            g_velocity_y[sprite] = (int16_t)-g_velocity_y[sprite];
        }

        twodfx_sprite_set(
            sprite,
            EGG_CE_SPRITE_ADDRESS_BASE +
                (uint32_t)sprite * EGG_CE_SPRITE_BYTES,
            (uint16_t)g_x[sprite], (uint16_t)g_y[sprite],
            EGG_CE_SPRITE_WIDTH, EGG_CE_SPRITE_HEIGHT,
            true, false, 0u);
    }
}

static void initialise_motion(void)
{
    g_x[0] = EGG_CE_SPRITE0_INITIAL_X;
    g_y[0] = EGG_CE_SPRITE0_INITIAL_Y;
    g_velocity_x[0] = EGG_CE_SPRITE0_VELOCITY_X;
    g_velocity_y[0] = EGG_CE_SPRITE0_VELOCITY_Y;

    g_x[1] = EGG_CE_SPRITE1_INITIAL_X;
    g_y[1] = EGG_CE_SPRITE1_INITIAL_Y;
    g_velocity_x[1] = EGG_CE_SPRITE1_VELOCITY_X;
    g_velocity_y[1] = EGG_CE_SPRITE1_VELOCITY_Y;

    g_x[2] = EGG_CE_SPRITE2_INITIAL_X;
    g_y[2] = EGG_CE_SPRITE2_INITIAL_Y;
    g_velocity_x[2] = EGG_CE_SPRITE2_VELOCITY_X;
    g_velocity_y[2] = EGG_CE_SPRITE2_VELOCITY_Y;

    g_x[3] = EGG_CE_SPRITE3_INITIAL_X;
    g_y[3] = EGG_CE_SPRITE3_INITIAL_Y;
    g_velocity_x[3] = EGG_CE_SPRITE3_VELOCITY_X;
    g_velocity_y[3] = EGG_CE_SPRITE3_VELOCITY_Y;
}

void egg_CE_layer_start(void)
{
    g_is_tvc = (twodfx_get_machine() == TWODFX_MACHINE_TVC);
    g_tick = 0u;

    twodfx_reset_sprites();
    twodfx_reset_2dpt_all();

    initialise_motion();
    upload_sprite_assets();
    build_background_2dpt();
    build_foreground_2dpt();
    update_sprites();
}

void egg_CE_layer_tick(void)
{
    ++g_tick;
    update_sprites();
    build_foreground_2dpt();
}
