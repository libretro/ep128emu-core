/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Károly Vaczkó
 */
#ifndef EP128EMU_TWODFX_ADAPTER_HPP
#define EP128EMU_TWODFX_ADAPTER_HPP
/*
 * Compileable, ep128emu-header-free example host connector.
 * This is the integration boundary, not a patch against a particular VM.
 * The maintainer attaches these methods at the VM's existing hooks.
 */
#include "twodfx_emu.h"
#include <stdint.h>
#include <stddef.h>

class Ep128emuTwoDfxAdapter {
public:
  explicit Ep128emuTwoDfxAdapter(twodfx_machine profile)
    : device_(twodfx_emu_create(profile)) {}
  ~Ep128emuTwoDfxAdapter() { twodfx_emu_destroy(device_); }
  bool valid() const { return device_ != 0; }
  void writePort(uint16_t z80Port, uint8_t data) {
    if ((z80Port & 255u) == 0xf8u) twodfx_emu_write_f8(device_, data);
    else if ((z80Port & 255u) == 0xf9u) twodfx_emu_write_f9(device_, data);
  }
  uint8_t readPort(uint16_t z80Port) const {
    return (z80Port & 255u) == 0xf8u ? twodfx_emu_read_f8(device_) : 0xff;
  }
  void videoVSync() { twodfx_emu_vsync(device_); }
  void videoHSync(unsigned absoluteLine) {
    twodfx_emu_hsync(device_, (uint16_t) absoluteLine);
  }
  bool wantsInterrupt() const { return twodfx_emu_irq_asserted(device_) != 0; }
  twodfx_engine_mode engineMode() const { return twodfx_emu_engine_mode(device_); }
  size_t readOverlayLine(unsigned storedX, unsigned storedY, uint8_t* indices,
                         uint8_t* coverage, size_t count) const {
    return twodfx_emu_overlay_line(device_, storedX, storedY, indices, coverage, count);
  }
  size_t readOverlayRawLine(unsigned rawX,unsigned rawY,uint8_t* indices,
                            uint8_t* coverage,size_t count) const {
    return twodfx_emu_overlay_raw_line(device_,rawX,rawY,indices,coverage,count);
  }
  bool installLogoArt(const void* image,size_t nbytes) {
    return twodfx_emu_set_logo_art(device_,image,nbytes)!=0;
  }
  void clearLogoArt() { twodfx_emu_clear_logo_art(device_); }
  twodfx_resource_job_status resourceJobStatus() const {
    return twodfx_emu_resource_job_status(device_);
  }
  uint32_t resourceJobOutputBytes() const {
    return twodfx_emu_resource_job_output_bytes(device_);
  }
  size_t snapshotSize() const { return twodfx_emu_snapshot_size(); }
  bool saveSnapshot(void* buffer, size_t capacity) const {
    return twodfx_emu_save_snapshot(device_, buffer, capacity) != 0;
  }
  bool loadSnapshot(const void* buffer, size_t length) {
    return twodfx_emu_load_snapshot(device_, buffer, length) != 0;
  }
  void physicalPowerCycle() { twodfx_emu_power_cycle(device_); }
private:
  Ep128emuTwoDfxAdapter(const Ep128emuTwoDfxAdapter&);
  Ep128emuTwoDfxAdapter& operator=(const Ep128emuTwoDfxAdapter&);
  twodfx_emu* device_;
};
#endif
