#include "eudaq/RunControl.hh"
#include "eudaq/TransportServer.hh"

using namespace eudaq;

class AhcalRunControl: public eudaq::RunControl {
   public:
      AhcalRunControl(const std::string & listenaddress);
      void Configure() override;
      void StartRun() override;
      void StopRun() override;
      void Exec() override;
      static const uint32_t m_id_factory = eudaq::cstr2hash("AhcalRunControl");

   private:
      void WaitForStates(const eudaq::Status::State state, const std::string message);
      void WaitForMessage(const std::string message);
      uint32_t m_stop_seconds; //maximum length of running in seconds
      uint32_t m_stop_filesize; //filesize in bytes. Run gets stopped after reaching this size
      uint32_t m_stop_events; // maximum number of events before the run gets stopped
      std::string m_next_conf_path; //filename of the next configuration file
      bool m_flag_running;
      std::chrono::steady_clock::time_point m_tp_start_run;
};

namespace {
   auto dummy0 = eudaq::Factory<eudaq::RunControl>::Register<AhcalRunControl, const std::string&>(AhcalRunControl::m_id_factory);
}

AhcalRunControl::AhcalRunControl(const std::string & listenaddress) :
      RunControl(listenaddress) {
   m_flag_running = false;
}

void AhcalRunControl::StartRun() {
   std::cout << "AHCAL runcontrol StartRun - beginning" << std::endl;
   RunControl::StartRun();
   m_tp_start_run = std::chrono::steady_clock::now();
   m_flag_running = true;
   std::cout << "AHCAL runcontrol StartRun - end" << std::endl;
}

void AhcalRunControl::StopRun() {
   std::cout << "AHCAL runcontrol StopRun - beginning" << std::endl;
   RunControl::StopRun();
   m_flag_running = false;
   std::cout << "AHCAL runcontrol StopRun - end" << std::endl;
}

void AhcalRunControl::Configure() {
   auto conf = GetConfiguration();
//   conf->SetSection("RunControl");
   m_stop_seconds = conf->Get("STOP_RUN_AFTER_N_SECONDS", 0);
   m_stop_filesize = conf->Get("STOP_RUN_AFTER_N_BYTES", 0);
   m_stop_events = conf->Get("STOP_RUN_AFTER_N_EVENTS", 0);
   m_next_conf_path = conf->Get("NEXT_RUN_CONF_FILE", "");
   if (conf->Get("RUN_NUMBER", 0)) {
      SetRunN(conf->Get("RUN_NUMBER", 0));
   }
   conf->Print();
   RunControl::Configure();
}

void AhcalRunControl::WaitForStates(const eudaq::Status::State state, const std::string message) {
   while (1) {
      bool waiting = false;
      auto map_conn_status = GetActiveConnectionStatusMap();
      for (auto &conn_status : map_conn_status) {
         auto state_conn = conn_status.second->GetState();
//         conn_status.second->GetMessage()
         if (state_conn != state) {
            waiting = true;
         }
      }
      if (!waiting) break;
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      EUDAQ_INFO(message); //"Waiting for end of stop run");
   }
}

void AhcalRunControl::Exec() {
   std::cout << "AHCAL runcontrol Exec - before StartRunControl" << std::endl;
   StartRunControl();
   std::cout << "AHCAL runcontrol Exec - after StartRunControl" << std::endl;
   while (IsActiveRunControl()) {
      //std::cout << "AHCAL runcontrol Exec - while loop" << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      bool restart_run = false; //whether to stop the run. There might be different reasons for it (timeout, number of evts, filesize...)
      if (m_flag_running) {
         //timeout condition
         if (m_stop_seconds) {
            auto tp_now = std::chrono::steady_clock::now();
            int duration_s = std::chrono::duration_cast<std::chrono::seconds>(tp_now - m_tp_start_run).count();
            std::cout << "Duration from start: " << duration_s;
            std::cout << ". Limit:" << m_stop_seconds << std::endl;
            if (duration_s > m_stop_seconds) {
               restart_run = true;
               EUDAQ_INFO_STREAMOUT("Timeout" + std::to_string(duration_s) + "reached.", std::cout, std::cerr);
            }
         }
         //number of event condition
         if (m_stop_events) {
            //std::vector<ConnectionSPC> conn_events;
            std::cout << "event sizes ";
            auto map_conn_status = GetActiveConnectionStatusMap();
            for (auto &conn_status : map_conn_status) {
               auto contype = conn_status.first->GetType();
               if (contype == "DataCollector") {
                  for (auto &elem : conn_status.second->GetTags()) {
                     if (elem.first == "EventN") {
                        std::cout << conn_status.first->GetName();
                        std::cout << "=" << stoi(elem.second) << " ";
                        if (stoi(elem.second) > m_stop_events) {
                           restart_run = true;
                           EUDAQ_INFO_STREAMOUT("Max event number" + elem.second + "reached.", std::cout, std::cerr);
                        }
                     }
                  }
               }
            }
            std::cout << "limit=" << m_stop_events << std::endl;
         }
         if (m_stop_filesize) {
            //TODO
         }
         //restart and reconfiguration handling
         if (restart_run) {
            std::cout << "AHCAL runcontrol Exec - restarting" << std::endl;
            EUDAQ_INFO("Stopping the run and restarting a new one.");
            StopRun(); // will stop and wait for all modules to stop daq
            //wait for everything to stop
            WaitForStates(eudaq::Status::STATE_CONF, "Waiting for end of stop run");

            if (m_next_conf_path.size()) {
               //TODO: check if file exists
               EUDAQ_INFO("Reading new config file: " + m_next_conf_path);
               ReadConfigureFile(m_next_conf_path);
               Configure();
               std::this_thread::sleep_for(std::chrono::seconds(2));
               //wait until everything is configured
               WaitForMessage("Configured");
//               WaitForStates(eudaq::Status::STATE_CONF, "Waiting for end of configure");
            }
            StartRun();
         }
      }
   }
}

inline void AhcalRunControl::WaitForMessage(const std::string message) {
   while (1) {
      bool waiting = false;
      auto map_conn_status = GetActiveConnectionStatusMap();
      for (auto &conn_status : map_conn_status) {
         auto conn_message = conn_status.second->GetMessage();
         if (conn_message.compare("Configured")) {
            //does not match
            std::cout << "DEBUG: waiting for " << conn_status.first->GetName() << ", which is " << conn_message << " instead of " << message << std::endl;
            waiting = true;
         }
      }
      if (!waiting) break;
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
   }
}
