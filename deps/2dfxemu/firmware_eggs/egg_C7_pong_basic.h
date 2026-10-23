#ifndef EGG_C7_PONG_BASIC_H
#define EGG_C7_PONG_BASIC_H

/*
 * Egg C7 — classic Pong
 * =====================
 *
 * Start from the host computer with:
 *
 *     OUT 248,199        (199 = C7h)
 *
 * Purpose
 * -------
 * Demonstrates a small interactive game built only from the public C API:
 *
 * - three generated sprites for the two paddles and the ball;
 * - a background 2DPT made from SET_XY_OFFSET, RECT and FILL_RECT commands;
 * - seven-segment scores assembled from ordinary filled rectangles;
 * - ordered, nonblocking idle-F9 input through twodfx_egg_read_f9();
 * - frame-by-frame sprite and 2DPT updates without private renderer access.
 *
 * What appears on screen
 * ----------------------
 * A deliberately simple retro Pong court. Q/A move the left paddle, O/L move
 * the right paddle and N starts a new game. When a point is scored, SET_XY_OFFSET
 * briefly shakes the complete 2DPT court while the sprites stay independent.
 *
 * Input
 * -----
 * Write one bit-mask byte with OUT 249,value while no normal 2dfx command
 * session is consuming F9h:
 *
 *     bit 7  N  new game, edge-triggered
 *     bit 3  Q  left paddle up
 *     bit 2  A  left paddle down
 *     bit 1  O  right paddle up
 *     bit 0  L  right paddle down
 *
 * The latest queued byte is treated as the current held-key state.
 *
 * Implementation style
 * --------------------
 * The .c file accesses 2dfx only through twodfx_api.h and twodfx_egg.h. Sprite
 * graphics are assembled in ordinary MCU arrays and uploaded with
 * twodfx_upload_raw(). Every 2DPT and sprite operation is a visible public API
 * call; the egg does not touch resource_ram, raw SPBs/2DPBs or shadow tables.
 * All tunable resource, geometry, timing, input and colour constants live here.
 */

/* ------------------------------------------------------------------------- */
/* Resource allocation and sprite slots                                      */
/* ------------------------------------------------------------------------- */

#define EGG_C7_LEFT_PADDLE_SPRITE  0u
#define EGG_C7_RIGHT_PADDLE_SPRITE 1u
#define EGG_C7_BALL_SPRITE         2u

#define EGG_C7_LEFT_PADDLE_ADDRESS  0x400000u
#define EGG_C7_RIGHT_PADDLE_ADDRESS 0x400400u
#define EGG_C7_BALL_ADDRESS         0x400800u

/* ------------------------------------------------------------------------- */
/* Machine-specific playfield geometry                                       */
/* ------------------------------------------------------------------------- */

/* Original Enterprise playfield. */
#define EGG_C7_EP_FIELD_X      128u
#define EGG_C7_EP_FIELD_Y       48u
#define EGG_C7_EP_FIELD_WIDTH  634u
#define EGG_C7_EP_FIELD_HEIGHT 212u

/* TVC playfield kept inside the measured VT-BASIC-safe area during shake. */
#define EGG_C7_TVC_FIELD_X      132u
#define EGG_C7_TVC_FIELD_Y       53u
#define EGG_C7_TVC_FIELD_WIDTH  500u
#define EGG_C7_TVC_FIELD_HEIGHT 232u

/* Paddle and ball source dimensions. */
#define EGG_C7_PADDLE_WIDTH     12u
#define EGG_C7_PADDLE_HEIGHT    48u
#define EGG_C7_BALL_WIDTH       12u
#define EGG_C7_BALL_HEIGHT       6u

/* Insets used to derive moving-object limits from the selected playfield. */
#define EGG_C7_PADDLE_X_INSET        8u
#define EGG_C7_PADDLE_Y_INSET       10u
#define EGG_C7_BALL_WALL_INSET       4u
#define EGG_C7_CENTER_DASH_X_OFFSET  3u
#define EGG_C7_CENTER_DASH_TOP      12u
#define EGG_C7_CENTER_DASH_BOTTOM   12u
#define EGG_C7_CENTER_DASH_STEP     20u
#define EGG_C7_CENTER_DASH_WIDTH     6u
#define EGG_C7_CENTER_DASH_HEIGHT   10u

/* Seven-segment score placement relative to the selected playfield centre. */
#define EGG_C7_SCORE_Y_OFFSET      24u
#define EGG_C7_LEFT_SCORE_X_OFFSET 108u
#define EGG_C7_RIGHT_SCORE_X_OFFSET 44u

/* ------------------------------------------------------------------------- */
/* Input bits                                                                */
/* ------------------------------------------------------------------------- */

#define EGG_C7_INPUT_NEW_GAME         0x80u
#define EGG_C7_INPUT_LEFT_PADDLE_UP    0x08u
#define EGG_C7_INPUT_LEFT_PADDLE_DOWN  0x04u
#define EGG_C7_INPUT_RIGHT_PADDLE_UP   0x02u
#define EGG_C7_INPUT_RIGHT_PADDLE_DOWN 0x01u

/* ------------------------------------------------------------------------- */
/* Game timing and motion                                                    */
/* ------------------------------------------------------------------------- */

#define EGG_C7_PADDLE_SPEED       5
#define EGG_C7_BALL_SPEED_X       4
#define EGG_C7_BALL_INITIAL_DY    3
#define EGG_C7_BALL_MIN_DY        1
#define EGG_C7_PADDLE_DEFLECT_DIV 8
#define EGG_C7_SERVE_DELAY_TICKS 30u
#define EGG_C7_SCORE_FLASH_TICKS 20u
#define EGG_C7_SHAKE_ACTIVE_ABOVE 8u
#define EGG_C7_SCORE_MODULUS      10u

/* Eight-frame SET_XY_OFFSET score-shake sequences. */
#define EGG_C7_SHAKE_X_VALUES { -4, 3, -3, 4, -2, 2, -1, 1 }
#define EGG_C7_SHAKE_Y_VALUES { -2, 1,  2,-1,  1,-2,  1,-1 }
#define EGG_C7_SHAKE_PHASE_MASK 7u

/* ------------------------------------------------------------------------- */
/* Seven-segment digit geometry and masks                                    */
/* ------------------------------------------------------------------------- */

#define EGG_C7_SEGMENT_A 0x01u
#define EGG_C7_SEGMENT_B 0x02u
#define EGG_C7_SEGMENT_C 0x04u
#define EGG_C7_SEGMENT_D 0x08u
#define EGG_C7_SEGMENT_E 0x10u
#define EGG_C7_SEGMENT_F 0x20u
#define EGG_C7_SEGMENT_G 0x40u

#define EGG_C7_DIGIT_MASKS \
    { 0x3Fu, 0x06u, 0x5Bu, 0x4Fu, 0x66u, \
      0x6Du, 0x7Du, 0x07u, 0x7Fu, 0x6Fu }

#define EGG_C7_DIGIT_V_BAR_WIDTH       8u
#define EGG_C7_DIGIT_H_BAR_LENGTH     48u
#define EGG_C7_DIGIT_H_BAR_HEIGHT  4u
#define EGG_C7_DIGIT_V_BAR_HEIGHT 16u

/* ------------------------------------------------------------------------- */
/* Colours and strokes                                                       */
/* ------------------------------------------------------------------------- */

#define EGG_C7_TRANSPARENT_COLOUR 0u

#define EGG_C7_EP_FIELD_COLOUR 6u
#define EGG_C7_EP_SCORE_COLOUR 4u
#define EGG_C7_EP_FLASH_COLOUR 5u

#define EGG_C7_TVC_FIELD_COLOUR 14u
#define EGG_C7_TVC_SCORE_COLOUR  4u
#define EGG_C7_TVC_FLASH_COLOUR  2u

#define EGG_C7_EP_SPRITE_COLOUR  7u
#define EGG_C7_TVC_SPRITE_COLOUR 15u

#define EGG_C7_FIELD_STROKE 1u

void egg_C7_pong_basic_start(void);
void egg_C7_pong_basic_tick(void);

#endif /* EGG_C7_PONG_BASIC_H */
