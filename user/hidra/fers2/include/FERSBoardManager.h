#ifndef HIDRA_FERS2_BOARD_MANAGER_H
#define HIDRA_FERS2_BOARD_MANAGER_H

#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "FERSBoard.h"
#include "FERSConfiguration.h"
#include "FERSlib.h"
#include "FERSTypes.h"
#include "FersHandle.h"

namespace hidra {
namespace fers2 {

/**
 * @brief Manager for a collection of `FERSBoard` instances.
 *
 * Responsibilities:
 *  - Instantiate boards from a parsed `FERSConfiguration` (Open[n] entries).
 *  - Coordinate connect/configure/start/stop operations across all boards.
 *  - Provide a simple polling interface to collect events from every board.
 *
 * The class favors simplicity over concurrency; callers typically call
 * `ReadAvailableEvents()` from a single-threaded producer loop and then
 * perform any trigger-alignment at the producer level.
 */
 class FERSBoardManager {
 public:
  // Default constructor: empty manager. Use the constructor below or
  // `BuildBoardsFromConfiguration` to initialize.
  FERSBoardManager() = default;

  // Destructor performs best-effort cleanup of boards (disconnect) and
  // concentrator handles. Never throws.
  ~FERSBoardManager() noexcept;

  // Manager owns move-only board/handle state.
  FERSBoardManager(const FERSBoardManager&) = delete;
  FERSBoardManager& operator=(const FERSBoardManager&) = delete;
  FERSBoardManager(FERSBoardManager&&) noexcept = default;
  FERSBoardManager& operator=(FERSBoardManager&&) noexcept = default;

  // Construct and fully initialise the manager from `config`.
  // Throws `FersError` on failure.
  explicit FERSBoardManager(const FERSConfiguration& config,
                            int first_board_id = 0,
                            int readout_mode = 0,
                            int configure_mode = CFG_HARD);
   /**
    * Create board objects for each Open[n] entry in `config`.
    * @param first_board_id Logical starting id assigned to the first board.
    */
  // Build board objects from configuration. Throws `FersError` on fatal errors.
  // Build and initialise boards from configuration. This will open and
  // configure each board using provided `readout_mode` and `configure_mode`.
  // Throws `FersError` on fatal failures.
  void BuildBoardsFromConfiguration(const FERSConfiguration& config,
                                   int first_board_id = 0,
                                   int readout_mode = 0,
                                   int configure_mode = CFG_HARD);

   /**
    * Add a single board manually.
    * @param board_id Logical id for the board.
    * @param connection_path Open[n]-style connection string.
    * @param error Optional human-readable error output.
    */
   bool AddBoard(int board_id, const std::string& connection_path, std::string* error = nullptr);

   /**
    * Connect all configured boards (open devices and prepare readout).
    */
  // Connect all configured boards. Throws `FersError` on failure.
  void ConnectAll(int readout_mode = 0);

   /**
    * Configure all boards using `config`. When `load_config_file_first` is
    * true the manager will call `config.LoadIntoLibrary()` before applying
    * per-board parameter sets.
    */
   bool ConfigureAll(const FERSConfiguration& config,
                     int configure_mode,
                     bool load_config_file_first = true,
                     std::string* error = nullptr);

  bool SetHighVoltageAll(bool on, std::string* error = nullptr);
   bool StartAll(int start_mode, int run_number, std::string* error = nullptr);
   bool StopAll(int start_mode, int run_number, std::string* error = nullptr);
   bool DisconnectAll(std::string* error = nullptr);

   /**
    * Poll every board and return all events collected. The returned vector
    * aggregates events from all boards; events are not guaranteed to be
    * ordered by trigger id across boards — alignment should be performed
    * by the caller if required.
    */
  // Poll every board and return up to `max_total_events` aggregated from all
  // boards. A value of 0 means "no limit" (run forever until no data).
  std::vector<FERSEvent> ReadAvailableEvents(size_t max_total_events = 0, std::string* error = nullptr);

   /**
    * Poll slow-control monitor values from every connected board.
    */
   std::vector<BoardMonitorStatus> ReadMonitorStatuses(std::string* error = nullptr) const;

   FERSBoard* FindBoard(int board_id);
   const std::vector<FERSBoard>& boards() const { return m_boards; }

 private:
   struct TDLBoardRoute {
     bool is_tdl = false;
     std::string cnc_path;
     int chain = -1;
     int node = -1;
   };

   struct ConcentratorRecord {
     std::string cnc_path;
     FersHandle handle{};
     bool discovered = false;
     bool info_logged = false;
     FERS_CncInfo_t info{};
     std::vector<int> board_ids;
     std::set<std::pair<int, int>> occupied_nodes;
   };

  void RegisterBoardRoute(int board_id, const std::string& connection_path);
   ConcentratorRecord* GetOrCreateConcentrator(const std::string& cnc_path);
  void OpenAndLogConcentrator(ConcentratorRecord* concentrator);

   std::vector<FERSBoard> m_boards;
   std::vector<TDLBoardRoute> m_board_routes;
   std::map<std::string, ConcentratorRecord> m_concentrators;
 };

} // namespace fers2
} // namespace hidra

#endif // HIDRA_FERS2_BOARD_MANAGER_H
