// SPDX-FileCopyrightText: (c) HiDRa developers
#ifndef HIDRA_FERS2_TYPES_H
#define HIDRA_FERS2_TYPES_H

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace hidra {
namespace fers2 {

/**
 * @file FERSTypes.h
 * @brief Lightweight type definitions used across the `fers2` module.
 *
 * This header centralizes small POD types used by the board wrapper and the
 * producer. Types are intentionally minimal to keep dependencies low and to
 * make serialization/inspection straightforward.
 */

/// Simple state machine for a single FERS board.
enum class BoardState { kDisconnected, kConnected, kConfigured, kRunning };

/// Runtime status information for a single board instance.
///
/// - `handle` is the integer handle returned by the underlying FERS API
///   (or -1 if not opened).
/// - `last_return_code` stores the most recent integer return code from
///   a FERS library call for diagnostic purposes.
struct BoardStatus {
  BoardState state = BoardState::kDisconnected;
  int handle = -1;
  int last_return_code = 0;
  std::string last_error;
  int allocated_readout_bytes = 0;
};

/**
 * @brief Latest slow-control monitor values read directly from one board.
 *
 * Each `*_valid` flag is independent because some FERS models or firmware
 * versions do not expose every monitor value.
 */
struct BoardMonitorStatus {
  int board_id = -1;
  uint64_t read_time_ns = 0;
  int last_return_code = 0;
  std::string last_error;

  bool fpga_temp_valid = false;
  bool board_temp_valid = false;
  bool tdc0_temp_valid = false;
  bool tdc1_temp_valid = false;
  bool hv_temp_valid = false;
  bool detector_temp_valid = false;
  bool hv_vmon_valid = false;
  bool hv_imon_valid = false;
  bool hv_status_valid = false;

  float temp_fpga = 0.0f;
  float temp_board = 0.0f;
  float temp_tdc0 = 0.0f;
  float temp_tdc1 = 0.0f;
  float temp_hv = 0.0f;
  float temp_detector = 0.0f;
  float hv_vmon = 0.0f;
  float hv_imon = 0.0f;

  int hv_on = 0;
  int hv_ramping = 0;
  int hv_over_current = 0;
  int hv_over_voltage = 0;
};

/**
 * @brief In-memory representation of a single event read from a FERS board.
 *
 * The structure is deliberately generic: the `payload` contains the raw
 * bytes of the event as emitted by the FERS library. Higher layers may
 * interpret or deserialize these bytes as needed for EuDAQ event creation.
 *
 * Fields:
 *  - `board_id`: logical identifier assigned by the manager (Open[n] index)
 *  - `board_index`: optional index within a physical crate (if applicable)
 *  - `data_qualifier`: the FERS data qualifier value (event format/type)
 *  - `timestamp_us`: best-effort event timestamp in microseconds
 *  - `trigger_id`: numeric trigger counter used for multi-board alignment
 *  - `payload`: raw event bytes produced by the FERS runtime
 */
struct FERSEvent {
  int board_id = -1;
  int board_index = -1;
  int data_qualifier = -1;
  double timestamp_us = 0.0;
  uint64_t trigger_id = 0;
  std::vector<uint8_t> payload;
  std::chrono::steady_clock::time_point acq_time;
};

} // namespace fers2
} // namespace hidra

#endif // HIDRA_FERS2_TYPES_H
