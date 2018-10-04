#include "eudaq/DataCollector.hh"
#include "eudaq/Logger.hh"
#include <mutex>
#include <deque>
#include <map>
#include <set>

class CaliceAhcalBifBxidDataCollector: public eudaq::DataCollector {
   public:
      CaliceAhcalBifBxidDataCollector(const std::string &name, const std::string &runcontrol);
      void DoStartRun() override;
      void DoConfigure() override;
      void DoConnect(eudaq::ConnectionSPC id) override;
      void DoDisconnect(eudaq::ConnectionSPC id) override;
      void DoReceive(eudaq::ConnectionSPC id, eudaq::EventSP ev) override;

      static const uint32_t m_id_factory = eudaq::cstr2hash("CaliceAhcalBifBxidDataCollector");

   private:
      void AddEvent_TimeStamp(uint32_t id, eudaq::EventSPC ev);
      void BuildEvent_bxid();

      std::mutex m_mutex;
      //to be edited according to the name of the processes. TODO implement as parameter to the configuration
      std::string mc_name_ahcal = "AHCAL1";
      std::string mc_name_bif = "BIF1";
      std::string mc_name_hodoscope1 = "Hodoscope1";
      std::string mc_name_hodoscope2 = "Hodoscope2";
      std::string mc_name_desytable = "desytable1";

      //correction factor to be added to the ROC tag. This can eliminate differences between starting ROC numbers in different producer - some can start from 1, some from 0
      int m_roc_offset_ahcal = 0;
      int m_roc_offset_bif = 0;
      int m_roc_offset_hodoscope1 = 0;
      int m_roc_offset_hodoscope2 = 0;

      static const int mc_roc_invalid = INT32_MAX;
      static const int mc_bxid_invalid = INT32_MAX;

      //ts
      std::deque<eudaq::EventSPC> m_que_ahcal;
      std::deque<eudaq::EventSPC> m_que_bif;
      std::deque<eudaq::EventSPC> m_que_hodoscope1;
      std::deque<eudaq::EventSPC> m_que_hodoscope2;
      std::deque<eudaq::EventSPC> m_que_desytable;
      bool m_active_ahcal = false;
      bool m_active_bif = false;
      bool m_active_hodoscope1 = false;
      bool m_active_hodoscope2 = false;
      bool m_active_desytable = false;

      //final test whether the event will be thrown away if a mandatory subevents are not present
      bool m_evt_mandatory_ahcal = true;
      bool m_evt_mandatory_bif = true;
      bool m_evt_mandatory_hodoscope1 = true;
      bool m_evt_mandatory_hodoscope2 = true;
      bool m_evt_mandatory_desytable = false;

      int m_thrown_incomplete = 0; //how many event were thrown away because there were incomplete

      std::chrono::time_point<std::chrono::system_clock> lastprinttime;
      eudaq::EventSPC m_ev_last_cal;
      eudaq::EventSPC m_ev_last_bif;
      uint64_t m_ts_end_last_cal;
      uint64_t m_ts_end_last_bif;
      bool m_offset_ahcal2bif_done;
      int64_t m_ts_offset_ahcal2bif;
      uint32_t m_ev_n;
};

namespace {
   auto dummy0 = eudaq::Factory<eudaq::DataCollector>::
         Register<CaliceAhcalBifBxidDataCollector, const std::string&, const std::string&>
         (CaliceAhcalBifBxidDataCollector::m_id_factory);
}

CaliceAhcalBifBxidDataCollector::CaliceAhcalBifBxidDataCollector(const std::string &name, const std::string &runcontrol) :
      DataCollector(name, runcontrol) {
}

void CaliceAhcalBifBxidDataCollector::DoStartRun() {
   m_que_ahcal.clear();
   m_que_bif.clear();
   m_que_hodoscope1.clear();
   m_que_hodoscope2.clear();
   m_que_desytable.clear();

   m_ts_end_last_cal = 0;
   m_ts_end_last_bif = 0;
   m_offset_ahcal2bif_done = false;
   m_ts_offset_ahcal2bif = 0;
   m_ev_n = 0;
   m_thrown_incomplete = 0;
}

void CaliceAhcalBifBxidDataCollector::DoConfigure() {
   auto conf = GetConfiguration();
   if (conf) {
      conf->Print();
      // m_pri_ts = conf->Get("PRIOR_TIMESTAMP", m_pri_ts?1:0);
   }
   int MandatoryBif = conf->Get("MandatoryBif", 1);
   m_evt_mandatory_bif = (MandatoryBif == 1) ? true : false;
   std::cout << "#MandatoryBif=" << MandatoryBif << std::endl;
   lastprinttime = std::chrono::system_clock::now();
}

void CaliceAhcalBifBxidDataCollector::DoConnect(eudaq::ConnectionSPC idx) {
   bool recognizedProducer = false;
   std::cout << "connecting " << idx << std::endl;
   if (idx->GetName() == mc_name_ahcal) {
      m_active_ahcal = true;
      recognizedProducer = true;
   }
   if (idx->GetName() == mc_name_bif) {
      m_active_bif = true;
      recognizedProducer = true;
   }
   if (idx->GetName() == mc_name_hodoscope1) {
      m_active_hodoscope1 = true;
      recognizedProducer = true;
   }
   if (idx->GetName() == mc_name_hodoscope2) {
      m_active_hodoscope2 = true;
      recognizedProducer = true;
   }
   if (idx->GetName() == mc_name_desytable) {
      m_active_desytable = true;
      recognizedProducer = true;
   }
   if (!recognizedProducer) {
      std::cout << "Producer " << idx->GetName() << " is not recognized by this datacollector" << std::endl;
      EUDAQ_ERROR_STREAMOUT("Producer " + idx->GetName() + " is not recognized by this datacollector", std::cout, std::cerr);
   }
}

void CaliceAhcalBifBxidDataCollector::DoDisconnect(eudaq::ConnectionSPC idx) {
   std::cout << "disconnecting " << idx << std::endl;
   if (idx->GetName() == mc_name_ahcal) m_active_ahcal = false;
   if (idx->GetName() == mc_name_bif) m_active_bif = false;
   if (idx->GetName() == mc_name_hodoscope1) m_active_hodoscope1 = false;
   if (idx->GetName() == mc_name_hodoscope2) m_active_hodoscope2 = false;
   if (idx->GetName() == mc_name_desytable) m_active_desytable = false;
   std::cout << m_thrown_incomplete << " incomplete events thrown away" << std::endl;
   if (!(m_active_ahcal && m_active_bif && m_active_desytable && m_active_hodoscope1 && m_active_hodoscope2)) {
//      BuildEvent_bxid();
      EUDAQ_INFO_STREAMOUT("Saved events: " + std::to_string(m_ev_n), std::cout, std::cerr);
   }
}

void CaliceAhcalBifBxidDataCollector::DoReceive(eudaq::ConnectionSPC idx, eudaq::EventSP ev) {
   if (ev->IsFlagFake()) {
      EUDAQ_WARN("Receive event fake");
      return;
   }
//   if (ev->GetTag("BXID", mc_bxid_invalid) == mc_bxid_invalid) {
//      EUDAQ_ERROR_STREAMOUT("Received event without BXID", std::cout, std::cerr);
//      return;
//   }

   std::string con_name = idx->GetName();
   if (con_name == mc_name_ahcal)
      m_que_ahcal.push_back(std::move(ev));
   else if (con_name == mc_name_bif)
      m_que_bif.push_back(std::move(ev));
   else if (con_name == mc_name_hodoscope1)
      m_que_hodoscope1.push_back(std::move(ev));
   else if (con_name == mc_name_hodoscope2)
      m_que_hodoscope2.push_back(std::move(ev));
   else if (con_name == mc_name_desytable)
      m_que_desytable.push_back(std::move(ev));
   else {
      EUDAQ_WARN("Receive event from unkonwn Producer");
      return;
   }

   //quit if not enough events to merge
   if (m_que_ahcal.empty() && m_active_ahcal) return;
   if (m_que_bif.empty() && m_active_bif) return;
   if (m_que_hodoscope1.empty() && m_active_hodoscope1) return;
   if (m_que_hodoscope2.empty() && m_active_hodoscope2) return;
// std::cout<<"p 1 \n";
   BuildEvent_bxid();
}

inline void CaliceAhcalBifBxidDataCollector::BuildEvent_bxid() {
   //calculate the ahcal2bif offset if needed
//   if (!m_offset_ahcal2bif_done) {
//      if ((m_active_ahcal == false) || (m_active_bif == false)) {
//         //we don't calculate offset if there are not 2 devices
//         m_ts_offset_ahcal2bif = 0LL;
//         m_offset_ahcal2bif_done = true;
//         return;
//      } else {
//         //at this point all queues are not empty and not everybody is active
//         if (!m_que_ahcal.front()->IsBORE() || !m_que_ahcal.front()->IsBORE()) {
//            std::cout << "ERROR: events in ahcal and bif queues do not have BOER status" << std::endl;
//            EUDAQ_THROW("the first event is not bore");
//         }
//         uint64_t start_ts_cal = m_que_ahcal.front()->GetTag("FirstROCStartTS", uint64_t(0));
//         uint64_t start_ts_bif = m_que_bif.front()->GetTag("FirstROCStartTS", uint64_t(0));
//         if ((start_ts_bif == 0LL) || (start_ts_cal == 0LL)) {
//            std::cout << "ERROR: FirstROCStartTS tags are not defined for both ahcal and bif events" << std::endl;
//         }
//         m_ts_offset_ahcal2bif = (start_ts_cal << 5) - start_ts_bif;
//         m_offset_ahcal2bif_done = true;
//         // std::cout<<"start_ts_bif "<< start_ts_bif <<std::endl;
//         // std::cout<<"start_ts_cal "<< start_ts_cal <<std::endl;
//         // std::cout<<"offset "<< m_ts_offset_cal2bif <<std::endl;
//      }
//   }
   std::lock_guard<std::mutex> lock(m_mutex); //only 1 process should be allowed to make events
   while (true) {
      SetStatusTag("Queue", std::string("(") + std::to_string(m_que_ahcal.size())
            + "," + std::to_string(m_que_bif.size())
            + "," + std::to_string(m_que_hodoscope1.size())
            + "," + std::to_string(m_que_hodoscope2.size())
            + "," + std::to_string(m_que_desytable.size()) + ")");
      if (std::chrono::system_clock::now() - lastprinttime > std::chrono::milliseconds(5000)) {
         lastprinttime = std::chrono::system_clock::now();
         std::cout << "Queue sizes: AHCAL: " << m_que_ahcal.size();
         std::cout << " BIF: " << m_que_bif.size();
         std::cout << " HODO1: " << m_que_hodoscope1.size();
         std::cout << " HODO2: " << m_que_hodoscope2.size();
         std::cout << " table:." << m_que_desytable.size() << std::endl;
      }

      int bxid_ahcal = mc_bxid_invalid;
      int bxid_bif = mc_bxid_invalid;
      int bxid_hodoscope1 = mc_bxid_invalid;
      int bxid_hodoscope2 = mc_bxid_invalid;

      int roc_ahcal = mc_roc_invalid;
      int roc_bif = mc_roc_invalid;
      int roc_hodoscope1 = mc_roc_invalid;
      int roc_hodoscope2 = mc_roc_invalid;
//      const eudaq::EventSP ev_front_cal;
//      const eudaq::EventSP ev_front_bif;

//      uint64_t ts_beg_bif = UINT64_MAX; //timestamps in 0.78125 ns steps
//      uint64_t ts_end_bif = UINT64_MAX; //timestamps in 0.78125 ns steps
//      uint64_t ts_beg_cal = UINT64_MAX; //timestamps in 0.78125 ns steps
//      uint64_t ts_end_cal = UINT64_MAX; //timestamps in 0.78125 ns steps
//      uint64_t ts_beg = UINT64_C(0); //timestamps in 0.78125 ns steps
//      uint64_t ts_end = UINT64_C(0); //timestamps in 0.78125 ns steps

      if (m_que_ahcal.empty()) {
         if (m_active_ahcal) return; //more will come
      } else {
         roc_ahcal = m_que_ahcal.front()->GetTag("ROC", mc_roc_invalid);
         bxid_ahcal = m_que_ahcal.front()->GetTag("BXID", mc_bxid_invalid);
         if ((roc_ahcal == mc_roc_invalid) || (bxid_ahcal == mc_bxid_invalid)) {
            EUDAQ_WARN_STREAMOUT(
                  "event " + std::to_string(m_que_ahcal.front()->GetEventN()) + " without ROC(" + std::to_string(roc_ahcal) + ") or BXID("
                        + std::to_string(bxid_ahcal) + ") in run " + std::to_string(m_que_ahcal.front()->GetRunNumber()), std::cout, std::cerr);
            m_que_ahcal.pop_front();
            continue;
         }
         roc_ahcal += m_roc_offset_ahcal;
      }

      if (m_que_bif.empty()) {
         if (m_active_bif) return; //more will come
      } else {
         roc_bif = m_que_bif.front()->GetTag("ROC", mc_roc_invalid);
         bxid_bif = m_que_bif.front()->GetTag("BXID", mc_bxid_invalid);
         if ((roc_bif == mc_roc_invalid) || (bxid_bif == mc_bxid_invalid)) {
            EUDAQ_WARN_STREAMOUT(
                  "event " + std::to_string(m_que_bif.front()->GetEventN()) + " without ROC(" + std::to_string(roc_bif) + ") or BXID("
                        + std::to_string(bxid_bif) + ") in run " + std::to_string(m_que_bif.front()->GetRunNumber()), std::cout, std::cerr);
            m_que_bif.pop_front();
            continue;
         }
         roc_bif += m_roc_offset_bif;
         if (bxid_bif < -100) { //cca 400 us, which can be spend on the power pulsing
            std::cout << "DEBUG: event " + std::to_string(m_que_bif.front()->GetEventN()) + " has too low bxid ("
                  + std::to_string(bxid_bif) + ") in run " + std::to_string(m_que_bif.front()->GetRunNumber()) << std::endl;
            m_que_bif.pop_front();
            continue;
         }
      }

      if (m_que_hodoscope1.empty()) {
         if (m_active_hodoscope1) return;
      } else {
         roc_hodoscope1 = m_que_hodoscope1.front()->GetTag("ROC", mc_roc_invalid);
         bxid_hodoscope1 = m_que_hodoscope1.front()->GetTag("BXID", mc_bxid_invalid);
         if ((roc_hodoscope1 == mc_roc_invalid) || (bxid_hodoscope1 == mc_bxid_invalid)) {
            EUDAQ_WARN_STREAMOUT("event " + std::to_string(m_que_hodoscope1.front()->GetEventN()) +
                  " without ROC(" + std::to_string(roc_hodoscope1) +
                  ") or BXID(" + std::to_string(bxid_hodoscope1) +
                  ") in run " + std::to_string(m_que_hodoscope1.front()->GetRunNumber()), std::cout, std::cerr);
            m_que_hodoscope1.pop_front();
            continue;
         }
         roc_hodoscope1 += m_roc_offset_hodoscope1;
      }

      if (m_que_hodoscope2.empty()) {
         if (m_active_hodoscope2) return;
      } else {
         roc_hodoscope2 = m_que_hodoscope2.front()->GetTag("ROC", mc_roc_invalid);
         bxid_hodoscope2 = m_que_hodoscope2.front()->GetTag("BXID", mc_bxid_invalid);
         if ((roc_hodoscope2 == mc_roc_invalid) || (bxid_hodoscope2 == mc_bxid_invalid)) {
            EUDAQ_WARN_STREAMOUT("event " + std::to_string(m_que_hodoscope2.front()->GetEventN()) +
                  " without ROC(" + std::to_string(roc_hodoscope2) +
                  ") or BXID(" + std::to_string(bxid_hodoscope2) +
                  ") in run " + std::to_string(m_que_hodoscope2.front()->GetRunNumber()), std::cout, std::cerr);
            m_que_hodoscope2.pop_front();
            continue;
         }
         roc_hodoscope2 += m_roc_offset_hodoscope2;
      }

      // at this point all producers have something in the buffer (if possible)
      bool present_ahcal = false, present_bif = false, present_hodoscope1 = false, present_hodoscope2 = false, present_desytable = false; //whether event is present in the merged event

      int processedRoc = mc_roc_invalid;
      if (roc_ahcal < processedRoc) processedRoc = roc_ahcal;
      if (roc_bif < processedRoc) processedRoc = roc_bif;
      if (roc_hodoscope1 < processedRoc) processedRoc = roc_hodoscope1;
      if (roc_hodoscope2 < processedRoc) processedRoc = roc_hodoscope2;

      int processedBxid = mc_bxid_invalid;
      if ((roc_ahcal == processedRoc) && (bxid_ahcal < processedBxid)) processedBxid = bxid_ahcal;
      if ((roc_bif == processedRoc) && (bxid_bif < processedBxid)) processedBxid = bxid_bif;
      if ((roc_hodoscope1 == processedRoc) && (bxid_hodoscope1 < processedBxid)) processedBxid = bxid_hodoscope1;
      if ((roc_hodoscope2 == processedRoc) && (bxid_hodoscope2 < processedBxid)) processedBxid = bxid_hodoscope2;

      // std::cout << "Trying to put together event with ROC=" << processedRoc << " BXID=" << processedBxid;
      // std::cout << "\tAHCALROC=" << roc_ahcal << ",AHCALBXID=" << bxid_ahcal << "\tBIFROC=" << roc_bif << ",BIFBXID=" << bxid_bif;
      // std::cout << "\tH1ROC=" << roc_hodoscope1 << ",H1BXID=" << bxid_hodoscope1;
      // std::cout << "\tH2ROC=" << roc_hodoscope2 << ",H2BXID=" << bxid_hodoscope2 << std::endl;

      auto ev_sync = eudaq::Event::MakeUnique("CaliceBxid");
      ev_sync->SetFlagPacket();
      if ((roc_ahcal == processedRoc) && (bxid_ahcal == processedBxid)) {
         if (!m_que_ahcal.empty()) {
            ev_sync->AddSubEvent(std::move(m_que_ahcal.front()));
            m_que_ahcal.pop_front();
            present_ahcal = true;
         }
      }

      if ((roc_bif == processedRoc) && (bxid_bif == processedBxid)) {
         if (!m_que_bif.empty()) {
            ev_sync->AddSubEvent(std::move(m_que_bif.front()));
            m_que_bif.pop_front();
            present_bif = true;
         }
      }

      if ((roc_hodoscope1 == processedRoc) && (bxid_hodoscope1 == processedBxid)) {
         if (!m_que_hodoscope1.empty()) {
            ev_sync->AddSubEvent(std::move(m_que_hodoscope1.front()));
            m_que_hodoscope1.pop_front();
            present_hodoscope1 = true;
         }
      }

      if ((roc_hodoscope2 == processedRoc) && (bxid_hodoscope2 == processedBxid)) {
         if (!m_que_hodoscope2.empty()) {
            ev_sync->AddSubEvent(std::move(m_que_hodoscope2.front()));
            m_que_hodoscope2.pop_front();
            present_hodoscope2 = true;
         }
      }

      if (!m_que_desytable.empty()) {
         ev_sync->AddSubEvent(std::move(m_que_desytable.front()));
         m_que_desytable.pop_front();
         present_desytable = true;
      }
      m_thrown_incomplete += 1; //increase in case the loop is exit in following lines
      if (m_evt_mandatory_ahcal && (!present_ahcal) && (m_active_ahcal)) continue; //throw awway incomplete event
      if (m_evt_mandatory_bif && (!present_bif) && (m_active_bif)) continue; //throw awway incomplete event
      if (m_evt_mandatory_hodoscope1 && (!present_hodoscope1) && (m_active_hodoscope1)) continue; //throw awway incomplete event
      if (m_evt_mandatory_hodoscope2 && (!present_hodoscope2) && (m_active_hodoscope2)) continue; //throw awway incomplete event
      m_thrown_incomplete -= 1; //and decrease back.
      ev_sync->SetEventN(m_ev_n++);
//      ev_sync->Print(std::cout);
      WriteEvent(std::move(ev_sync));
   }
}
