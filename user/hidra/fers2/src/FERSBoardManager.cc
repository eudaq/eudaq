#include "FERSBoardManager.h"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "FERSlib.h"
#include "FersException.h"
#include "FERSPayloadSerialization.h"
#include "HidraUtils.hh"

namespace hidra {
namespace fers2 {

std::string BuildConcentratorSummary(const FERS_CncInfo_t& info) {
  std::ostringstream oss;
  oss << "PID=" << info.pid << " Model=" << info.ModelName << " Code=" << info.ModelCode
      << " FW=" << info.FPGA_FWrev << " SW=" << info.SW_rev << " Links=" << info.NumLink;
  return oss.str();
}

void FERSBoardManager::BuildBoardsFromConfiguration(const FERSConfiguration& config,
                                                   int first_board_id,
                                                   int readout_mode,
                                                   int configure_mode) {
  m_boards.clear();
  m_board_routes.clear();
  m_concentrators.clear();

  int board_id = first_board_id;
  for (const auto& path : config.open_paths()) {
    try {
      // Construct and fully initialise the board. Throws on failure.
      m_boards.emplace_back(board_id, path, &config, configure_mode, readout_mode);
    } catch (const FersError& e) {
      HIDRA_DEBUG("[WARNING][BRD {}] Ignoring board path {}: {}",
                 board_id,
                 path.c_str(),
                 e.what());
    }
    ++board_id;
  }

  if (m_boards.empty()) {
    throw FersError(std::string("No FERS boards configured from provided configuration"));
  }
}

FERSBoardManager::FERSBoardManager(const FERSConfiguration& config,
                                   int first_board_id,
                                   int readout_mode,
                                   int configure_mode) {
  BuildBoardsFromConfiguration(config, first_board_id, readout_mode, configure_mode);
}

FERSBoardManager::~FERSBoardManager() noexcept {
  std::string error;
  DisconnectAll(&error);
}

bool FERSBoardManager::AddBoard(int board_id, const std::string& connection_path, std::string* error) {
  for (const auto& board : m_boards) {
    if (board.board_id() == board_id) {
      if (error != nullptr) {
        *error = "Board id already present: " + std::to_string(board_id);
      }
      return false;
    }
  }

  try {
    RegisterBoardRoute(board_id, connection_path);
  } catch (const FersError& e) {
    if (error != nullptr) {
      *error = e.what();
    }
    return false;
  }

  m_boards.emplace_back(board_id, connection_path);
  return true;
}

void FERSBoardManager::ConnectAll(int readout_mode) {
  for (auto& entry : m_concentrators) {
    OpenAndLogConcentrator(&entry.second); // may throw
  }

  for (auto& board : m_boards) {
    if (!board.Connect(readout_mode)) {
      throw FersError("Connect failed for board id " + std::to_string(board.board_id()) + ": " + board.status().last_error);
    }
  }
}

void FERSBoardManager::RegisterBoardRoute(int board_id, const std::string& connection_path) {
  TDLBoardRoute route;
  const bool is_tdl = connection_path.find(":tdl:") != std::string::npos;

  if (is_tdl) {
    const std::string marker = ":tdl:";
    const size_t marker_pos = connection_path.find(marker);
    if (marker_pos == std::string::npos) {
      throw FersError("Path does not contain a TDL segment");
    }

    char cnc_buffer[512] = {0};
    if (FERS_Get_CncPath(const_cast<char*>(connection_path.c_str()), cnc_buffer) != 0 || cnc_buffer[0] == '\0') {
      throw FersError("Cannot infer concentrator path from '" + connection_path + "'");
    }

    const std::string remainder = connection_path.substr(marker_pos + marker.size());
    const size_t sep = remainder.find(':');
    if (sep == std::string::npos) {
      throw FersError("TDL path is missing chain/node indexes: " + connection_path);
    }

    const std::string chain_text = remainder.substr(0, sep);
    const std::string node_text = remainder.substr(sep + 1);
    try {
      size_t consumed = 0;
      route.chain = std::stoi(chain_text, &consumed, 0);
      if (consumed != chain_text.size()) {
        throw FersError("TDL path has invalid chain/node indexes: " + connection_path);
      }

      consumed = 0;
      route.node = std::stoi(node_text, &consumed, 0);
      if (consumed != node_text.size()) {
        throw FersError("TDL path has invalid chain/node indexes: " + connection_path);
      }
    } catch (const std::exception&) {
      throw FersError("TDL path has invalid chain/node indexes: " + connection_path);
    }

    if (route.chain < 0 || route.chain >= FERSLIB_MAX_NTDL || route.node < 0 || route.node >= FERSLIB_MAX_NNODES) {
      throw FersError("TDL chain/node out of range: " + connection_path);
    }

    route.cnc_path = cnc_buffer;

    ConcentratorRecord* concentrator = GetOrCreateConcentrator(route.cnc_path);
    if (concentrator == nullptr) {
      throw FersError("Failed to allocate concentrator record for " + route.cnc_path);
    }

    const auto node_key = std::make_pair(route.chain, route.node);
    if (!concentrator->occupied_nodes.insert(node_key).second) {
      throw FersError("Duplicate TDL chain/node for concentrator " + route.cnc_path + ": chain " +
                      std::to_string(route.chain) + " node " + std::to_string(route.node));
    }

    concentrator->board_ids.push_back(board_id);
  }

  route.is_tdl = is_tdl;
  m_board_routes.push_back(route);
}

FERSBoardManager::ConcentratorRecord* FERSBoardManager::GetOrCreateConcentrator(const std::string& cnc_path) {
  auto it = m_concentrators.find(cnc_path);
  if (it != m_concentrators.end()) {
    return &it->second;
  }

  ConcentratorRecord record;
  record.cnc_path = cnc_path;
  auto inserted = m_concentrators.emplace(cnc_path, std::move(record));
  return &inserted.first->second;
}

void FERSBoardManager::OpenAndLogConcentrator(ConcentratorRecord* concentrator) {
  if (concentrator == nullptr) {
    return;
  }

  if (concentrator->info_logged) {
    return;
  }

  if (!concentrator->discovered) {
    int handle = -1;
    int ret = FERS_OpenDevice(const_cast<char*>(concentrator->cnc_path.c_str()), &handle);
    if (ret != 0) {
      throw FersError("Failed to open concentrator '" + concentrator->cnc_path + "'", ret);
    }

    concentrator->handle = FersHandle(handle);

    FERS_CncInfo_t info{};
    ret = FERS_ReadConcentratorInfo(handle, &info);
    if (ret != 0) {
      throw FersError("Failed to read concentrator info for '" + concentrator->cnc_path + "'", ret);
    }

    concentrator->info = info;
    concentrator->discovered = true;
  }

  int total_boards = 0;
  for (const auto& chain : concentrator->info.ChainInfo) {
    total_boards += chain.BoardCount;
  }

  if (total_boards <= 0) {
    throw FersError("Concentrator '" + concentrator->cnc_path + "' reported no connected boards");
  }

  const int handle = concentrator->handle.get();
  HIDRA_DEBUG("[INFO][CNC {}] Connected concentrator {} ({}), board_count={}",
              FERS_INDEX(handle),
              concentrator->cnc_path.c_str(),
              BuildConcentratorSummary(concentrator->info).c_str(),
              total_boards);
  for (uint16_t chain = 0; chain < concentrator->info.NumLink && chain < 8; ++chain) {
    const auto& chain_info = concentrator->info.ChainInfo[chain];
    if (chain_info.BoardCount > 0) {
      HIDRA_DEBUG(const_cast<char*>("[INFO][CNC {}] Chain {}: board_count={} rate={} cps throughput={} Mbps"),
          FERS_INDEX(handle),
          chain,
          chain_info.BoardCount,
          chain_info.EventRate,
          chain_info.Mbps);
    }
  }

  concentrator->info_logged = true;
}

bool FERSBoardManager::ConfigureAll(const FERSConfiguration& config,
                                    int configure_mode,
                                    bool load_config_file_first,
                                    std::string* error) {
  /*
  if (load_config_file_first && !config.LoadIntoLibrary(error)) {
    return false;
  }
  */

  for (auto& board : m_boards) {
    if (!board.Configure(config, configure_mode)) {
      if (error != nullptr) {
        *error = "Configure failed for board id " + std::to_string(board.board_id()) + ": " + board.status().last_error;
      }
      return false;
    }
  }

  return true;
}

bool FERSBoardManager::SetHighVoltageAll(bool on, std::string* error) {
  for (auto& board : m_boards) {
    if (!board.SetHighVoltage(on)) {
      if (error != nullptr) {
        *error = "High voltage toggle failed for board id " + std::to_string(board.board_id()) + ": " +
                 board.status().last_error;
      }
      return false;
    }
  }

  return true;
}

bool FERSBoardManager::StartAll(int start_mode, int run_number, std::string* error) {
  if (m_boards.empty()) {
    if (error != nullptr) {
      *error = "No FERS boards configured.";
    }
    return false;
  }

  std::vector<int> handles;
  handles.reserve(m_boards.size());
  for (const auto& board : m_boards) {
    handles.push_back(board.handle());
    //FERS_FlushData(board.handle()); //TODO Test this, disabled for now
  }
  handles.push_back(-1); //rscaglio fix from gemini


  int ret = FERS_StartAcquisition(handles.data(), static_cast<int>(handles.size()-1), start_mode, run_number);
  if (ret != 0) {
    if (error != nullptr) {
      *error = "FERS_StartAcquisition failed with code " + std::to_string(ret);
    }
    return false;
  }
  return true;
}

bool FERSBoardManager::StopAll(int start_mode, int run_number, std::string* error) {
  if (m_boards.empty()) {
    return true;
  }

  std::vector<int> handles;
  handles.reserve(m_boards.size());
  for (const auto& board : m_boards) {
    handles.push_back(board.handle());
  }

  int ret = FERS_StopAcquisition(handles.data(), static_cast<int>(handles.size()-1), start_mode, run_number);
  if (ret != 0) {
    if (error != nullptr) {
      *error = "FERS_StopAcquisition failed with code " + std::to_string(ret);
    }
    return false;
  }
  return true;
}

bool FERSBoardManager::DisconnectAll(std::string* error) {
  bool ok = true;
  std::string combined_error;
  for (auto& board : m_boards) {
    if (!board.Disconnect()) {
      ok = false;
      if (!combined_error.empty()) {
        combined_error += "; ";
      }
      combined_error += "Disconnect failed for board id " + std::to_string(board.board_id()) + ": " +
                        board.status().last_error;
    }
  }
  if (!ok && error != nullptr) {
    *error = combined_error;
  }
  return ok;
}

std::vector<FERSEvent> FERSBoardManager::ReadAvailableEvents(size_t max_total_events, std::string* error) {
  std::vector<FERSEvent> out;
  if (m_boards.empty()) {
    if (error != nullptr) {
      *error = "No FERS boards configured.";
    }
    return {};
  }

  std::vector<int> handles;
  handles.reserve(m_boards.size());
  for (const auto& board : m_boards) {
    const int handle = board.handle();
    if (handle < 0) {
      if (error != nullptr) {
        *error = "Cannot read events: board id " + std::to_string(board.board_id()) + " is not connected.";
      }
      return {};
    }
    handles.push_back(handle);
  }

  size_t read_count = 0;
  while (max_total_events == 0 || read_count < max_total_events) {
    int board_index = -1;
    int data_qualifier = 0;
    double timestamp_us = 0.0;
    void* event_ptr = nullptr;
    int nb = 0;

    const int ret = FERS_GetEvent(handles.data(), &board_index, &data_qualifier, &timestamp_us, &event_ptr, &nb);
    if (ret == 0 || ret == 2) {
      break;
    }
    if (ret == FERSLIB_ERR_READOUT_ERROR && nb == 0) {
      HIDRA_DEBUG("FERS_GetEvent failed with code {}, data red {}. Continuing data acquisition...\n", ret, nb);
      return {}; // DO NOT SET ERROR HERE AS WE SHOULD BE ABLE TO KEEP ON READING
    } else if (ret < 0) {
      if (error != nullptr) {
        *error = "FERS_GetEvent failed with code " + std::to_string(ret) + " with data red " + std::to_string(nb);
      }
      return {};
    }

    if (board_index < 0 || static_cast<size_t>(board_index) >= m_boards.size()) {
      if (error != nullptr) {
        *error = "FERS_GetEvent returned out-of-range board index " + std::to_string(board_index);
      }
      return {};
    }

    if (nb <= 0 || event_ptr == nullptr) {
      break;
    }

    // Filtering service events before reaching the queue
    if (data_qualifier == DTQ_SERVICE) {
      HIDRA_DEBUG("From board {}: received service event with data qualifier {}. Skipping", board_index, data_qualifier);
      continue;
    }


    FERSEvent event;
    event.timestamp_us = timestamp_us;
    event.acq_time = std::chrono::steady_clock::now();
    // `board_index` is the vendor-provided index returned by FERS_GetEvent.
    const int vendor_index = board_index;
    const int logical_board_id = m_boards[board_index].board_id();
    if (!SerializeFersEventPayload(event_ptr, data_qualifier, vendor_index, logical_board_id, &event, error)) {
      return {};
    }

    out.push_back(std::move(event));
    ++read_count;
  }

  return out;
}

std::vector<BoardMonitorStatus> FERSBoardManager::ReadMonitorStatuses(std::string* error) const {
  std::vector<BoardMonitorStatus> out;
  out.reserve(m_boards.size());

  for (const auto& board : m_boards) {
    BoardMonitorStatus status;
    if (!board.ReadMonitorStatus(&status)) {
      if (error != nullptr) {
        *error = "Monitor status read failed for board id " + std::to_string(board.board_id()) + ": " +
                 status.last_error;
      }
      return {};
    }

    out.push_back(std::move(status));
  }

  return out;
}

FERSBoard* FERSBoardManager::FindBoard(int board_id) {
  for (auto& board : m_boards) {
    if (board.board_id() == board_id) {
      return &board;
    }
  }
  return nullptr;
}

} // namespace fers2
} // namespace hidra
