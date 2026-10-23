#ifndef EGG_D4_3D_SCOPE_H
#define EGG_D4_3D_SCOPE_H
#include <stdbool.h>
#include <stdint.h>
/* D4: diagnostic only, OUT 248,212. No new public drawing commands. */
void egg_D4_3d_scope_start(void);
void egg_D4_3d_scope_tick(void);
void egg_D4_3d_scope_step(void);
bool egg_D4_3d_scope_ready(void);
void egg_D4_3d_scope_stop(void);
#endif
