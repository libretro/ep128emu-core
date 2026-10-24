#ifndef TWODFX_EGG_H
#define TWODFX_EGG_H

/*
 * 2dfx firmware egg registration, lifecycle and interactive input.
 *
 * Graphics, resources and renderer operations belong to twodfx_api.h. This
 * header only describes how compile-time firmware extensions are registered,
 * activated and supplied with standalone idle/unclaimed F9h input bytes.
 */

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Number of firmware egg command slots; valid egg IDs are 0..31. */
#define TWODFX_EGG_COUNT 32u

/** Machine-availability flags for one registry entry. */
#define TWODFX_EGG_MACHINE_ENTERPRISE 0x01u
#define TWODFX_EGG_MACHINE_TVC        0x02u
#define TWODFX_EGG_MACHINE_BOTH \
    (TWODFX_EGG_MACHINE_ENTERPRISE | TWODFX_EGG_MACHINE_TVC)

/**
 * Egg initialization callback.
 *
 * It is called once on core0 when a requested egg reaches the next safe scene-
 * update point, immediately after the egg manager has restored clean
 * power-on-style public graphics defaults. Use it to select any non-default
 * global modes, upload assets, define descriptors and construct the initial
 * shadow scene.
 *
 * It must not sleep or busy-wait for a long asynchronous operation. If it
 * starts RRAM clear or UNZX1 work, store an initialization phase in the egg's
 * own static state and continue from tick after the resource-job status reports
 * completion.
 */
typedef void (*twodfx_egg_start_fn)(void);

/**
 * Egg scene-update callback.
 *
 * It is called on accepted core0 scene-update opportunities according to the
 * selected 25/50 fps cadence, immediately before shadow state is published for
 * the next render. A tick can be delayed while an earlier render or resource
 * job remains busy, so it is not a guaranteed wall-clock timer.
 *
 * Only changing state normally belongs here. A sprite, descriptor or 2DPT slot
 * rewritten on every tick will overwrite a host-side F8h/F9h tweak of the same
 * object on the next tick. A static egg may register a NULL tick callback.
 */
typedef void (*twodfx_egg_tick_fn)(void);

/** One compile-time egg registration entry; registry array index is the ID. */
typedef struct {
    /** OR combination of TWODFX_EGG_MACHINE_ENTERPRISE/TVC. */
    uint8_t machine_flags;

    /** Required initialization callback; NULL means the slot is unavailable. */
    twodfx_egg_start_fn start;

    /** Optional scene-update callback; may be NULL for a static egg. */
    twodfx_egg_tick_fn tick;
} twodfx_egg_definition_t;

/**
 * Compile-time 32-entry egg registry defined in twodfx_egg_registry.c.
 *
 * Selecting any entry immediately begins a fresh pending input stream. At the
 * deferred safe point, a registered, machine-compatible entry deactivates the
 * previous egg, calls start once and then schedules tick normally. Selecting
 * an unavailable or incompatible slot leaves no egg active.
 *
 * Before every valid start callback, the manager automatically restores 50 Hz
 * updates, normal engine mode, transparent index 0, hidden built-in logo,
 * disabled raster interrupts, cleared overrun/collision state, no sprites,
 * empty background/foreground 2DPTs and empty resource descriptors. Resource
 * RAM bytes are retained intentionally, but cannot be referenced through stale
 * descriptors or scene objects.
 */
extern const twodfx_egg_definition_t
    twodfx_egg_registry[TWODFX_EGG_COUNT];

/**
 * Reads and removes the oldest pending idle/unclaimed F9h byte.
 *
 * @param value
 *     Receives one byte when available. NULL returns false.
 *
 * @return
 *     true when a byte was returned; false immediately when the FIFO is empty.
 *
 * This operation is nonblocking, bounded-time and safe from normal core0 egg
 * code or core0 interrupt context. Bytes are returned in original order.
 *
 * Only writes already reaching the command parser's existing idle/unclaimed
 * F9h path are queued. Parameters and payload bytes consumed by uploads,
 * descriptor commands, 2DPT commands, ALTER_SPB or other active sessions are
 * not mirrored. Existing command-session behavior is otherwise unchanged.
 *
 * Selecting an egg immediately begins a fresh pending input stream, so an
 * idle F9h byte written after the C0h..DFh start command is retained even
 * before the deferred start callback runs. The previous egg cannot consume
 * that pending stream. Selecting another egg replaces it, and stopping egg
 * mode clears it.
 */
bool twodfx_egg_read_f9(uint8_t *value);

/**
 * Returns the sticky egg-input overflow indication.
 *
 * @return
 *     true if at least one incoming byte was discarded because the FIFO was
 *     full; false otherwise.
 *
 * When the finite FIFO is full, already queued bytes are preserved and the
 * newest arriving byte is discarded. The indication remains set until it is
 * explicitly cleared or egg input state is reset by a new selection/stop.
 */
bool twodfx_egg_f9_overflowed(void);

/** Clears the sticky egg-input FIFO overflow indication. */
void twodfx_egg_clear_f9_overflow(void);

#ifdef __cplusplus
}
#endif

#endif /* TWODFX_EGG_H */
