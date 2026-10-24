#include "egg_C0_logo.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "egg_C0_logo_assets.h"
#include "pico/rand.h"
#include "pico/stdlib.h"

typedef struct {
    uint16_t x, y;
    uint16_t tx, ty;
    int16_t vx, vy;
    uint16_t w, h;
    uint32_t addr;
    uint8_t transform;

    /*
     * ENTERPRISE contains repeated letters.  The visible letter sprites
     * remain independent SPBs, but their graphics data can share one of the
     * unique glyph assets below.
     */
    uint8_t glyph;

    /* Smooth horizontal/vertical card-turn state, used during boundary bounces. */
    bool xturn_active;
    uint8_t xturn_step;
    bool stable_xflip;

    bool yturn_active;
    uint8_t yturn_step;
    bool stable_yflip;
} logo_sprite_t;

typedef struct {
    uint16_t w;
    uint16_t h;
    const uint8_t *data;
} logo_asset_t;

static uint32_t g_rng = EGG_C0_RNG_INITIAL_FALLBACK;

static uint32_t rnd32(void);

/*
 * Unique glyph asset table.
 *
 * The displayed word is still ten independent sprites, but repeated letters
 * share the same generated sprite data.  This saves resource RAM and also
 * makes the generated X-turn animation phases shared between repeated glyphs.
 */
static const logo_asset_t
unique_letter_assets[EGG_C0_UNIQUE_GLYPH_COUNT] =
    EGG_C0_UNIQUE_LETTER_ASSET_VALUES;

static const uint8_t logo_word_glyph[EGG_C0_LETTER_COUNT] =
    EGG_C0_WORD_GLYPH_VALUES;

static const logo_asset_t stripe_assets[EGG_C0_STRIPE_COUNT] =
    EGG_C0_STRIPE_ASSET_VALUES;




static bool g_active = false;
static egg_C0_phase_t g_phase = EGG_C0_PHASE_CHAOS;
static uint32_t g_phase_timer = 0;

static logo_sprite_t g_letters[EGG_C0_LETTER_COUNT];
static logo_sprite_t g_strips[EGG_C0_STRIPE_COUNT];
static logo_sprite_t g_logo_2dfx;

static bool g_2dfx_drop_started = false;
static bool g_2dfx_drop_settled = false;
static int32_t g_2dfx_y_fp = ((int32_t)EGG_C0_2DFX_START_Y << EGG_C0_FIXED_POINT_SHIFT);
static int32_t g_2dfx_vy_fp = EGG_C0_2DFX_INITIAL_VELOCITY;

static uint32_t g_glyph_xturn_addr[EGG_C0_UNIQUE_GLYPH_COUNT][EGG_C0_XTURN_PHASES];
static uint32_t g_glyph_yturn_addr[EGG_C0_UNIQUE_GLYPH_COUNT][EGG_C0_YTURN_PHASES];

static uint8_t g_flipback_index = 0;
static uint16_t g_flipback_wait = 0;

static inline uint16_t logo_display_w(const logo_sprite_t *s)
{
    return (uint16_t)(s->w * ((s->transform & EGG_C0_X_DOUBLE) ? 2u : 1u));
}

static inline uint16_t logo_display_h(const logo_sprite_t *s)
{
    return (uint16_t)(s->h * ((s->transform & EGG_C0_Y_DOUBLE) ? 2u : 1u));
}

static uint8_t g_asset_work[EGG_C0_MAX_GLYPH_BYTES];
static uint8_t g_logo_row_work[LOGO_2DFX_BYTES_PER_ROW];
static bool g_is_tvc;

static uint16_t active_screen_width(void)
{
    return g_is_tvc
        ? EGG_C0_TVC_SCREEN_WIDTH
        : EGG_C0_ENTERPRISE_SCREEN_WIDTH;
}

static uint16_t active_visible_first_y(void)
{
    return g_is_tvc
        ? EGG_C0_TVC_VISIBLE_FIRST_Y
        : EGG_C0_ENTERPRISE_VISIBLE_FIRST_Y;
}

static uint16_t active_visible_height(void)
{
    return g_is_tvc
        ? EGG_C0_TVC_VISIBLE_HEIGHT
        : EGG_C0_ENTERPRISE_VISIBLE_HEIGHT;
}

static int32_t active_chaos_min_x(void)
{
    return g_is_tvc
        ? EGG_C0_TVC_CHAOS_MIN_X
        : EGG_C0_ENTERPRISE_CHAOS_MIN_X;
}

static int32_t active_chaos_max_x(void)
{
    return g_is_tvc
        ? EGG_C0_TVC_CHAOS_MAX_X
        : EGG_C0_ENTERPRISE_CHAOS_MAX_X;
}

static int32_t active_chaos_min_y(void)
{
    return g_is_tvc
        ? EGG_C0_TVC_CHAOS_MIN_Y
        : EGG_C0_ENTERPRISE_CHAOS_MIN_Y;
}

static int32_t active_chaos_max_y(void)
{
    return g_is_tvc
        ? EGG_C0_TVC_CHAOS_MAX_Y
        : EGG_C0_ENTERPRISE_CHAOS_MAX_Y;
}

static inline uint16_t logo_apply_global_x_offset(int32_t x)
{
    int32_t shifted = x + EGG_C0_GLOBAL_X_OFFSET;

    if (shifted < 0) {
        shifted = 0;
    }

    if (shifted > (int32_t)EGG_C0_RAW_COORDINATE_MAX) {
        shifted = (int32_t)EGG_C0_RAW_COORDINATE_MAX;
    }

    return (uint16_t)shifted;
}

static inline uint16_t logo_random_coord_inside(int32_t min_value,
                                                 int32_t max_value,
                                                 uint16_t size)
{
    int32_t span = max_value - min_value - (int32_t)size;

    if (span <= 0) {
        return (uint16_t)min_value;
    }

    return (uint16_t)(min_value +
                      (int32_t)(rnd32() % (uint32_t)(span + 1)));
}

static inline uint8_t internal_pair_from_source_pair(uint8_t source_byte)
{
    return (uint8_t)((source_byte >> 4) | (source_byte << 4));
}

static uint8_t recolour_large_logo_nibble(uint8_t colour)
{
    colour &= 0x0Fu;

    if (!g_is_tvc) {
        return colour;
    }

    if (colour == EGG_C0_SOURCE_LOGO_TEXT_COLOUR) {
        return EGG_C0_TVC_LOGO_TEXT_COLOUR;
    }

    if (colour == EGG_C0_SOURCE_LOGO_ACCENT_COLOUR) {
        return EGG_C0_TVC_LOGO_ACCENT_COLOUR;
    }

    return colour;
}

static uint8_t recolour_large_logo_source_pair(uint8_t source_byte)
{
    uint8_t left = recolour_large_logo_nibble((uint8_t)(source_byte >> 4));
    uint8_t right = recolour_large_logo_nibble(source_byte);

    return (uint8_t)((left << 4) | right);
}

static uint16_t logo_2dfx_target_y(void)
{
    return g_is_tvc
        ? EGG_C0_TVC_2DFX_TARGET_Y
        : EGG_C0_ENTERPRISE_2DFX_TARGET_Y;
}

static uint8_t source_asset_get_pixel(const uint8_t *source,
                                      uint16_t width,
                                      uint16_t x,
                                      uint16_t y)
{
    uint16_t bytes_per_row = (uint16_t)((width + 1u) >> 1);
    uint8_t packed = source[(uint32_t)y * bytes_per_row + (x >> 1)];

    return (x & 1u)
        ? (uint8_t)(packed & 0x0Fu)
        : (uint8_t)((packed >> 4) & 0x0Fu);
}

static void work_buffer_set_pixel(uint8_t *buffer,
                                  uint16_t width,
                                  uint16_t x,
                                  uint16_t y,
                                  uint8_t colour)
{
    uint16_t bytes_per_row = (uint16_t)((width + 1u) >> 1);
    uint32_t offset = (uint32_t)y * bytes_per_row + (x >> 1);

    colour &= 0x0Fu;

    if (x & 1u) {
        buffer[offset] = (uint8_t)((buffer[offset] & 0x0Fu) | (colour << 4));
    } else {
        buffer[offset] = (uint8_t)((buffer[offset] & 0xF0u) | colour);
    }
}

static void convert_source_asset(uint8_t *destination,
                                 const uint8_t *source,
                                 uint32_t length)
{
    for (uint32_t i = 0u; i < length; ++i) {
        destination[i] = internal_pair_from_source_pair(source[i]);
    }
}

static void build_horizontal_turn_phase(uint8_t *destination,
                                        const uint8_t *source,
                                        uint16_t width,
                                        uint16_t height,
                                        uint8_t phase)
{
    uint16_t bytes_per_row = (uint16_t)((width + 1u) >> 1);
    uint32_t bytes_total = (uint32_t)bytes_per_row * height;

    memset(destination, 0, bytes_total);

    if (phase == 0u) {
        convert_source_asset(destination, source, bytes_total);
        return;
    }

    uint16_t visible_width = (uint16_t)(
        width - ((uint32_t)phase *
                 (width - EGG_C0_TURN_MIN_VISIBLE_PIXELS)) /
                    (EGG_C0_XTURN_PHASES - 1u));

    if (visible_width < EGG_C0_TURN_MIN_VISIBLE_PIXELS) {
        visible_width = EGG_C0_TURN_MIN_VISIBLE_PIXELS;
    }

    uint16_t left = (uint16_t)((width - visible_width) / 2u);

    for (uint16_t y = 0u; y < height; ++y) {
        for (uint16_t destination_x = 0u;
             destination_x < visible_width;
             ++destination_x) {
            uint16_t source_x;

            if (visible_width <= 1u) {
                source_x = (uint16_t)(width / 2u);
            } else {
                source_x = (uint16_t)(
                    ((uint32_t)destination_x * (width - 1u) +
                     ((visible_width - 1u) / 2u)) /
                    (visible_width - 1u));
            }

            uint8_t colour =
                source_asset_get_pixel(source, width, source_x, y);

            if (colour != EGG_C0_SOURCE_TRANSPARENT_COLOUR) {
                work_buffer_set_pixel(destination, width,
                                      (uint16_t)(left + destination_x), y,
                                      colour);
            }
        }
    }
}

static void build_vertical_turn_phase(uint8_t *destination,
                                      const uint8_t *source,
                                      uint16_t width,
                                      uint16_t height,
                                      uint8_t phase)
{
    uint16_t bytes_per_row = (uint16_t)((width + 1u) >> 1);
    uint32_t bytes_total = (uint32_t)bytes_per_row * height;

    memset(destination, 0, bytes_total);

    if (phase == 0u) {
        convert_source_asset(destination, source, bytes_total);
        return;
    }

    uint16_t visible_height = (uint16_t)(
        height - ((uint32_t)phase *
                  (height - EGG_C0_TURN_MIN_VISIBLE_PIXELS)) /
                     (EGG_C0_YTURN_PHASES - 1u));

    if (visible_height < EGG_C0_TURN_MIN_VISIBLE_PIXELS) {
        visible_height = EGG_C0_TURN_MIN_VISIBLE_PIXELS;
    }

    uint16_t top = (uint16_t)((height - visible_height) / 2u);

    for (uint16_t destination_y = 0u;
         destination_y < visible_height;
         ++destination_y) {
        uint16_t source_y;

        if (visible_height <= 1u) {
            source_y = (uint16_t)(height / 2u);
        } else {
            source_y = (uint16_t)(
                ((uint32_t)destination_y * (height - 1u) +
                 ((visible_height - 1u) / 2u)) /
                (visible_height - 1u));
        }

        for (uint16_t x = 0u; x < width; ++x) {
            uint8_t colour =
                source_asset_get_pixel(source, width, x, source_y);

            if (colour != EGG_C0_SOURCE_TRANSPARENT_COLOUR) {
                work_buffer_set_pixel(destination, width, x,
                                      (uint16_t)(top + destination_y),
                                      colour);
            }
        }
    }
}

static void logo_letter_set_xflip(int index, bool enabled)
{
    if (enabled) {
        g_letters[index].transform |= EGG_C0_X_FLIP;
    } else {
        g_letters[index].transform &= (uint8_t)~EGG_C0_X_FLIP;
    }
}

static void logo_letter_start_xturn(int index)
{
    if (g_letters[index].xturn_active || g_letters[index].yturn_active) {
        return;
    }

    g_letters[index].stable_xflip = (g_letters[index].transform & EGG_C0_X_FLIP) != 0;
    g_letters[index].xturn_active = true;
    g_letters[index].xturn_step = 1;
}

static void logo_letter_tick_xturn(int index)
{
    logo_sprite_t *letter = &g_letters[index];

    if (!letter->xturn_active) {
        return;
    }

    uint8_t phase;
    bool xflip;

    if (letter->xturn_step <= (EGG_C0_XTURN_PHASES - 1)) {
        phase = letter->xturn_step;
        xflip = letter->stable_xflip;
    } else {
        phase = (uint8_t)(EGG_C0_XTURN_STEPS - letter->xturn_step);
        xflip = !letter->stable_xflip;
    }

    /*
     * Keep the phase index safe even while experimenting with
     * EGG_C0_XTURN_PHASES / EGG_C0_XTURN_STEPS.  This also allows deliberately
     * holding the edge-on phase for an extra frame by making STEPS one larger
     * than the perfectly symmetric value.
     */
    if (phase >= EGG_C0_XTURN_PHASES) {
        phase = (uint8_t)(EGG_C0_XTURN_PHASES - 1);
    }

    letter->addr = g_glyph_xturn_addr[letter->glyph][phase];
    logo_letter_set_xflip(index, xflip);

    if (letter->xturn_step >= EGG_C0_XTURN_STEPS) {
        letter->xturn_active = false;
        letter->stable_xflip = !letter->stable_xflip;
        letter->xturn_step = 0;

        letter->addr = g_glyph_xturn_addr[letter->glyph][0];
        logo_letter_set_xflip(index, letter->stable_xflip);
    } else {
        letter->xturn_step++;
    }
}

static void logo_letter_reset_xturn(int index)
{
    logo_sprite_t *letter = &g_letters[index];

    letter->xturn_active = false;
    letter->xturn_step = 0;
    letter->stable_xflip = false;

    letter->addr = g_glyph_xturn_addr[letter->glyph][0];
    logo_letter_set_xflip(index, false);
}

static void logo_letter_set_yflip(int index, bool enabled)
{
    if (enabled) {
        g_letters[index].transform |= EGG_C0_Y_FLIP;
    } else {
        g_letters[index].transform &= (uint8_t)~EGG_C0_Y_FLIP;
    }
}

static void logo_letter_start_yturn(int index)
{
    if (g_letters[index].yturn_active || g_letters[index].xturn_active) {
        return;
    }

    g_letters[index].stable_yflip = (g_letters[index].transform & EGG_C0_Y_FLIP) != 0;
    g_letters[index].yturn_active = true;
    g_letters[index].yturn_step = 1;
}

static void logo_letter_tick_yturn(int index)
{
    logo_sprite_t *letter = &g_letters[index];

    if (!letter->yturn_active) {
        return;
    }

    uint8_t phase;
    bool yflip;

    if (letter->yturn_step <= (EGG_C0_YTURN_PHASES - 1)) {
        phase = letter->yturn_step;
        yflip = letter->stable_yflip;
    } else {
        phase = (uint8_t)(EGG_C0_YTURN_STEPS - letter->yturn_step);
        yflip = !letter->stable_yflip;
    }

    if (phase >= EGG_C0_YTURN_PHASES) {
        phase = (uint8_t)(EGG_C0_YTURN_PHASES - 1);
    }

    letter->addr = g_glyph_yturn_addr[letter->glyph][phase];
    logo_letter_set_yflip(index, yflip);

    if (letter->yturn_step >= EGG_C0_YTURN_STEPS) {
        letter->yturn_active = false;
        letter->stable_yflip = !letter->stable_yflip;
        letter->yturn_step = 0;

        letter->addr = g_glyph_xturn_addr[letter->glyph][0];
        logo_letter_set_yflip(index, letter->stable_yflip);
    } else {
        letter->yturn_step++;
    }
}

static void logo_letter_reset_yturn(int index)
{
    logo_sprite_t *letter = &g_letters[index];

    letter->yturn_active = false;
    letter->yturn_step = 0;
    letter->stable_yflip = false;

    letter->addr = g_glyph_xturn_addr[letter->glyph][0];
    logo_letter_set_yflip(index, false);
}

static void logo_letter_cancel_turns_keep_orientation(int index)
{
    logo_sprite_t *letter = &g_letters[index];

    letter->xturn_active = false;
    letter->xturn_step = 0;
    letter->stable_xflip = (letter->transform & EGG_C0_X_FLIP) != 0;

    letter->yturn_active = false;
    letter->yturn_step = 0;
    letter->stable_yflip = (letter->transform & EGG_C0_Y_FLIP) != 0;

    letter->addr = g_glyph_xturn_addr[letter->glyph][0];
}

/*
 * Visual flip correction rules for the final ENTERPRISE word.
 *
 * During chaos, sprites may finish with SPB X/Y flip bits set.  Some letters
 * are visually symmetric, so forcing every raw flip bit back to zero causes
 * unnecessary card-turns even though the letter already looks correct.
 *
 * Rules:
 *   E: Y flip is visually acceptable; X flip is not.
 *   N: X+Y together is visually acceptable; only X or only Y is not.
 *   T: X flip is visually acceptable; Y flip is not.
 *   I: any flip state is visually acceptable.
 *   others: both X and Y flips must be corrected.
 */
static uint8_t logo_letter_unwanted_flip_mask(int index)
{
    const logo_sprite_t *letter = &g_letters[index];
    bool xflip = (letter->transform & EGG_C0_X_FLIP) != 0;
    bool yflip = (letter->transform & EGG_C0_Y_FLIP) != 0;

    switch (letter->glyph) {
        case EGG_C0_GLYPH_E:
            return xflip ? EGG_C0_X_FLIP : 0u;

        case EGG_C0_GLYPH_N:
            if (xflip && yflip) {
                return 0u;
            }
            return (uint8_t)((xflip ? EGG_C0_X_FLIP : 0u) |
                             (yflip ? EGG_C0_Y_FLIP : 0u));

        case EGG_C0_GLYPH_T:
            return yflip ? EGG_C0_Y_FLIP : 0u;

        case EGG_C0_GLYPH_I:
            return 0u;

        default:
            return (uint8_t)((xflip ? EGG_C0_X_FLIP : 0u) |
                             (yflip ? EGG_C0_Y_FLIP : 0u));
    }
}

static inline bool logo_letter_needs_x_correction(int index)
{
    return (logo_letter_unwanted_flip_mask(index) & EGG_C0_X_FLIP) != 0;
}

static inline bool logo_letter_needs_y_correction(int index)
{
    return (logo_letter_unwanted_flip_mask(index) & EGG_C0_Y_FLIP) != 0;
}

static void logo_reset_all_turns(void)
{
    for (uint8_t i = 0u; i < EGG_C0_LETTER_COUNT; ++i) {
        logo_letter_reset_xturn(i);
        logo_letter_reset_yturn(i);
    }
}

static void logo_2dfx_reset_drop(void)
{
    g_2dfx_drop_started = false;
    g_2dfx_drop_settled = false;
    g_2dfx_y_fp = ((int32_t)EGG_C0_2DFX_START_Y << EGG_C0_FIXED_POINT_SHIFT);
    g_2dfx_vy_fp = EGG_C0_2DFX_INITIAL_VELOCITY;
}

static void logo_2dfx_start_drop(void)
{
    if (g_2dfx_drop_started) {
        return;
    }

    g_2dfx_drop_started = true;
    g_2dfx_drop_settled = false;
    g_2dfx_y_fp = ((int32_t)EGG_C0_2DFX_START_Y << EGG_C0_FIXED_POINT_SHIFT);
    g_2dfx_vy_fp = EGG_C0_2DFX_INITIAL_VELOCITY;
}

static void logo_2dfx_tick_drop(void)
{
    if (!g_2dfx_drop_started) {
        logo_2dfx_start_drop();
    }

    if (g_2dfx_drop_settled) {
        return;
    }

    g_2dfx_vy_fp += EGG_C0_2DFX_GRAVITY;
    g_2dfx_y_fp += g_2dfx_vy_fp;

    const int32_t target_y_fp = ((int32_t)logo_2dfx_target_y() << EGG_C0_FIXED_POINT_SHIFT);

    if (g_2dfx_y_fp >= target_y_fp) {
        g_2dfx_y_fp = target_y_fp;

        if (g_2dfx_vy_fp > EGG_C0_2DFX_SETTLE_VELOCITY) {
            g_2dfx_vy_fp = -(
                (g_2dfx_vy_fp * EGG_C0_2DFX_BOUNCE_NUMERATOR) /
                EGG_C0_2DFX_BOUNCE_DENOMINATOR);
        } else {
            g_2dfx_vy_fp = 0;
            g_2dfx_drop_settled = true;
        }
    }
}

static uint16_t logo_2dfx_current_y(void)
{
    int32_t y = g_2dfx_y_fp >> EGG_C0_FIXED_POINT_SHIFT;

    if (y < 0) {
        y = 0;
    }

    if (y > (int32_t)EGG_C0_RAW_COORDINATE_MAX) {
        y = (int32_t)EGG_C0_RAW_COORDINATE_MAX;
    }

    return (uint16_t)y;
}

static void install_assets(void)
{
    uint32_t address = EGG_C0_RESOURCE_BASE;

    /*
     * Install each unique glyph once, followed by its generated horizontal
     * and vertical card-turn phases. Repeated letters share these addresses.
     */
    for (uint8_t glyph = 0u;
         glyph < EGG_C0_UNIQUE_GLYPH_COUNT;
         ++glyph) {
        const logo_asset_t *asset = &unique_letter_assets[glyph];
        uint32_t bytes_per_row = (uint32_t)((asset->w + 1u) >> 1);
        uint32_t bytes_total = bytes_per_row * asset->h;

        g_glyph_xturn_addr[glyph][0] = address;
        g_glyph_yturn_addr[glyph][0] = address;
        convert_source_asset(g_asset_work, asset->data, bytes_total);
        twodfx_upload_raw(address, g_asset_work, bytes_total);
        address += bytes_total;

        for (uint8_t phase = 1u; phase < EGG_C0_XTURN_PHASES; ++phase) {
            g_glyph_xturn_addr[glyph][phase] = address;
            build_horizontal_turn_phase(g_asset_work,
                                        asset->data,
                                        asset->w,
                                        asset->h,
                                        phase);
            twodfx_upload_raw(address, g_asset_work, bytes_total);
            address += bytes_total;
        }

        for (uint8_t phase = 1u; phase < EGG_C0_YTURN_PHASES; ++phase) {
            g_glyph_yturn_addr[glyph][phase] = address;
            build_vertical_turn_phase(g_asset_work,
                                      asset->data,
                                      asset->w,
                                      asset->h,
                                      phase);
            twodfx_upload_raw(address, g_asset_work, bytes_total);
            address += bytes_total;
        }
    }

    /* Ten independent letter sprites reference the shared glyph phase banks. */
    for (uint8_t i = 0u; i < EGG_C0_LETTER_COUNT; ++i) {
        uint8_t glyph = logo_word_glyph[i];
        const logo_asset_t *asset = &unique_letter_assets[glyph];

        g_letters[i].glyph = glyph;
        g_letters[i].addr = g_glyph_xturn_addr[glyph][0];
        g_letters[i].w = asset->w;
        g_letters[i].h = asset->h;
        g_letters[i].transform = EGG_C0_X_DOUBLE;
        g_letters[i].xturn_active = false;
        g_letters[i].xturn_step = 0u;
        g_letters[i].stable_xflip = false;
        g_letters[i].yturn_active = false;
        g_letters[i].yturn_step = 0u;
        g_letters[i].stable_yflip = false;
    }

    /*
     * Install the six rainbow stripes. Enterprise retains the established
     * source artwork byte-for-byte. TVC uses vivid fixed-palette colours; its
     * orange substitute is a 96x6 single-pixel red/yellow checkerboard.
     */
    static const uint8_t tvc_solid_colours[EGG_C0_STRIPE_COUNT] = {
        EGG_C0_TVC_STRIPE_RED_COLOUR,
        0u, /* checkerboard is generated separately */
        EGG_C0_TVC_STRIPE_YELLOW_COLOUR,
        EGG_C0_TVC_STRIPE_GREEN_COLOUR,
        EGG_C0_TVC_STRIPE_BLUE_COLOUR,
        EGG_C0_TVC_STRIPE_MAGENTA_COLOUR
    };

    for (uint8_t i = 0u; i < EGG_C0_STRIPE_COUNT; ++i) {
        uint16_t stripe_w = EGG_C0_STRIPE_WIDTH;
        uint8_t transform = EGG_C0_X_DOUBLE;
        uint32_t bytes_total;

        if (g_is_tvc && i == 1u) {
            stripe_w = EGG_C0_TVC_ORANGE_STRIPE_WIDTH;
            transform = 0u;
            bytes_total = ((uint32_t)stripe_w >> 1) * EGG_C0_STRIPE_HEIGHT;
            memset(g_asset_work, 0, bytes_total);

            for (uint16_t y = 0u; y < EGG_C0_STRIPE_HEIGHT; ++y) {
                for (uint16_t x = 0u; x < stripe_w; ++x) {
                    uint8_t colour = ((x ^ y) & 1u)
                        ? EGG_C0_TVC_STRIPE_ORANGE_A_COLOUR
                        : EGG_C0_TVC_STRIPE_ORANGE_B_COLOUR;
                    work_buffer_set_pixel(g_asset_work, stripe_w, x, y, colour);
                }
            }
        } else if (g_is_tvc) {
            bytes_total = ((uint32_t)stripe_w >> 1) * EGG_C0_STRIPE_HEIGHT;
            uint8_t colour = tvc_solid_colours[i];
            memset(g_asset_work, (uint8_t)(colour | (colour << 4)), bytes_total);
        } else {
            uint32_t bytes_per_row =
                (uint32_t)((stripe_assets[i].w + 1u) >> 1);
            bytes_total = bytes_per_row * stripe_assets[i].h;
            convert_source_asset(g_asset_work, stripe_assets[i].data, bytes_total);
        }

        twodfx_upload_raw(address, g_asset_work, bytes_total);

        g_strips[i].addr = address;
        g_strips[i].w = stripe_w;
        g_strips[i].h = EGG_C0_STRIPE_HEIGHT;
        g_strips[i].transform = transform;

        address += bytes_total;
    }

    /*
     * Install the shared 515x198 2dfx logo one packed row at a time. TVC uses
     * fixed white/red palette indexes; Enterprise keeps the source colours.
     */
    for (uint16_t y = 0u; y < LOGO_2DFX_HEIGHT; ++y) {
        const uint8_t *source_row =
            &logo_2dfx_data[(uint32_t)y * LOGO_2DFX_BYTES_PER_ROW];

        for (uint16_t byte_x = 0u;
             byte_x < LOGO_2DFX_BYTES_PER_ROW;
             ++byte_x) {
            uint8_t recoloured =
                recolour_large_logo_source_pair(source_row[byte_x]);
            g_logo_row_work[byte_x] =
                internal_pair_from_source_pair(recoloured);
        }

        twodfx_upload_raw(address +
                              (uint32_t)y * LOGO_2DFX_BYTES_PER_ROW,
                          g_logo_row_work,
                          sizeof(g_logo_row_work));
    }

    g_logo_2dfx.addr = address;
    g_logo_2dfx.w = LOGO_2DFX_WIDTH;
    g_logo_2dfx.h = LOGO_2DFX_HEIGHT;
    g_logo_2dfx.transform = 0u;
}

static void setup_targets(void)
{
    const uint16_t screen_width = active_screen_width();
    const uint16_t first_visible_y = active_visible_first_y();

    int total_letter_width = 0;
    for (uint8_t i = 0u; i < EGG_C0_LETTER_COUNT; ++i) {
        total_letter_width += logo_display_w(&g_letters[i]);
        total_letter_width += EGG_C0_LETTER_ADVANCE_GAP - 1;
    }

    int base_x = ((int)screen_width - total_letter_width) / 2;
    int base_y = (int)first_visible_y + EGG_C0_LETTER_BASE_Y_OFFSET;
    int x = base_x;

    for (uint8_t i = 0u; i < EGG_C0_LETTER_COUNT; ++i) {
        g_letters[i].tx = (uint16_t)x;
        g_letters[i].ty = (uint16_t)base_y;
        x += logo_display_w(&g_letters[i]);
        x += EGG_C0_LETTER_ADVANCE_GAP;
    }

    int stripe_total_width = 0;
    for (uint8_t i = 0u; i < EGG_C0_STRIPE_COUNT; ++i) {
        stripe_total_width += logo_display_w(&g_strips[i]);
        stripe_total_width += EGG_C0_STRIPE_ADVANCE_GAP - 1;
    }

    int stripe_x = ((int)screen_width - stripe_total_width) / 2;
    int stripe_y = base_y + EGG_C0_STRIPE_Y_OFFSET;
    x = stripe_x;

    for (uint8_t i = 0u; i < EGG_C0_STRIPE_COUNT; ++i) {
        g_strips[i].tx = (uint16_t)x;
        g_strips[i].ty = (uint16_t)stripe_y;
        g_strips[i].x = g_strips[i].tx;
        g_strips[i].y = g_strips[i].ty;
        x += logo_display_w(&g_strips[i]);
        x += EGG_C0_STRIPE_ADVANCE_GAP;
    }
}

static uint32_t rnd32(void)
{
    g_rng = g_rng * EGG_C0_RNG_MULTIPLIER + EGG_C0_RNG_INCREMENT;
    return g_rng;
}

static void logo_seed_rng(void)
{
    /*
     * Avoid the egg starting with exactly the same movement pattern after
     * every reset. get_rand_32() gives us RP hardware randomness, while
     * time_us_32() adds a little extra variation depending on when the egg
     * command was issued.
     */
    g_rng = get_rand_32() ^ time_us_32() ^ EGG_C0_RNG_SEED_XOR;

    if (g_rng == 0) {
        g_rng = EGG_C0_RNG_INITIAL_FALLBACK;
    }
}

static int16_t logo_random_velocity(void)
{
    int16_t v = (int16_t)((rnd32() % EGG_C0_RANDOM_SPEED_RANGE) +
                          EGG_C0_RANDOM_SPEED_MIN);

    if (rnd32() & 1u) {
        v = (int16_t)-v;
    }

    return v;
}

static void randomize_letter_positions(void)
{
    for (uint8_t i = 0u; i < EGG_C0_LETTER_COUNT; ++i) {
        uint16_t dw = logo_display_w(&g_letters[i]);
        uint16_t dh = logo_display_h(&g_letters[i]);

        g_letters[i].x = logo_random_coord_inside(active_chaos_min_x(), active_chaos_max_x(), dw);
        g_letters[i].y = logo_random_coord_inside(active_chaos_min_y(), active_chaos_max_y(), dh);

        g_letters[i].vx = logo_random_velocity();
        g_letters[i].vy = logo_random_velocity();

        /* Avoid a very boring perfectly diagonal start for narrow letters. */
        if (g_letters[i].vx == g_letters[i].vy) {
            g_letters[i].vy = (int16_t)-g_letters[i].vy;
        }
    }
}

static void clear_logo_sprites(void)
{
    /*
     * Preserve the original per-frame behavior: clear only the eighteen sprite
     * slots owned by C0, leaving any host-created higher-numbered sprites alone.
     */
    for (uint8_t sprite = EGG_C0_LETTER_SPRITE_BASE;
         sprite < EGG_C0_USED_SPRITE_COUNT;
         ++sprite) {
        twodfx_sprite_set(sprite,
                          0u, 0u, 0u, 0u, 0u,
                          false, false, 0u);
    }
}

static void commit_logo_scene(void)
{
    const uint16_t screen_width = active_screen_width();
    const uint16_t first_visible_y = active_visible_first_y();
    const uint16_t visible_height = active_visible_height();

    clear_logo_sprites();

    for (uint8_t i = 0u; i < EGG_C0_LETTER_COUNT; ++i) {
        twodfx_sprite_set(
            (uint8_t)(EGG_C0_LETTER_SPRITE_BASE + i),
            g_letters[i].addr,
            logo_apply_global_x_offset(g_letters[i].x),
            g_letters[i].y,
            g_letters[i].w,
            g_letters[i].h,
            true,
            i == EGG_C0_COLLISION_LETTER_INDEX,
            g_letters[i].transform);
    }

    for (uint8_t i = 0u; i < EGG_C0_STRIPE_COUNT; ++i) {
        twodfx_sprite_set(
            (uint8_t)(EGG_C0_STRIPE_SPRITE_BASE + i),
            g_strips[i].addr,
            logo_apply_global_x_offset(g_strips[i].x),
            g_strips[i].y,
            g_strips[i].w,
            g_strips[i].h,
            true,
            i == EGG_C0_COLLISION_STRIPE_INDEX,
            g_strips[i].transform);
    }

    /* Show the previous completed frame's collision result as a small stripe. */
    if (twodfx_collision_detected()) {
        const logo_sprite_t *marker =
            &g_strips[EGG_C0_COLLISION_STRIPE_INDEX];
        uint16_t marker_x =
            (uint16_t)((screen_width - logo_display_w(marker)) / 2u);
        uint16_t marker_y = (uint16_t)(
            first_visible_y +
            ((uint32_t)visible_height *
             EGG_C0_COLLISION_MARKER_Y_NUMERATOR) /
                EGG_C0_COLLISION_MARKER_Y_DENOMINATOR);

        twodfx_sprite_set(EGG_C0_COLLISION_MARKER_SPRITE,
                          marker->addr,
                          logo_apply_global_x_offset(marker_x),
                          marker_y,
                          marker->w,
                          marker->h,
                          true,
                          false,
                          marker->transform);
    }

    /* Publish the large falling logo only after its delayed drop has started. */
    if ((g_phase == EGG_C0_PHASE_SETTLE && g_2dfx_drop_started) ||
        g_phase == EGG_C0_PHASE_RAINBOW) {
        uint16_t logo_x =
            (uint16_t)((screen_width - LOGO_2DFX_WIDTH) / 2u);

        twodfx_sprite_set(EGG_C0_2DFX_LOGO_SPRITE,
                          g_logo_2dfx.addr,
                          logo_apply_global_x_offset(logo_x),
                          logo_2dfx_current_y(),
                          g_logo_2dfx.w,
                          g_logo_2dfx.h,
                          true,
                          false,
                          g_logo_2dfx.transform);
    }
}

static void tick_chaos(void)
{
    for (uint8_t i = 0u; i < EGG_C0_LETTER_COUNT; ++i) {
        int32_t nx = (int32_t)g_letters[i].x + g_letters[i].vx;
        int32_t ny = (int32_t)g_letters[i].y + g_letters[i].vy;

        uint16_t dw = logo_display_w(&g_letters[i]);
        uint16_t dh = logo_display_h(&g_letters[i]);

        if (nx < active_chaos_min_x() ||
            nx > (active_chaos_max_x() - (int32_t)dw)) {
            g_letters[i].vx = -g_letters[i].vx;

            /*
             * Smooth horizontal turn for every letter.  The direction changes
             * immediately, but the visible X-flip happens through generated
             * card-turn phases instead of a single instant flip.
             */
            logo_letter_start_xturn(i);

            nx = (int32_t)g_letters[i].x + g_letters[i].vx;
        }
        if (ny < active_chaos_min_y() ||
            ny > (active_chaos_max_y() - (int32_t)dh)) {
            g_letters[i].vy = -g_letters[i].vy;

            /*
             * Smooth vertical turn. If a horizontal turn is already active,
             * the start function will deliberately ignore this bounce to avoid
             * two phase-address animations fighting over the same SPB address.
             */
            logo_letter_start_yturn(i);

            ny = (int32_t)g_letters[i].y + g_letters[i].vy;
        }

        g_letters[i].x = (uint16_t)nx;
        g_letters[i].y = (uint16_t)ny;

        logo_letter_tick_xturn(i);
        logo_letter_tick_yturn(i);
    }

    g_phase_timer++;
    if (g_phase_timer > EGG_C0_CHAOS_FRAMES) {
        g_phase = EGG_C0_PHASE_ASSEMBLE;
        g_phase_timer = 0;
    }
}

static void tick_assemble(void)
{
    bool all_close = true;

    for (uint8_t i = 0u; i < EGG_C0_LETTER_COUNT; ++i) {
        int16_t dx = (int16_t)g_letters[i].tx - (int16_t)g_letters[i].x;
        int16_t dy = (int16_t)g_letters[i].ty - (int16_t)g_letters[i].y;

        if (dx >= 1) g_letters[i].x++;
        else if (dx <= -1) g_letters[i].x--;

        if (dy >= 1) g_letters[i].y++;
        else if (dy <= -1) g_letters[i].y--;

        if (g_letters[i].xturn_active) {
            logo_letter_tick_xturn(i);
        }

        if (g_letters[i].yturn_active) {
            logo_letter_tick_yturn(i);
        }

        if (dx >= 2 || dx <= -2 || dy >= 2 || dy <= -2 ||
            g_letters[i].xturn_active || g_letters[i].yturn_active) {
            all_close = false;
        }
    }

    if (all_close) {
       /*
        * The chaos phase may leave letters flipped. Snap them to the final
        * ENTERPRISE positions, but do not clear the flip bits instantly.
        * The next phase restores readability with a left-to-right animated
        * flip-back sequence.
        */
       for (uint8_t i = 0u; i < EGG_C0_LETTER_COUNT; ++i) {
            g_letters[i].x = g_letters[i].tx;
            g_letters[i].y = g_letters[i].ty;
            logo_letter_cancel_turns_keep_orientation(i);
        }

        g_flipback_index = 0;
        g_flipback_wait = 0;
        g_phase = EGG_C0_PHASE_FLIPBACK;
        g_phase_timer = 0;
    }
}

static void tick_flipback(void)
{
    while (g_flipback_index < EGG_C0_LETTER_COUNT) {
        logo_sprite_t *letter = &g_letters[g_flipback_index];

        if (g_flipback_wait > 0) {
            g_flipback_wait--;
            return;
        }

        /* Continue an already-started correction animation. */
        if (letter->xturn_active) {
            logo_letter_tick_xturn(g_flipback_index);

            if (!letter->xturn_active) {
                if (logo_letter_needs_y_correction(g_flipback_index)) {
                    logo_letter_start_yturn(g_flipback_index);
                } else {
                    g_flipback_wait = EGG_C0_FLIPBACK_WAIT_FRAMES;
                }
            }
            return;
        }

        if (letter->yturn_active) {
            logo_letter_tick_yturn(g_flipback_index);

            if (!letter->yturn_active) {
                g_flipback_wait = EGG_C0_FLIPBACK_WAIT_FRAMES;
            }
            return;
        }

        /*
         * Waiting finished after a visually-corrected letter; move to the next
         * one.  "Visually corrected" is not always the same as "raw SPB flip
         * bits are zero" because E/N/T/I have useful symmetries.
         */
        if (logo_letter_unwanted_flip_mask(g_flipback_index) == 0u) {
            g_flipback_index++;
            continue;
        }

        /*
         * Correct only the axes that are visually necessary.
         *
         * Examples:
         *   E with Y flip only: keep it.
         *   N with both X+Y flips: keep it.
         *   T with X flip only: keep it.
         *   I with any flip state: keep it.
         */
        if (logo_letter_needs_x_correction(g_flipback_index)) {
            logo_letter_start_xturn(g_flipback_index);
            return;
        }

        if (logo_letter_needs_y_correction(g_flipback_index)) {
            logo_letter_start_yturn(g_flipback_index);
            return;
        }
    }

    logo_2dfx_reset_drop();
    g_phase = EGG_C0_PHASE_SETTLE;
    g_phase_timer = 0;
}

static void tick_settle(void)
{
    /*
     * Let the assembled ENTERPRISE logo sit for about ten seconds before the
     * 2dfx gravity-drop starts.  During this delay the 2dfx logo is not emitted
     * by commit_logo_scene(), so the viewer sees only the settled word/stripes.
     */
    g_phase_timer++;

    if (g_phase_timer <= EGG_C0_2DFX_PRE_DROP_WAIT_FRAMES) {
        return;
    }

    logo_2dfx_tick_drop();

    if (g_2dfx_drop_settled) {
        g_phase = EGG_C0_PHASE_RAINBOW;
        g_phase_timer = 0;
    }
}

static void tick_rainbow(void)
{
    logo_2dfx_tick_drop();

    g_phase_timer++;

    /* Rotate the stripe positions so the colours circulate forever. */
    if (!(g_phase_timer % EGG_C0_RAINBOW_SWAP_FRAMES)) {
        uint16_t temp = g_strips[0].x;
        g_strips[0].x = g_strips[1].x;
        g_strips[1].x = g_strips[2].x;
        g_strips[2].x = g_strips[3].x;
        g_strips[3].x = g_strips[4].x;
        g_strips[4].x = g_strips[5].x;
        g_strips[5].x = temp;
    }
}

void egg_C0_logo_start(void)
{
    g_is_tvc = (twodfx_get_machine() == TWODFX_MACHINE_TVC);
    twodfx_reset_sprites();

    logo_seed_rng();
    install_assets();
    setup_targets();
    randomize_letter_positions();
    logo_reset_all_turns();
    logo_2dfx_reset_drop();

    g_active = true;
    g_phase = EGG_C0_PHASE_CHAOS;
    g_phase_timer = 0;

    commit_logo_scene();
}

void egg_C0_logo_tick(void)
{
    if (!g_active) return;

    switch (g_phase) {
        case EGG_C0_PHASE_CHAOS:    tick_chaos();    break;
        case EGG_C0_PHASE_ASSEMBLE: tick_assemble(); break;
        case EGG_C0_PHASE_FLIPBACK: tick_flipback();  break;
        case EGG_C0_PHASE_SETTLE:   tick_settle();   break;
        case EGG_C0_PHASE_RAINBOW:  tick_rainbow();  break;
        default: break;
    }

    commit_logo_scene();
}
