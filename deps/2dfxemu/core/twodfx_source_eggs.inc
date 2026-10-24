/* SPDX-License-Identifier: MIT
 * Portable lifecycle adapter; executes original v1.1.0 egg source callbacks.
 * Stage scheduling is emulated logically, NOT as MCU wall-time or render overrun.
 */
#include "../firmware_eggs/egg_C0_logo.h"
#include "../firmware_eggs/egg_C1_benchmark.h"
#include "../firmware_eggs/egg_C2_primitives.h"
#include "../firmware_eggs/egg_C3_text.h"
#include "../firmware_eggs/egg_C4_blit.h"
#include "../firmware_eggs/egg_C5_pattern.h"
#include "../firmware_eggs/egg_C6_tilemap.h"
#include "../firmware_eggs/egg_C7_pong_basic.h"
#include "../firmware_eggs/egg_C8_pong_wow.h"
#include "../firmware_eggs/egg_C9_wriggler.h"
#include "../firmware_eggs/egg_CA_boulder.h"
#include "../firmware_eggs/egg_CB_arcpie_benchmark.h"
#include "../firmware_eggs/egg_CC_diag.h"
#include "../firmware_eggs/egg_CD_blit_full.h"
#include "../firmware_eggs/egg_CE_layer.h"
#include "../firmware_eggs/egg_CF_tvc_bitpattern.h"
#include "../firmware_eggs/egg_D0_tvc_crisp_tearing.h"
#include "../firmware_eggs/egg_D1_ruler.h"
#include "../firmware_eggs/egg_D2_xy_offset.h"
#include "../firmware_eggs/egg_D3_elite_showcase.h"
#include "../firmware_eggs/egg_D4_3d_scope.h"
#include "../firmware_eggs/egg_D5_public_mesh.h"
#include "../firmware_eggs/egg_DF_graphics_test.h"

typedef struct { void (*start)(void); void (*tick)(void); } source_egg_entry;
static const source_egg_entry source_eggs[32] = {
  [0xC0u-0xC0u] = {egg_C0_logo_start, egg_C0_logo_tick},
  [0xC1u-0xC0u] = {egg_C1_benchmark_start, egg_C1_benchmark_tick},
  [0xC2u-0xC0u] = {egg_C2_primitives_start, egg_C2_primitives_tick},
  [0xC3u-0xC0u] = {egg_C3_text_start, egg_C3_text_tick},
  [0xC4u-0xC0u] = {egg_C4_blit_start, egg_C4_blit_tick},
  [0xC5u-0xC0u] = {egg_C5_pattern_start, egg_C5_pattern_tick},
  [0xC6u-0xC0u] = {egg_C6_tilemap_start, egg_C6_tilemap_tick},
  [0xC7u-0xC0u] = {egg_C7_pong_basic_start, egg_C7_pong_basic_tick},
  [0xC8u-0xC0u] = {egg_C8_pong_wow_start, egg_C8_pong_wow_tick},
  [0xC9u-0xC0u] = {egg_C9_wriggler_start, egg_C9_wriggler_tick},
  [0xCAu-0xC0u] = {egg_CA_boulder_start, egg_CA_boulder_tick},
  [0xCBu-0xC0u] = {egg_CB_arcpie_benchmark_start, egg_CB_arcpie_benchmark_tick},
  [0xCCu-0xC0u] = {egg_CC_diag_start, egg_CC_diag_tick},
  [0xCDu-0xC0u] = {egg_CD_blit_full_start, egg_CD_blit_full_tick},
  [0xCEu-0xC0u] = {egg_CE_layer_start, egg_CE_layer_tick},
  [0xCFu-0xC0u] = {egg_CF_tvc_bitpattern_start, egg_CF_tvc_bitpattern_tick},
  [0xD0u-0xC0u] = {egg_D0_tvc_crisp_tearing_start, egg_D0_tvc_crisp_tearing_tick},
  [0xD1u-0xC0u] = {egg_D1_ruler_start, egg_D1_ruler_tick},
  [0xD2u-0xC0u] = {egg_D2_xy_offset_start, egg_D2_xy_offset_tick},
  [0xD3u-0xC0u] = {egg_D3_elite_start, egg_D3_elite_tick},
  [0xD4u-0xC0u] = {egg_D4_3d_scope_start, egg_D4_3d_scope_tick},
  [0xD5u-0xC0u] = {egg_D5_public_mesh_start, egg_D5_public_mesh_tick},
  [0xDFu-0xC0u] = {egg_DF_graphics_test_start, egg_DF_graphics_test_tick},
};
static void source_egg_work(uint8_t id) {
  unsigned steps;
  if(id==19u) {
    for(steps=0;steps<200000u&&!egg_D3_elite_ready();steps++)egg_D3_elite_step();
  } else if(id==20u) {
    for(steps=0;steps<200000u&&!egg_D4_3d_scope_ready();steps++)egg_D4_3d_scope_step();
  } else if(id==21u) {
    for(steps=0;steps<200000u&&!egg_D5_public_mesh_ready();steps++)egg_D5_public_mesh_step();
  }
}
static void source_egg_start(twodfx_emu *d,uint8_t id) {
  if(!d || id>=32u || !source_eggs[id].start)return;
  egg_api_bind(d);
  /* Same clean public defaults as the firmware egg registry. Retain RRAM. */
  d->engine=TWODFX_EMU_ENGINE_NORMAL;d->fps=50;d->transparent=0;
  d->logo_visible=d->overrun_visual=0;
  d->raster_pending=0;memset(d->raster_lines,0,sizeof d->raster_lines);
  d->collision_pending=d->collision_ready=d->collision_reported=0;
  memset(d->shadow_spt,0,sizeof d->shadow_spt);
  twodfx_reset_2dpt_all();twodfx_reset_resource_descriptors();
  d->resource_job=TWODFX_RESOURCE_JOB_NONE;
  d->egg_pending=0;
  source_eggs[id].start();
  source_egg_work(id);
  egg_api_bind(NULL);
}
static void source_egg_tick(twodfx_emu *d,uint8_t id) {
  if(!d || id>=32u)return;
  egg_api_bind(d);
  if(source_eggs[id].tick)source_eggs[id].tick();
  source_egg_work(id);
  egg_api_bind(NULL);
}
