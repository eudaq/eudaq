#include "eudaq/DataCollector.hh"
#include <mutex>
#include <deque>
#include <map>
#include <set>

class AhcalHodoscopeDataCollector: public eudaq::DataCollector {
   public:
      AhcalHodoscopeDataCollector(const std::string &name,
            const std::string &runcontrol);
      void DoConnect(eudaq::ConnectionSPC id) override;
      void DoDisconnect(eudaq::ConnectionSPC id) override;
      void DoReceive(eudaq::ConnectionSPC id, eudaq::EventSP ev) override;
      void printStats();
      static const uint32_t m_id_factory = eudaq::cstr2hash("AhcalHodoscopeDataCollector");
      private:
      void BuildEvent();

      std::mutex m_mtx_map;
      std::map<eudaq::ConnectionSPC, std::deque<eudaq::EventSPC>> m_ahcal_conn_evts;
      std::map<eudaq::ConnectionSPC, std::deque<eudaq::EventSPC>> m_hodoscope_conn_evts;
      std::set<eudaq::ConnectionSPC> m_conn_disconnected;
      std::map<int, int> tdc_hist_ahcal, tdc_hist_hodoscope, tdc_hist_difference;
      int debug_unmatched;
//      std::set<eudaq::ConnectionSPC> m_event_ready_ts;
//      uint64_t m_ts_last_end;
//      uint64_t m_ts_curr_beg;
//      uint64_t m_ts_curr_end;
};

namespace {
auto dummy0 = eudaq::Factory<eudaq::DataCollector>::
      Register<AhcalHodoscopeDataCollector, const std::string&, const std::string&>
      (AhcalHodoscopeDataCollector::m_id_factory);
}

AhcalHodoscopeDataCollector::AhcalHodoscopeDataCollector(const std::string &name, const std::string &runcontrol)
      : DataCollector(name, runcontrol) {
}

void AhcalHodoscopeDataCollector::DoConnect(eudaq::ConnectionSPC idx) {
   std::cout << "New connection: " << idx->GetName() << std::endl;
   std::unique_lock<std::mutex> lk(m_mtx_map);
   if (m_ahcal_conn_evts.empty() && m_ahcal_conn_evts.empty()) {
      m_conn_disconnected.clear();
      debug_unmatched = 0;
      tdc_hist_ahcal.clear();
      tdc_hist_hodoscope.clear();
      tdc_hist_difference.clear();
   }
   //TODO make a parameter for the name of the calice producer
   if (idx->GetName().find("alice") != std::string::npos) {
      m_ahcal_conn_evts[idx].clear();
      return;
   }
   if (idx->GetName().find("AHCAL") != std::string::npos) {
      m_ahcal_conn_evts[idx].clear();
      return;
   }
   if (idx->GetName().find("odoscope") != std::string::npos) {
      m_hodoscope_conn_evts[idx].clear();
      return;
   }
   std::cout << "#error: connection from unknown producer. The name should contain AHCAL, Calice or Hodoscope string." << std::endl;
}

void AhcalHodoscopeDataCollector::DoDisconnect(eudaq::ConnectionSPC idx) {
   std::cout << "Disconnecting: " << idx->GetName() << std::endl;
   std::unique_lock<std::mutex> lk(m_mtx_map);
   m_conn_disconnected.insert(idx);
   if (m_conn_disconnected.size() == (m_ahcal_conn_evts.size() + m_hodoscope_conn_evts.size())) {
      m_conn_disconnected.clear();
      m_ahcal_conn_evts.clear();
      m_hodoscope_conn_evts.clear();
      printStats();
      debug_unmatched = 0;
      tdc_hist_ahcal.clear();
      tdc_hist_hodoscope.clear();
      tdc_hist_difference.clear();
   }
}

void AhcalHodoscopeDataCollector::printStats() {
   std::cout << "-----------------------------------------------" << std::endl;
   std::cout << "- AHCAL TDC histogram -------------------------" << std::endl;
   std::cout << "-----------------------------------------------" << std::endl;
   for (auto& it : tdc_hist_ahcal) {
      std::cout << it.first << "\t" << it.second << std::endl;
   }
   std::cout << "-----------------------------------------------" << std::endl;
   std::cout << "- Hodoscope TDC histogram ---------------------" << std::endl;
   std::cout << "-----------------------------------------------" << std::endl;
   for (auto& it : tdc_hist_hodoscope) {
      std::cout << it.first << "\t" << it.second << std::endl;
   }
   std::cout << "-----------------------------------------------" << std::endl;
   std::cout << "- TDC difference histogram --------------------" << std::endl;
   std::cout << "-----------------------------------------------" << std::endl;
   for (auto& it : tdc_hist_difference) {
      std::cout << it.first << "\t" << it.second << std::endl;
   }
   std::cout << "-----------------------------------------------" << std::endl;
   std::cout << "Unmatched events: " << debug_unmatched << std::endl;
}

void AhcalHodoscopeDataCollector::DoReceive(eudaq::ConnectionSPC idx, eudaq::EventSP ev) {
   std::unique_lock<std::mutex> lk(m_mtx_map);
   eudaq::EventSP evsp = ev;

   auto it_que_ahcal = m_ahcal_conn_evts.find(idx);
   if (it_que_ahcal != m_ahcal_conn_evts.end()) {
      evsp->SetTag("SRC", it_que_ahcal->first->GetName());         //put the name of the producer into the data stream for later identification
      it_que_ahcal->second.push_back(evsp);
   } else {
      auto it_que_hodoscope = m_hodoscope_conn_evts.find(idx);
      if (it_que_hodoscope != m_hodoscope_conn_evts.end()) {
         evsp->SetTag("SRC", it_que_hodoscope->first->GetName());         //put the name of the producer into the data stream for later identification
         it_que_hodoscope->second.push_back(evsp);
      } else {
         EUDAQ_THROW("no matching queue found: it_que_ahcal == m_conn_evts.end() and it_que_hodoscope == m_hodoscope_conn_evts.end()");
      }
   }

   if (m_ahcal_conn_evts.size() < 1) {
      std::cout << "~" << std::flush; //not enough ahcal connections
      return;
   }
   if (m_hodoscope_conn_evts.size() < 2) {
      std::cout << "~" << std::flush; //not enough hodoscope connections
      return;
   }
   BuildEvent();
}

void AhcalHodoscopeDataCollector::BuildEvent() {
   bool everybodyHasEvent;
   while (true) { // try to build everything in the queue
      bool everybodyHasEvent = true;
//      eudaq::ConnectionSPC ahcal_conn;
      int roc_ahcal = -1;
      int bxid_ahcal = -1;

      for (auto & conn_evque : m_ahcal_conn_evts) {
//         std::cout << "1" << std::flush;
         if (conn_evque.second.size() < 1) return; //if no event in at least one queue -> quit
//            ahcal_conn = conn_evque.first;
//            ahcal_conn = conn_evque;
         roc_ahcal = conn_evque.second.front()->GetTag("ROC", -1);
         bxid_ahcal = conn_evque.second.front()->GetTag("BXID", -1);
         if ((roc_ahcal == -1) || (bxid_ahcal == -1) || (bxid_ahcal == 0)) {
            conn_evque.second.pop_front(); //untagged event. impossible to synchronize. dropping.
            std::cout << "#dropping ROC=" << roc_ahcal << " BXID=" << bxid_ahcal << std::endl;
            everybodyHasEvent = false; //a new check is required, because another event is now in front
            break; //no reason to continue looking
         }
      }
      //TODO only 1 AHCAL connection assumed.
//      std::cout << "2" << std::flush;
      if (!everybodyHasEvent) continue;
      bool matchFound = false;
      int tdc_ahcal = -1;
      int tdc_hodoscope = -1;
      int roc_hodoscope = -1;
      int bxid_hodoscope = -1;
      for (auto & conn_evque : m_hodoscope_conn_evts) {
         if (conn_evque.second.size() < 1) return; //if no event in at least one queue -> quit
         roc_hodoscope = -1;
         bxid_hodoscope = -1;
//         std::cout << "3" << std::flush;
         roc_hodoscope = conn_evque.second.front()->GetTag("ROC", -1);
         bxid_hodoscope = conn_evque.second.front()->GetTag("BXID", -1);
         if (roc_hodoscope < roc_ahcal) {
            //AHCAL has no matching. The front data are already from next ROC
            conn_evque.second.pop_front();
            everybodyHasEvent = false;
         }
         if (roc_hodoscope > roc_ahcal) {
            //no matching event from this hodoscope in this ROC. the event is from next ROC.
            continue;
         }
         if (roc_hodoscope == roc_ahcal) {
            if (bxid_hodoscope < bxid_ahcal) {
               if ((bxid_hodoscope + 1) == bxid_ahcal) std::cout << "#hodoscope matches the next bxid!" << std::endl;
               conn_evque.second.pop_front();
               everybodyHasEvent = false;
            }
            if (bxid_hodoscope > bxid_ahcal) {
               //no matching event from this hodoscope. This event is from next ROC
               continue;
            }
            if (bxid_hodoscope == bxid_ahcal) {
               matchFound = true;
            }
         }
      }
//      std::cout << "4" << std::flush;
      if (!everybodyHasEvent) continue;
//      std::cout << "5" << std::flush;
      if (!matchFound) {
         for (auto & conn_evque : m_ahcal_conn_evts) {
            tdc_ahcal = conn_evque.second.front()->GetTag("TrigBxidTdc", -1);
            tdc_hist_ahcal[conn_evque.second.front()->GetTag("TrigBxidTdc", -1)]++;
            conn_evque.second.pop_front();
         }
         debug_unmatched++;
         std::cout << "#no matching trigger from Hodoscope. ROC=" << roc_ahcal << " BXID=" << bxid_ahcal << " TDC_a=" << tdc_ahcal << ".";
         std::cout << " queue: ROC=" << roc_hodoscope << ", BXID=" << bxid_hodoscope << std::endl;
         continue;
      }
//      std::cout << "6" << std::flush;
      auto ev_sync = eudaq::Event::MakeUnique("CaliceAhcalHodoscope"); //new combined event
      ev_sync->SetFlagPacket();
      for (auto & conn_evque : m_ahcal_conn_evts) { //fill with ahcal events
         auto &ev_front = conn_evque.second.front();
         if ((conn_evque.second.front()->GetTag("ROC", -1) == roc_ahcal) && (conn_evque.second.front()->GetTag("BXID", -1) == bxid_ahcal)) {
            tdc_ahcal = conn_evque.second.front()->GetTag("TrigBxidTdc", -1);
            ev_sync->AddSubEvent(conn_evque.second.front());
            conn_evque.second.pop_front();
         }
      }
      for (auto &conn_evque : m_hodoscope_conn_evts) { //fill with hodoscope events
//         std::cout << "7" << std::flush;
         auto &ev_front = conn_evque.second.front();

         if ((conn_evque.second.front()->GetTag("ROC", -1) == roc_ahcal) && (conn_evque.second.front()->GetTag("BXID", -1) == bxid_ahcal)) {
            tdc_hodoscope = conn_evque.second.front()->GetTag("TrigBxidTdc", -1);
            ev_sync->AddSubEvent(ev_front);
            conn_evque.second.pop_front();
         }
      }
      tdc_hist_ahcal[tdc_ahcal]++;
      tdc_hist_hodoscope[tdc_hodoscope]++;
      tdc_hist_difference[tdc_ahcal - tdc_hodoscope]++;
      std::cout << "ROC=" << roc_ahcal << "\tBXID=" << bxid_ahcal << "\tTDC_a=" << tdc_ahcal << "\tTDC_h=" << tdc_hodoscope << "\tTDC_diff="
            << tdc_ahcal - tdc_hodoscope << "\t#OK" << std::endl;
//      ev_sync->Print(std::cout);
      WriteEvent(std::move(ev_sync));
      continue;
   }
}
