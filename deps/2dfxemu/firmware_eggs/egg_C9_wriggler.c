#include "egg_C9_wriggler.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "egg_C9_wriggler_assets.h"

/* ------------------------------------------------------------------------- */
/* Runtime state                                                             */
/* ------------------------------------------------------------------------- */

typedef struct {
    int16_t x;
    int16_t y;
    int8_t dx;
    int8_t dy;
    uint8_t phase;
    uint8_t phase_tick;
    uint8_t phase_hold;
    uint8_t colour;
    bool x_flipped;
    bool active;
} egg_C9_worm_t;

static bool g_is_tvc;
static egg_C9_scene_t g_scene;
static uint16_t g_scene_tick;

static egg_C9_worm_t g_worms[EGG_C9_NORMAL_SPRITE_COUNT];
static uint8_t g_visible_worms;

static int16_t g_giant_x;
static uint8_t g_giant_phase;
static uint8_t g_giant_phase_tick;

static uint32_t g_random_state;

/* One work buffer is enough because each converted phase is uploaded at once. */
static uint8_t g_conversion_buffer[EGG_C9_NORMAL_PHASE_BYTES];

/* ------------------------------------------------------------------------- */
/* Profile-selected safe-area staging helpers                                */
/* ------------------------------------------------------------------------- */

static uint16_t safe_min_x(void)
{
    return g_is_tvc ? TWODFX_TVC_BASIC_SAFE_MIN_X
                    : TWODFX_ENTERPRISE_BASIC_SAFE_MIN_X;
}

static uint16_t safe_max_x(void)
{
    return g_is_tvc ? TWODFX_TVC_BASIC_SAFE_MAX_X
                    : TWODFX_ENTERPRISE_BASIC_SAFE_MAX_X;
}

static uint16_t safe_min_y(void)
{
    return g_is_tvc ? TWODFX_TVC_BASIC_SAFE_MIN_Y
                    : TWODFX_ENTERPRISE_BASIC_SAFE_MIN_Y;
}

static uint16_t safe_max_y(void)
{
    return g_is_tvc ? TWODFX_TVC_BASIC_SAFE_MAX_Y
                    : TWODFX_ENTERPRISE_BASIC_SAFE_MAX_Y;
}

static int16_t head_left_x(void)
{
    return (int16_t)(safe_min_x() -
                     (EGG_C9_PHASE0_VISIBLE_WIDTH -
                      EGG_C9_HEAD_VISIBLE_PIXELS));
}

static int16_t head_right_x(void)
{
    /*
     * Phase 0 has a transparent right margin. X_FLIP moves that margin to
     * the left edge of a right-entering sprite, so account for it before
     * exposing the requested visible head pixels at the safe-area edge.
     */
    const uint16_t transparent_margin =
        EGG_C9_WORM_WIDTH - EGG_C9_PHASE0_VISIBLE_WIDTH;
    return (int16_t)(safe_max_x() - EGG_C9_HEAD_VISIBLE_PIXELS + 1u -
                     transparent_margin);
}

static uint16_t head_y_min(void)
{
    return (uint16_t)(safe_min_y() + EGG_C9_HEAD_VERTICAL_MARGIN);
}

static uint16_t head_y_max(void)
{
    return (uint16_t)(safe_max_y() - EGG_C9_WORM_HEIGHT -
                      EGG_C9_HEAD_VERTICAL_MARGIN + 1u);
}

static uint16_t head_bottom_x_min(void)
{
    return (uint16_t)(safe_min_x() + EGG_C9_HEAD_BOTTOM_MARGIN);
}

static uint16_t head_bottom_x_max(void)
{
    return (uint16_t)(safe_max_x() - EGG_C9_WORM_WIDTH -
                      EGG_C9_HEAD_BOTTOM_MARGIN + 1u);
}

static uint16_t head_bottom_y(void)
{
    return (uint16_t)(safe_max_y() - EGG_C9_HEAD_VISIBLE_PIXELS + 1u +
                      EGG_C9_BOTTOM_EXTRA_OFFSET_Y);
}

static int16_t giant_start_x(void)
{
    return g_is_tvc ? (int16_t)EGG_C9_TVC_GIANT_START_X
                    : (int16_t)EGG_C9_EP_GIANT_START_X;
}

static int16_t giant_target_x(void)
{
    return g_is_tvc ? (int16_t)EGG_C9_TVC_GIANT_TARGET_X
                    : (int16_t)EGG_C9_EP_GIANT_TARGET_X;
}

/* ------------------------------------------------------------------------- */
/* Deterministic random helper                                               */
/* ------------------------------------------------------------------------- */

static uint32_t random_next(void)
{
    g_random_state = g_random_state * 1664525u + 1013904223u;
    return g_random_state;
}

static uint16_t random_range(uint16_t minimum, uint16_t maximum)
{
    const uint16_t span = (uint16_t)(maximum - minimum + 1u);
    return (uint16_t)(minimum + (uint16_t)(random_next() % span));
}

/* ------------------------------------------------------------------------- */
/* Source-artwork conversion and upload                                      */
/* ------------------------------------------------------------------------- */

static uint8_t remap_body_colour(uint8_t source_colour, uint8_t body_colour)
{
    if ((source_colour & 0x0Fu) == EGG_C9_SOURCE_TRANSPARENT_COLOUR) {
        return 0u;
    }

    body_colour &= 0x0Fu;
    return body_colour == 0u ? EGG_C9_FALLBACK_BODY_COLOUR : body_colour;
}

static void build_and_upload_normal_phases(void)
{
    /*
     * Pre-generate eight phases for every body colour. Only one 512-byte
     * phase is held in MCU RAM at a time; the complete 60 KiB bank lives in
     * 2dfx resource RAM.
     */
    for (uint8_t colour = 1u;
         colour <= EGG_C9_BODY_COLOUR_COUNT;
         ++colour) {
        const uint32_t colour_base =
            EGG_C9_NORMAL_BASE_ADDRESS +
            (uint32_t)(colour - 1u) * EGG_C9_COLOUR_SET_BYTES;

        for (uint8_t phase = 0u; phase < EGG_C9_PHASE_COUNT; ++phase) {
            const uint8_t *source = wrig_sources[phase];

            for (uint16_t i = 0u; i < EGG_C9_NORMAL_PHASE_BYTES; ++i) {
                const uint8_t packed = source[i];
                const uint8_t left =
                    remap_body_colour((uint8_t)(packed >> 4), colour);
                const uint8_t right =
                    remap_body_colour((uint8_t)(packed & 0x0Fu), colour);

                /* Internal packing: even/left pixel in the low nibble. */
                g_conversion_buffer[i] =
                    (uint8_t)(left | (uint8_t)(right << 4));
            }

            twodfx_upload_raw(colour_base +
                                  (uint32_t)phase * EGG_C9_NORMAL_PHASE_BYTES,
                              g_conversion_buffer,
                              EGG_C9_NORMAL_PHASE_BYTES);
        }
    }
}

static void write_giant_row_pixel(uint16_t x, uint8_t colour)
{
    const uint16_t offset = (uint16_t)(x >> 1);
    uint8_t packed = g_conversion_buffer[offset];

    if ((x & 1u) == 0u) {
        packed = (uint8_t)((packed & 0xF0u) | (colour & 0x0Fu));
    } else {
        packed = (uint8_t)((packed & 0x0Fu) |
                           (uint8_t)((colour & 0x0Fu) << 4));
    }

    g_conversion_buffer[offset] = packed;
}

static void build_and_upload_giant_phases(void)
{
    /*
     * Expand one 64-pixel source row to one 256-pixel giant row, duplicate it
     * four times vertically in the same 512-byte work buffer, then upload that
     * complete four-row strip. The egg therefore never reserves an 8 KiB
     * giant-phase buffer in MCU RAM.
     */
    for (uint8_t phase = 0u; phase < EGG_C9_PHASE_COUNT; ++phase) {
        const uint8_t *source = wrig_sources[phase];
        const uint32_t phase_base =
            EGG_C9_GIANT_BASE_ADDRESS +
            (uint32_t)phase * EGG_C9_GIANT_PHASE_BYTES;

        for (uint16_t source_y = 0u;
             source_y < EGG_C9_WORM_HEIGHT;
             ++source_y) {
            memset(g_conversion_buffer, 0, EGG_C9_GIANT_BYTES_PER_ROW);

            for (uint16_t source_byte = 0u;
                 source_byte < EGG_C9_WORM_BYTES_PER_ROW;
                 ++source_byte) {
                const uint8_t packed =
                    source[(uint32_t)source_y * EGG_C9_WORM_BYTES_PER_ROW +
                           source_byte];
                const uint8_t colours[2] = {
                    remap_body_colour(
                        (uint8_t)(packed >> 4),
                        g_is_tvc ? EGG_C9_TVC_GIANT_BODY_COLOUR
                                 : EGG_C9_EP_GIANT_BODY_COLOUR),
                    remap_body_colour(
                        (uint8_t)(packed & 0x0Fu),
                        g_is_tvc ? EGG_C9_TVC_GIANT_BODY_COLOUR
                                 : EGG_C9_EP_GIANT_BODY_COLOUR)
                };

                for (uint8_t source_pixel = 0u;
                     source_pixel < 2u;
                     ++source_pixel) {
                    const uint16_t source_x =
                        (uint16_t)(source_byte * 2u + source_pixel);

                    for (uint8_t scale_x = 0u;
                         scale_x < EGG_C9_GIANT_STORED_SCALE;
                         ++scale_x) {
                        write_giant_row_pixel(
                            (uint16_t)(source_x * EGG_C9_GIANT_STORED_SCALE +
                                       scale_x),
                            colours[source_pixel]);
                    }
                }
            }

            for (uint8_t scale_y = 1u;
                 scale_y < EGG_C9_GIANT_STORED_SCALE;
                 ++scale_y) {
                memcpy(g_conversion_buffer +
                           (uint32_t)scale_y * EGG_C9_GIANT_BYTES_PER_ROW,
                       g_conversion_buffer,
                       EGG_C9_GIANT_BYTES_PER_ROW);
            }

            twodfx_upload_raw(
                phase_base +
                    (uint32_t)source_y * EGG_C9_GIANT_STORED_SCALE *
                        EGG_C9_GIANT_BYTES_PER_ROW,
                g_conversion_buffer,
                EGG_C9_GIANT_BYTES_PER_ROW * EGG_C9_GIANT_STORED_SCALE);
        }
    }
}

/* ------------------------------------------------------------------------- */
/* Sprite submission                                                         */
/* ------------------------------------------------------------------------- */

static uint32_t normal_phase_address(uint8_t phase, uint8_t colour)
{
    colour &= 0x0Fu;
    if (colour == 0u) {
        colour = EGG_C9_FALLBACK_BODY_COLOUR;
    }

    return EGG_C9_NORMAL_BASE_ADDRESS +
           (uint32_t)(colour - 1u) * EGG_C9_COLOUR_SET_BYTES +
           (uint32_t)(phase & 7u) * EGG_C9_NORMAL_PHASE_BYTES;
}

static uint32_t giant_phase_address(uint8_t phase)
{
    return EGG_C9_GIANT_BASE_ADDRESS +
           (uint32_t)(phase & 7u) * EGG_C9_GIANT_PHASE_BYTES;
}

static bool worm_is_visible_enough(const egg_C9_worm_t *worm)
{
    if (!worm->active || worm->x < 0 || worm->y < 0) {
        return false;
    }

    const int16_t right_limit = g_is_tvc
        ? (int16_t)(EGG_C9_TVC_FRAMEBUFFER_WIDTH + EGG_C9_WORM_WIDTH)
        : (int16_t)(EGG_C9_EP_SCANOUT_WIDTH + EGG_C9_WORM_WIDTH);
    const int16_t bottom_limit = g_is_tvc
        ? (int16_t)(EGG_C9_TVC_LAST_RENDER_Y + EGG_C9_WORM_HEIGHT)
        : (int16_t)(EGG_C9_EP_LAST_RENDER_Y + EGG_C9_WORM_HEIGHT);

    return worm->x <= right_limit && worm->y <= bottom_limit;
}

static void submit_small_worm(uint8_t sprite, const egg_C9_worm_t *worm)
{
    if (!worm_is_visible_enough(worm)) {
        return;
    }

    twodfx_sprite_set(
        sprite,
        normal_phase_address(worm->phase, worm->colour),
        (uint16_t)worm->x,
        (uint16_t)worm->y,
        EGG_C9_WORM_WIDTH,
        EGG_C9_WORM_HEIGHT,
        true,
        false,
        worm->x_flipped ? TWODFX_SPRITE_X_FLIP : 0u);
}

static void submit_giant_sprite(void)
{
    twodfx_sprite_set(EGG_C9_GIANT_SPRITE,
                      giant_phase_address(g_giant_phase),
                      (uint16_t)g_giant_x,
                      EGG_C9_GIANT_Y,
                      EGG_C9_GIANT_STORED_WIDTH,
                      EGG_C9_GIANT_STORED_HEIGHT,
                      true,
                      false,
                      TWODFX_SPRITE_X_FLIP |
                          TWODFX_SPRITE_X_DOUBLE |
                          TWODFX_SPRITE_Y_DOUBLE);
}

/* ------------------------------------------------------------------------- */
/* Worm initialization                                                       */
/* ------------------------------------------------------------------------- */

static void initialize_one_head(uint8_t index)
{
    egg_C9_worm_t *worm = &g_worms[index];
    memset(worm, 0, sizeof(*worm));

    const uint8_t side = (uint8_t)(random_next() % 3u);

    worm->phase_hold =
        (uint8_t)random_range(EGG_C9_PHASE_HOLD_MIN,
                              EGG_C9_PHASE_HOLD_MAX);
    /*
     * Colour zero is the transparent source index.  Both machine profiles
     * therefore select from every pre-generated visible colour bank 1..15.
     */
    worm->colour = (uint8_t)random_range(1u, EGG_C9_BODY_COLOUR_COUNT);
    worm->active = true;

    if (side == 0u) {
        worm->x = head_left_x();
        worm->y = (int16_t)random_range(head_y_min(), head_y_max());
        worm->dx = 1;
        worm->dy = (int8_t)((int)(random_next() % 3u) - 1);
        worm->x_flipped = false;
    } else if (side == 1u) {
        worm->x = head_right_x();
        worm->y = (int16_t)random_range(head_y_min(), head_y_max());
        worm->dx = -1;
        worm->dy = (int8_t)((int)(random_next() % 3u) - 1);
        worm->x_flipped = true;
    } else {
        worm->x =
            (int16_t)random_range(head_bottom_x_min(), head_bottom_x_max());
        worm->y = (int16_t)head_bottom_y();
        worm->dx = (random_next() & 1u) != 0u ? 1 : -1;
        worm->dy = -1;
        worm->x_flipped = worm->dx < 0;
    }
}

static void initialize_all_heads(void)
{
    g_random_state = EGG_C9_RANDOM_SEED;

    for (uint8_t i = 0u; i < EGG_C9_NORMAL_SPRITE_COUNT; ++i) {
        initialize_one_head(i);
    }

    g_visible_worms = 0u;
}

static void respawn_attack_worm(uint8_t index)
{
    initialize_one_head(index);
    g_worms[index].phase = (uint8_t)(random_next() & 7u);
}

/* ------------------------------------------------------------------------- */
/* Scene animation                                                           */
/* ------------------------------------------------------------------------- */

static void advance_scene(egg_C9_scene_t next_scene)
{
    g_scene = next_scene;
    g_scene_tick = 0u;
}

static void update_head_reveal(void)
{
    if ((g_scene_tick % EGG_C9_HEAD_APPEAR_EVERY) == 0u &&
        g_visible_worms < EGG_C9_WORM_COUNT &&
        g_visible_worms < EGG_C9_NORMAL_SPRITE_COUNT) {
        ++g_visible_worms;
    }

    if (g_visible_worms >= EGG_C9_WORM_COUNT ||
        g_visible_worms >= EGG_C9_NORMAL_SPRITE_COUNT) {
        advance_scene(EGG_C9_SCENE_WAIT_ATTACK);
    }
}

static void update_attack_worms(void)
{
    uint8_t worm_count = EGG_C9_WORM_COUNT;
    if (worm_count > EGG_C9_NORMAL_SPRITE_COUNT) {
        worm_count = EGG_C9_NORMAL_SPRITE_COUNT;
    }

    for (uint8_t i = 0u; i < worm_count; ++i) {
        egg_C9_worm_t *worm = &g_worms[i];

        ++worm->phase_tick;
        if (worm->phase_tick >= worm->phase_hold) {
            worm->phase_tick = 0u;
            worm->phase = (uint8_t)((worm->phase + 1u) & 7u);

            if (worm->dy != 0) {
                worm->y = (int16_t)(worm->y + worm->dy);
            }

            if (worm->phase == 0u) {
                worm->x =
                    (int16_t)(worm->x +
                              (int16_t)worm->dx * EGG_C9_ATTACK_STEP_X);
            }
        }

        bool escaped;
        if (g_is_tvc) {
            escaped =
                worm->x < -(int16_t)EGG_C9_WORM_WIDTH ||
                worm->x > (int16_t)(EGG_C9_TVC_FRAMEBUFFER_WIDTH +
                                    EGG_C9_WORM_WIDTH) ||
                worm->y < (int16_t)(EGG_C9_TVC_FIRST_RENDER_Y -
                                    EGG_C9_WORM_HEIGHT) ||
                worm->y > (int16_t)(EGG_C9_TVC_LAST_RENDER_Y +
                                    EGG_C9_WORM_HEIGHT);
        } else {
            escaped =
                worm->x < 0 ||
                worm->x > (int16_t)(EGG_C9_EP_SCANOUT_WIDTH +
                                    EGG_C9_WORM_WIDTH) ||
                worm->y < (int16_t)(EGG_C9_EP_FIRST_RENDER_Y -
                                    EGG_C9_WORM_HEIGHT) ||
                worm->y > (int16_t)(EGG_C9_EP_LAST_RENDER_Y +
                                    EGG_C9_WORM_HEIGHT);
        }

        if (escaped) {
            respawn_attack_worm(i);
        }
    }
}

static void update_giant(void)
{
    if (g_scene == EGG_C9_SCENE_GIANT_ENTER) {
        ++g_giant_phase_tick;

        if (g_giant_phase_tick >= EGG_C9_GIANT_PHASE_HOLD) {
            g_giant_phase_tick = 0u;
            g_giant_phase = (uint8_t)((g_giant_phase + 1u) & 7u);

            if (g_giant_phase == 0u) {
                const int16_t target_x = giant_target_x();

                if (g_giant_x > target_x) {
                    if (g_is_tvc &&
                        g_giant_x <
                            (int16_t)(target_x +
                                      EGG_C9_GIANT_ADVANCE_PIXELS)) {
                        g_giant_x = target_x;
                    } else {
                        g_giant_x =
                            (int16_t)(g_giant_x -
                                      (int16_t)EGG_C9_GIANT_ADVANCE_PIXELS);
                    }
                }

                if (g_giant_x <= target_x) {
                    g_giant_x = target_x;
                    g_giant_phase = EGG_C9_GIANT_FINAL_PHASE;
                    advance_scene(EGG_C9_SCENE_FOREVER);
                }
            }
        }
    } else {
        g_giant_phase = EGG_C9_GIANT_FINAL_PHASE;
    }
}

static void submit_current_scene(void)
{
    twodfx_reset_sprites();

    switch (g_scene) {
        case EGG_C9_SCENE_HEADS_APPEAR:
        case EGG_C9_SCENE_WAIT_ATTACK:
        case EGG_C9_SCENE_ATTACK:
        case EGG_C9_SCENE_GIANT_ENTER:
        case EGG_C9_SCENE_FOREVER: {
            uint8_t visible = g_visible_worms;
            if (visible > EGG_C9_WORM_COUNT) {
                visible = EGG_C9_WORM_COUNT;
            }
            if (visible > EGG_C9_NORMAL_SPRITE_COUNT) {
                visible = EGG_C9_NORMAL_SPRITE_COUNT;
            }

            for (uint8_t i = 0u; i < visible; ++i) {
                submit_small_worm(i, &g_worms[i]);
            }

            if (g_scene == EGG_C9_SCENE_GIANT_ENTER ||
                g_scene == EGG_C9_SCENE_FOREVER) {
                submit_giant_sprite();
            }
            break;
        }

        default:
            break;
    }
}

/* ------------------------------------------------------------------------- */
/* Public egg entry points                                                   */
/* ------------------------------------------------------------------------- */

void egg_C9_wriggler_start(void)
{
    g_is_tvc = twodfx_get_machine() == TWODFX_MACHINE_TVC;

    twodfx_reset_2dpt_all();
    twodfx_reset_sprites();

    g_scene = EGG_C9_SCENE_HEADS_APPEAR;
    g_scene_tick = 0u;

    g_giant_x = giant_start_x();
    g_giant_phase = EGG_C9_GIANT_FINAL_PHASE;
    g_giant_phase_tick = 0u;

    build_and_upload_normal_phases();
    build_and_upload_giant_phases();
    initialize_all_heads();

    /* Display the first warning head immediately on both profiles. */
    g_visible_worms = 1u;

    submit_current_scene();
}

void egg_C9_wriggler_tick(void)
{
    ++g_scene_tick;

    switch (g_scene) {
        case EGG_C9_SCENE_HEADS_APPEAR:
            update_head_reveal();
            break;

        case EGG_C9_SCENE_WAIT_ATTACK:
            if (g_scene_tick >= EGG_C9_HEAD_WAIT_AFTER) {
                g_visible_worms = EGG_C9_WORM_COUNT;
                if (g_visible_worms > EGG_C9_NORMAL_SPRITE_COUNT) {
                    g_visible_worms = EGG_C9_NORMAL_SPRITE_COUNT;
                }
                advance_scene(EGG_C9_SCENE_ATTACK);
            }
            break;

        case EGG_C9_SCENE_ATTACK:
            update_attack_worms();
            if (g_scene_tick >= EGG_C9_ATTACK_FRAMES) {
                g_giant_x = giant_start_x();
                g_giant_phase = 0u;
                g_giant_phase_tick = 0u;
                advance_scene(EGG_C9_SCENE_GIANT_ENTER);
            }
            break;

        case EGG_C9_SCENE_GIANT_ENTER:
            update_attack_worms();
            update_giant();
            break;

        case EGG_C9_SCENE_FOREVER:
            update_attack_worms();
            update_giant();
            break;

        default:
            advance_scene(EGG_C9_SCENE_HEADS_APPEAR);
            break;
    }

    submit_current_scene();
}
