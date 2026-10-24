#include "egg_CD_blit_full.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* ------------------------------------------------------------------------- */
/* Runtime state and small generation buffers                                */
/* ------------------------------------------------------------------------- */

static uint16_t g_stage_tick;
static uint8_t g_stage;
static bool g_is_tvc;

static uint16_t g_bitmap_x;
static uint16_t g_bitmap_y;
static uint16_t g_bitmap_width;
static uint16_t g_bitmap_height;
static uint16_t g_bitmap_row_bytes;

static uint8_t g_bitmap_row[EGG_CD_MAX_ROW_BYTES];
static uint8_t g_smiley_pixels[EGG_CD_SMILEY_BYTES];

static uint8_t pack_pixels(uint8_t left, uint8_t right)
{
    return (uint8_t)((left & 0x0Fu) | ((right & 0x0Fu) << 4));
}

static void select_profile_geometry(void)
{
    g_is_tvc = (twodfx_get_machine() == TWODFX_MACHINE_TVC);

    if (g_is_tvc) {
        g_bitmap_x = EGG_CD_TVC_BITMAP_X;
        g_bitmap_y = EGG_CD_TVC_BITMAP_Y;
        g_bitmap_width = EGG_CD_TVC_BITMAP_WIDTH;
        g_bitmap_height = EGG_CD_TVC_BITMAP_HEIGHT;
        g_bitmap_row_bytes = EGG_CD_TVC_ROW_BYTES;
    } else {
        g_bitmap_x = EGG_CD_EP_BITMAP_X;
        g_bitmap_y = EGG_CD_EP_BITMAP_Y;
        g_bitmap_width = EGG_CD_EP_BITMAP_WIDTH;
        g_bitmap_height = EGG_CD_EP_BITMAP_HEIGHT;
        g_bitmap_row_bytes = EGG_CD_EP_ROW_BYTES;
    }
}

/* ------------------------------------------------------------------------- */
/* Procedural resources                                                      */
/* ------------------------------------------------------------------------- */

static bool inside_rect(uint16_t x, uint16_t y,
                        uint16_t rx, uint16_t ry,
                        uint16_t rw, uint16_t rh)
{
    return x >= rx && x < (uint16_t)(rx + rw) &&
           y >= ry && y < (uint16_t)(ry + rh);
}

static uint8_t mondrian_black(void)
{
    return g_is_tvc ? EGG_CD_TVC_OPAQUE_BLACK_COLOUR
                    : EGG_CD_EP_OPAQUE_BLACK_COLOUR;
}

static uint8_t mondrian_white(void)
{
    return g_is_tvc ? EGG_CD_TVC_WHITE_COLOUR : EGG_CD_EP_WHITE_COLOUR;
}

static uint8_t mondrian_red(void)
{
    return g_is_tvc ? EGG_CD_TVC_RED_COLOUR : EGG_CD_EP_RED_COLOUR;
}

static uint8_t mondrian_yellow(void)
{
    return g_is_tvc ? EGG_CD_TVC_YELLOW_COLOUR : EGG_CD_EP_YELLOW_COLOUR;
}

static uint8_t mondrian_blue(void)
{
    return g_is_tvc ? EGG_CD_TVC_BLUE_COLOUR : EGG_CD_EP_BLUE_COLOUR;
}

static uint8_t mondrian_cyan(void)
{
    return g_is_tvc ? EGG_CD_TVC_CYAN_COLOUR : EGG_CD_EP_CYAN_COLOUR;
}

static uint8_t mondrian_colour(uint16_t x, uint16_t y)
{
    /*
     * Convert stored-bitmap coordinates to the real host-visible rectangle,
     * then scale both profiles into the same 540x230 C1 design space.
     */
    const uint16_t global_x = (uint16_t)(g_bitmap_x + x);
    const uint16_t global_y = (uint16_t)(g_bitmap_y + y);
    const uint16_t min_x = g_is_tvc ? TWODFX_TVC_CRTC_VISIBLE_MIN_X
                                    : TWODFX_ENTERPRISE_NICK_VISIBLE_MIN_X;
    const uint16_t max_x = g_is_tvc ? TWODFX_TVC_CRTC_VISIBLE_MAX_X
                                    : TWODFX_ENTERPRISE_NICK_VISIBLE_MAX_X;
    const uint16_t min_y = g_is_tvc ? TWODFX_TVC_CRTC_VISIBLE_MIN_Y
                                    : TWODFX_ENTERPRISE_NICK_VISIBLE_MIN_Y;
    const uint16_t max_y = g_is_tvc
        ? TWODFX_TVC_CRTC_VISIBLE_MAX_Y
        : (uint16_t)(EGG_CD_EP_BITMAP_Y + EGG_CD_EP_BITMAP_HEIGHT - 1u);

    if (global_x < min_x || global_x > max_x ||
        global_y < min_y || global_y > max_y) {
        return mondrian_black();
    }

    const uint16_t visible_width = (uint16_t)(max_x - min_x + 1u);
    const uint16_t visible_height = (uint16_t)(max_y - min_y + 1u);
    const uint16_t mx = (uint16_t)(
        ((uint32_t)(global_x - min_x) * EGG_CD_C1_MONDRIAN_WIDTH) /
        visible_width);
    const uint16_t my = (uint16_t)(
        ((uint32_t)(global_y - min_y) * EGG_CD_C1_MONDRIAN_HEIGHT) /
        visible_height);
    uint8_t colour = mondrian_white();

    if (inside_rect(mx, my, 0u,   0u,   154u,  62u) ||
        inside_rect(mx, my, 246u, 100u, 126u, 130u)) {
        colour = mondrian_red();
    } else if (inside_rect(mx, my, 372u, 0u,   168u, 86u) ||
               inside_rect(mx, my, 158u, 66u,   88u, 96u)) {
        colour = mondrian_blue();
    } else if (inside_rect(mx, my, 0u,   160u, 122u, 70u) ||
               inside_rect(mx, my, 392u, 132u, 148u, 98u)) {
        colour = mondrian_yellow();
    } else if (inside_rect(mx, my, 122u, 162u, 124u, 68u)) {
        colour = mondrian_cyan();
    }

    static const uint16_t verticals[] = {0u, 154u, 246u, 372u, 392u, 534u};
    static const uint16_t horizontals[] = {0u, 62u, 100u, 132u, 160u, 224u};

    for (uint8_t i = 0u; i < sizeof(verticals) / sizeof(verticals[0]); ++i) {
        if (mx >= verticals[i] &&
            mx < (uint16_t)(verticals[i] + EGG_CD_C1_MONDRIAN_GRID_THICKNESS)) {
            return mondrian_black();
        }
    }
    for (uint8_t i = 0u; i < sizeof(horizontals) / sizeof(horizontals[0]); ++i) {
        if (my >= horizontals[i] &&
            my < (uint16_t)(horizontals[i] + EGG_CD_C1_MONDRIAN_GRID_THICKNESS)) {
            return mondrian_black();
        }
    }

    if (inside_rect(mx, my, 122u, 160u, 6u, 70u) ||
        inside_rect(mx, my, 246u,  62u, 6u, 38u) ||
        inside_rect(mx, my, 372u,  86u, 20u, 6u)) {
        return mondrian_black();
    }

    return colour;
}

static void upload_mondrian_bitmap(void)
{
    for (uint16_t y = 0u; y < g_bitmap_height; ++y) {
        for (uint16_t byte_x = 0u; byte_x < g_bitmap_row_bytes; ++byte_x) {
            uint16_t left_x = (uint16_t)(byte_x * 2u);
            uint8_t left = mondrian_colour(left_x, y);
            uint8_t right = mondrian_colour((uint16_t)(left_x + 1u), y);
            g_bitmap_row[byte_x] = pack_pixels(left, right);
        }

        twodfx_upload_raw(
            EGG_CD_BITMAP_ADDRESS + (uint32_t)y * g_bitmap_row_bytes,
            g_bitmap_row,
            g_bitmap_row_bytes);
    }
}

static void generate_smiley(uint8_t face_colour,
                            uint8_t detail_colour,
                            uint8_t dark_colour)
{
    memset(g_smiley_pixels, 0, sizeof(g_smiley_pixels));

    for (uint8_t y = 0u; y < EGG_CD_SMILEY_HEIGHT; ++y) {
        for (uint8_t byte_x = 0u; byte_x < EGG_CD_SMILEY_ROW_BYTES; ++byte_x) {
            uint8_t colours[2];

            for (uint8_t pixel = 0u; pixel < 2u; ++pixel) {
                uint8_t x = (uint8_t)(byte_x * 2u + pixel);
                int8_t dx = (int8_t)x - EGG_CD_SMILEY_CENTRE_X;
                int8_t dy = (int8_t)y - EGG_CD_SMILEY_CENTRE_Y;
                uint8_t colour = EGG_CD_SMILEY_TRANSPARENT_COLOUR;

                if ((int16_t)dx * dx + (int16_t)dy * dy <
                    EGG_CD_SMILEY_RADIUS_SQUARED) {
                    colour = face_colour;

                    if ((x == EGG_CD_SMILEY_LEFT_EYE_X ||
                         x == EGG_CD_SMILEY_RIGHT_EYE_X) &&
                        y >= EGG_CD_SMILEY_EYE_MIN_Y &&
                        y <= EGG_CD_SMILEY_EYE_MAX_Y) {
                        colour = dark_colour;
                    }

                    if (y == EGG_CD_SMILEY_MOUTH_Y &&
                        x >= EGG_CD_SMILEY_MOUTH_MIN_X &&
                        x <= EGG_CD_SMILEY_MOUTH_MAX_X) {
                        colour = detail_colour;
                    }

                    if ((x == EGG_CD_SMILEY_MOUTH_CORNER0_X ||
                         x == EGG_CD_SMILEY_MOUTH_CORNER1_X) &&
                        y == EGG_CD_SMILEY_MOUTH_CORNER_Y) {
                        colour = detail_colour;
                    }
                }

                if (x == 0u || x == EGG_CD_SMILEY_WIDTH - 1u ||
                    y == 0u || y == EGG_CD_SMILEY_HEIGHT - 1u) {
                    colour = EGG_CD_SMILEY_TRANSPARENT_COLOUR;
                }

                colours[pixel] = colour;
            }

            g_smiley_pixels[(uint16_t)y * EGG_CD_SMILEY_ROW_BYTES + byte_x] =
                pack_pixels(colours[0], colours[1]);
        }
    }
}

static void upload_smiley_resources(void)
{
    const uint8_t smiley0_face = g_is_tvc
        ? EGG_CD_TVC_SMILEY0_FACE_COLOUR
        : EGG_CD_EP_SMILEY0_FACE_COLOUR;
    const uint8_t smiley0_detail = g_is_tvc
        ? EGG_CD_TVC_SMILEY0_DETAIL_COLOUR
        : EGG_CD_EP_SMILEY0_DETAIL_COLOUR;
    const uint8_t smiley1_face = g_is_tvc
        ? EGG_CD_TVC_SMILEY1_FACE_COLOUR
        : EGG_CD_EP_SMILEY1_FACE_COLOUR;
    const uint8_t smiley1_detail = g_is_tvc
        ? EGG_CD_TVC_SMILEY1_DETAIL_COLOUR
        : EGG_CD_EP_SMILEY1_DETAIL_COLOUR;
    const uint8_t dark = g_is_tvc
        ? EGG_CD_TVC_SMILEY_DARK_COLOUR
        : EGG_CD_EP_SMILEY_DARK_COLOUR;

    generate_smiley(smiley0_face, smiley0_detail, dark);
    twodfx_upload_raw(EGG_CD_SMILEY0_ADDRESS,
                      g_smiley_pixels,
                      sizeof(g_smiley_pixels));

    generate_smiley(smiley1_face, smiley1_detail, dark);
    twodfx_upload_raw(EGG_CD_SMILEY1_ADDRESS,
                      g_smiley_pixels,
                      sizeof(g_smiley_pixels));
}

/* ------------------------------------------------------------------------- */
/* Public-API scene                                                          */
/* ------------------------------------------------------------------------- */

static void build_scene(void)
{
    uint8_t slot = 0u;

    twodfx_reset_sprites();
    twodfx_reset_2dpt_all();

    twodfx_define_bitmap(EGG_CD_BITMAP_ID,
                         EGG_CD_BITMAP_ADDRESS,
                         g_bitmap_width,
                         g_bitmap_height);

    twodfx_2d_blit(TWODFX_LAYER_BACKGROUND, slot++,
                   g_bitmap_x, g_bitmap_y,
                   EGG_CD_BITMAP_ID,
                   TWODFX_BLIT_EXACT);

    /* Small stage markers remain outside the expensive bitmap operation. */
    twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                        EGG_CD_MARKER0_COLOUR, 0u);
    twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND, slot++,
                        EGG_CD_MARKER0_X, EGG_CD_MARKER_Y,
                        EGG_CD_MARKER_WIDTH, EGG_CD_MARKER_HEIGHT);

    if (g_stage == EGG_CD_STAGE_WITH_SPRITES) {
        twodfx_2d_set_color(TWODFX_LAYER_BACKGROUND, slot++,
                            EGG_CD_MARKER1_COLOUR, 0u);
        twodfx_2d_fill_rect(TWODFX_LAYER_BACKGROUND, slot++,
                            EGG_CD_MARKER1_X, EGG_CD_MARKER_Y,
                            EGG_CD_MARKER_WIDTH, EGG_CD_MARKER_HEIGHT);
    }

    twodfx_2d_ret(TWODFX_LAYER_BACKGROUND, slot);

    if (g_stage == EGG_CD_STAGE_WITH_SPRITES) {
        twodfx_sprite_set(EGG_CD_SMILEY0_SPRITE,
                          EGG_CD_SMILEY0_ADDRESS,
                          EGG_CD_SMILEY0_X, EGG_CD_SMILEY0_Y,
                          EGG_CD_SMILEY_WIDTH, EGG_CD_SMILEY_HEIGHT,
                          true, false, EGG_CD_SMILEY_TRANSFORMS);

        twodfx_sprite_set(EGG_CD_SMILEY1_SPRITE,
                          EGG_CD_SMILEY1_ADDRESS,
                          EGG_CD_SMILEY1_X, EGG_CD_SMILEY1_Y,
                          EGG_CD_SMILEY_WIDTH, EGG_CD_SMILEY_HEIGHT,
                          true, false, EGG_CD_SMILEY_TRANSFORMS);
    }
}

void egg_CD_blit_full_start(void)
{
    g_stage = EGG_CD_STAGE_BLIT_ONLY;
    g_stage_tick = 0u;

    select_profile_geometry();
    upload_mondrian_bitmap();
    upload_smiley_resources();
    build_scene();
}

void egg_CD_blit_full_tick(void)
{
    ++g_stage_tick;

    if (g_stage_tick >= EGG_CD_STAGE_TICKS) {
        g_stage_tick = 0u;
        g_stage = (uint8_t)((g_stage + 1u) % EGG_CD_STAGE_COUNT);
        build_scene();
    }
}
