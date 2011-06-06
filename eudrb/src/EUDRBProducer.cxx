#include "EUDRBController.hh"
#include "eudaq/Producer.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Timer.hh"

#include <iostream>
#include <ostream>
#include <cstdio>

#include "EUDRBController.hh"

using eudaq::EUDRBController;
using eudaq::RawDataEvent;
using eudaq::to_string;
using eudaq::Timer;

static unsigned MAX_ERR_CLOCKSYNC = 10, MAX_ERR_2 = 10;

inline bool doprint(int n) {
  if (n < 10) return true;
  if (n < 100 && n%10 == 0) return true;
  if (n < 1000 && n%100 == 0) return true;
  if (n%1000 == 0) return true;
  return false;
}

template <typename T>
void DoTags(const std::string & name, T (EUDRBController::*func)() const,
            eudaq::Event & ev, std::vector<counted_ptr<EUDRBController> > & boards) {
  const EUDRBController * ptr = boards[0].get();
  std::string val = to_string((ptr->*func)());
  for (size_t i = 1; i < boards.size(); ++i) {
    const EUDRBController * ptr = boards[i].get();
    if (val != to_string((ptr->*func)())) {
      val = "Mixed";
      break;
    }
  }
  ev.SetTag(name, val);
  if (val == "Mixed") {
    for (size_t i = 0; i < boards.size(); ++i) {
      const EUDRBController * ptr = boards[i].get();
      ev.SetTag(name + to_string(i), to_string((ptr->*func)()));
    }
  }
}

class EUDRBProducer : public eudaq::Producer {
public:
  EUDRBProducer(const std::string & name, const std::string & runcontrol)
    : eudaq::Producer(name, runcontrol),
      done(false),
      started(false),
      juststopped(false),
      n_error(0),
      m_buffer(32768), // will get resized as needed
      m_idoffset(0),
      m_version(-1),
      m_numerr_clocksync(0),
      m_num_err_2(0)
  {
    m_buffer.reserve(40000);
  }
  virtual void OnConfigure(const eudaq::Configuration & param) {
    SetStatus(eudaq::Status::LVL_OK, "Wait");
    try {
      std::cout << "Configuring (" << param.Name() << ")..." << std::endl;
      int numboards = param.Get("NumBoards", 0);
      m_idoffset = param.Get("IDOffset", 0);
      m_resetbusy = param.Get("ResetBusy", 0);
      m_boards.clear();
      m_version = param.Get("Version", 0);
      for (int n_eudrb = 0; n_eudrb < numboards; ++n_eudrb) {
        int id = m_idoffset + n_eudrb;
        int version = param.Get("Board" + to_string(id) + ".Version", "Version", 0);
        unsigned slot = param.Get("Board" + to_string(id) + ".Slot", "Slot", 0);
        unsigned addr = param.Get("Board" + to_string(id) + ".Addr", "Addr", 0);
        if (slot == 0) slot = addr >> 27;
        if (slot != 0 && addr != 0 && addr != (slot << 27)) EUDAQ_THROW("Mismatched Slot and Addr for board " + to_string(id));
        if (slot < 2 || slot > 21) EUDAQ_THROW("Bad Slot number (" + to_string(slot) + ") for board " + to_string(id));
        m_boards.push_back(counted_ptr<EUDRBController>(new EUDRBController(id-m_idoffset, slot, version)));
      }
      m_unsync = param.Get("Unsynchronized", 0);
      std::cout << "Running in " << (m_unsync ? "UNSYNCHRONIZED" : "synchronized") << " mode" << std::endl;
      m_master = param.Get("Master", -1);
      std::cout << "Pausing while TLU gets configured..." << std::endl;
      eudaq::mSleep(4000);
      std::cout << "OK, let's continue." << std::endl;
      if (m_master < 0) m_master += numboards;
      for (int n_eudrb = 0; n_eudrb < numboards; ++n_eudrb) {
        m_boards[n_eudrb]->Configure(param, m_master);
      }
      std::cout << "Waiting for boards to reset..." << std::endl;

      bool ok = true;
      for (size_t n_eudrb = 0; n_eudrb < m_boards.size(); n_eudrb++) {
        // give the first board ~20 seconds, then only wait 2 extra seconds for each other board
        ok &= m_boards[n_eudrb]->WaitForReady((n_eudrb == 0) ? 20.0 : 2.0);
      }

      std::cout << (ok ? "OK" : "*** Timeout ***") << std::endl;

      m_pedfiles.clear();
      for (size_t n_eudrb = 0; n_eudrb < m_boards.size(); n_eudrb++) {
        m_pedfiles.push_back(m_boards[n_eudrb]->PostConfigure(param, m_master));
      }

      std::cout << "...Configured (" << param.Name() << ")" << std::endl;
      EUDAQ_INFO("Configured (" + param.Name() + ")");
      SetStatus(eudaq::Status::LVL_OK, "Configured (" + param.Name() + ")");
    } catch (const std::exception & e) {
      EUDAQ_ERROR("Config exception: " + to_string(e.what()));
      SetStatus(eudaq::Status::LVL_ERROR, "Configuration Error");
    } catch (...) {
      EUDAQ_ERROR("Unknown Config exception");
      SetStatus(eudaq::Status::LVL_ERROR, "Configuration Error");
    }
  }
  virtual void OnStartRun(unsigned param) {
    try {
      m_run = param;
      m_ev = 0;
      m_numerr_clocksync = 0;
      m_num_err_2 = 0;
      std::cout << "Start Run: " << param << std::endl;
#if 0
      for (size_t i = 0; i < m_boards.size(); ++i) {
        m_boards[i]->ResetBoard();
      }

      std::cout << "Waiting for boards to reset..." << std::endl;
      bool ok = true;
      for (size_t n_eudrb = 0; n_eudrb < m_boards.size(); n_eudrb++) {
        // give the first board ~20 seconds, then only wait 2 extra seconds for each other board
        ok &= m_boards[n_eudrb]->WaitForReady((n_eudrb == 0) ? 20.0 : 2.0);
      }
      std::cout << (ok ? "OK" : "*** Timeout ***") << std::endl;

      m_boards[m_master]->SendStartPulse();
      
#endif
      // EUDRB startup (activation of triggers etc)
      RawDataEvent ev(RawDataEvent::BORE("EUDRB", m_run));

      DoTags("VERSION", &EUDRBController::Version, ev, m_boards);
      DoTags("DET", &EUDRBController::Det, ev, m_boards);
      DoTags("MODE", &EUDRBController::Mode, ev, m_boards);
      DoTags("AdcDelay", &EUDRBController::AdcDelay, ev, m_boards);
      DoTags("ClkSelect", &EUDRBController::ClkSelect, ev, m_boards);
      DoTags("PostDetResetDelay", &EUDRBController::PostDetResetDelay, ev, m_boards);
      for (size_t i = 0; i < m_boards.size(); ++i) {
        ev.SetTag("ID" + to_string(i), to_string(i + m_idoffset));
        if (m_pedfiles[i] != "") ev.SetTag("PEDESTAL" + to_string(i), m_pedfiles[i]);
        if (m_boards[i]->HasDebugRegister()) {
          ev.SetTag("DEBUGREG" + to_string(i), m_boards[i]->UpdateDebugRegister());
        }
      }
      ev.SetTag("BOARDS", m_boards.size());
      ev.SetTag("Unsynchronized", m_unsync);
      ev.SetTag("ResetBusy", m_resetbusy);
      SendEvent(ev);
      eudaq::mSleep(100);
      started=true;
      SetStatus(eudaq::Status::LVL_OK, "Started");
    } catch (const std::exception & e) {
      printf("Caught exception: %s\n", e.what());
      SetStatus(eudaq::Status::LVL_ERROR, "Start Error");
    } catch (...) {
      printf("Unknown exception\n");
      SetStatus(eudaq::Status::LVL_ERROR, "Start Error");
    }
  }
  void Process() {
    if (!started) {
      eudaq::mSleep(10);
      return;
    }

    //static Timer t_total;
    Timer t_event;

    RawDataEvent ev("EUDRB", m_run, m_ev);

    bool ok = (m_version < 3) ? ReadoutEventOld(ev) : ReadoutEventNew(ev);
    for (size_t i = 0; i < m_boards.size(); ++i) {
      if (m_boards[i]->HasDebugRegister()) {
        unsigned oldval = m_boards[i]->GetDebugRegister();
        unsigned newval = m_boards[i]->UpdateDebugRegister();
        if (newval != oldval) {
          ev.SetTag("DEBUGREG" + to_string(i), eudaq::to_hex(newval, 8));
          ev.SetTag("DEBUGREGOLD" + to_string(i), eudaq::to_hex(oldval, 8));
          if (m_numerr_clocksync < MAX_ERR_CLOCKSYNC) {
            m_numerr_clocksync++;
            EUDAQ_WARN("Clock sync error on board " + to_string(i) + " in event " + to_string(m_ev) +
                       " (" + eudaq::to_hex(oldval, 8) + " -> " + eudaq::to_hex(newval, 8) + ")");
          }
        }
      }
    }
    
    if (ok) {
      Timer t_send;
      SendEvent(ev);
      t_send.Stop();
      //if (doprint(m_ev)) printf("*** Event %d of length %ld sent!\n",m_ev, total_bytes);
      n_error=0;
      if (doprint(m_ev)) {
        std::cout << "EV " << m_ev << ": send=" << t_send.uSeconds() << "us, "
                  << "event=" << t_event.uSeconds() << "us, "
          //<< "running=" << t_total.mSeconds() << "ms, "
          //<< "bytes=" << total_bytes << "\n"
                  << std::endl;
      }
      ++m_ev;
    }
  }
  bool ReadoutEventOld(RawDataEvent & ev) {
    static bool readingstarted = false;
    unsigned long total_bytes=0;
    std::cout << "--" << std::endl; 
    for (size_t i=0; i <= m_boards.size(); ++i) {
      size_t n_test = i < m_boards.size() ? i : m_master; // make sure master is read out last
      std::cout << i << " " << m_master << " " << n_test << " " << m_boards.size()<< std::endl;
      if ((int)i == m_master) continue; // don't do master yet
      size_t n_eudrb = i < m_boards.size() ? i : m_master; // make sure master is read out last
      Timer t_board;
      bool badev=false;
      Timer t_wait;
      int datasize = m_boards[n_eudrb]->EventDataReady_size();
      t_wait.Stop();
      if (datasize == 0 && n_eudrb == 0) {
        if (juststopped) started = false;
        return false;
      }
      if (!readingstarted) {
        readingstarted = true;
        //t_total.Restart();
      }

      Timer t_mblt, t_reset;
      unsigned long number_of_bytes=datasize*4;

      //printf("number of bytes = %ld\n",number_of_bytes);
      if (number_of_bytes <= 0) {
        printf("Board: %d, Event: %d, empty\n",(int)n_eudrb,m_ev);
        EUDAQ_WARN("Board " + to_string(n_eudrb) +" in Event " + to_string(m_ev) + " empty");
        badev=true;
      } else {
        if (number_of_bytes>1048576) {
          printf("Board: %d, Event: %d, number of bytes = %ld\n",(int)n_eudrb,m_ev,number_of_bytes);
          EUDAQ_WARN("Board " + to_string(n_eudrb) +" in Event " + to_string(m_ev) + " too big: " + to_string(number_of_bytes));
          badev=true;
        }
        //t_mblt.Restart();
        m_buffer.resize(((number_of_bytes+7) & ~7) / 4);
        m_boards[n_eudrb]->ReadEvent(m_buffer);
        t_mblt.Stop();
        if (number_of_bytes > 16) ev.SetFlags(eudaq::Event::FLAG_HITS);

        // Reset the BUSY on the MASTER after all MBLTs are done
        t_reset.Restart();
        if (m_resetbusy && (int)n_eudrb == m_master) { // Master
          m_boards[n_eudrb]->ResetTriggerBusy();
          //eudaq::mSleep(20);
        }
        t_reset.Stop();

        if (/*doprint(ev) ||*/ badev) {
          printf("event   =0x%x, eudrb   =%3d, nbytes = %ld\n",m_ev,(int)n_eudrb,number_of_bytes);
          printf("\theader =0x%lx\n", m_buffer[0]);
          if ( (m_buffer[number_of_bytes/4-2]&0x54000000)==0x54000000) printf("\ttrailer=0x%lx\n", m_buffer[number_of_bytes/4-2]);
          else printf("\ttrailer=x%lx\n", m_buffer[number_of_bytes/4-3]);
          badev=false;
        }
      }
      Timer t_add;
      ev.AddBlock(m_idoffset+n_eudrb, &m_buffer[0], number_of_bytes);
      t_add.Stop();
      total_bytes+=(number_of_bytes+7)&~7;
      if (doprint(m_ev)) {
        std::cout << "B" << n_eudrb << ": wait=" << t_wait.uSeconds() << "us, "
                  << "mblt=" << t_mblt.uSeconds() << "us, "
                  << "reset=" << t_reset.uSeconds() << "us, "
                  << "add=" << t_add.uSeconds() << "us, "
                  << "board=" << t_board.uSeconds() << "us, "
                  << "bytes=" << number_of_bytes << "\n";
      }
    }
    std::cout << "--" << std::endl;
    return true;
  }
  bool ReadoutEventNew(RawDataEvent & ev) {
    static bool readingstarted = false;
    unsigned long total_bytes=0;
    Timer t_wait;
    //std::cout << "ReadoutEventNew" << std::endl;
    if (!m_boards[m_boards.size()-1]->EventDataReady()) {
      //if (!m_boards[0]->EventDataReady()) {
      if (juststopped) {
        started = false;
        std::cout << "Stopping readout" << std::endl;
      }
      return false;
    }
    t_wait.Stop();
    if (!readingstarted) {
      readingstarted = true;
    }
    //std::cout<< "--"<< std::endl;
    unsigned long off = 0;
    unsigned long pivot = 0;
    bool off_set = false;
    bool pivot_set = false;

    for (size_t i=0; i <= m_boards.size(); ++i) {
      // size_t n_test = i < m_boards.size() ? i : m_master; 
      //std::cout << i << " " << m_master << " " << n_test << " " << m_boards.size()<< std::endl;      
      if ((int)i == m_master) continue; // don't do master yet
      size_t n_eudrb = i < m_boards.size() ? i : m_master; // make sure master is read out last
      Timer t_board;
      bool badev=false;

      if (m_boards[n_eudrb]->IsDisabled()) {
        continue;
      }
      if (!m_boards[n_eudrb]->EventDataReady()) {
        EUDAQ_ERROR("Board " + to_string(n_eudrb) + " not ready in event " + to_string(m_ev) + ", disabling");
        //m_boards[n_eudrb]->Disable();
        continue;
      }

      size_t blockindex = ev.AddBlock(m_idoffset+n_eudrb);

      Timer t_mblt, t_reset;
      //std::cout << "Reading block 1, board " << n_eudrb << std::endl;
      if (doprint(m_ev)) std::cout << "Checking ready board " << n_eudrb << std::endl;
      bool ready = false;
      Timer t;
      while (t.Seconds() < 0.1) {
        ready = m_boards[n_eudrb]->EventDataReady();
        if (ready) break;
      }
      if (!ready) {
        EUDAQ_ERROR("Board " + to_string(n_eudrb) + " not ready in event " + to_string(m_ev));
      }
      m_buffer.resize(4);
      m_boards[n_eudrb]->ReadEvent(m_buffer);
      ev.AppendBlock(blockindex, m_buffer);

      if (m_boards[0]->Det() == "MIMOSA26") {
        if (!off_set) {
          off = m_buffer[3];
          off_set = true;
        } else {
          unsigned long diff = (9216 + off - m_buffer[3]) % 9216;
          if(diff > 4 && diff < 9212) {
            EUDAQ_WARN("data consistency check 1 for board " + to_string(n_eudrb) +" in event " + to_string(m_ev) + " failed! The offset difference in pixel is " + to_string(diff) + "!");
//            std::cout << "------ header -----" << std::endl; 
//            for(size_t u = 0; u < m_buffer.size(); u++) {
//              std::cout << "board " << n_eudrb<< " - " << u << " : ";
//              printf("%x\n", (unsigned int)m_buffer[u]);
//            } 
//            std::cout << "->->->->->->->->->->->-" << std::endl;
          }
        }
      }
      const unsigned long number_of_bytes = 4 * (m_buffer[0] & 0xFFFFF);
            
      if (doprint(m_ev)) std::cout << "DEBUG: read leading words board " << n_eudrb << ", remaining = " << number_of_bytes << std::endl;

      //printf("number of bytes = %ld\n",number_of_bytes);
      if (number_of_bytes == 0) {
        printf("Board: %d, Event: %d, empty\n",(int)n_eudrb,m_ev);
        EUDAQ_WARN("Board " + to_string(n_eudrb) +" in Event " + to_string(m_ev) + " empty");
        badev=true;
      } else {
        //t_mblt.Restart();
        m_buffer.resize(number_of_bytes / 4);
        
        if (doprint(m_ev)) std::cout << "Reading block 2, board " << n_eudrb << std::endl;
        m_boards[n_eudrb]->ReadEvent(m_buffer);
        if (doprint(m_ev)) std::cout << "OK" << std::endl;
        
	if (m_boards[0]->Det() == "MIMOSA26") {
          const unsigned long p = m_buffer[1] & 0x3FFF;
          if(!pivot_set)
            {
              pivot = p;
              pivot_set = true;
            }
          else
            { 
             
            unsigned long diff = (9216 + pivot - p) % 9216;
             
            if(diff > 4 && diff < 9212)
              {
                if (m_num_err_2 < MAX_ERR_2) {
                  m_num_err_2++;
                  EUDAQ_WARN("data consistency check 2 for board " + to_string(n_eudrb) +" in event " + to_string(m_ev) + " failed! The pivot pixel  difference is " + to_string(diff) + "!");
                }
//                std::cout << "------ data -----" << std::endl; 
//
//                for(size_t u = 0; u < m_buffer.size(); u++)
//                  {
//                    std::cout <<"board " << n_eudrb<< " - " <<  u << " : ";
//                    printf("%x\n", (unsigned int) m_buffer[u]);
//                  } 
//                std::cout << "-----------" << std::endl; 
              }
          }
	}
       
        t_mblt.Stop();
        if (number_of_bytes > 16) ev.SetFlags(eudaq::Event::FLAG_HITS);

        // Reset the BUSY on the MASTER after all MBLTs are done
        t_reset.Restart();
        if (m_resetbusy && (int)n_eudrb == m_master) { // Master
          m_boards[n_eudrb]->ResetTriggerBusy();
          eudaq::mSleep(20);
        }
        t_reset.Stop();

        if (/*doprint(m_ev) ||*/ badev) {
          printf("event   =0x%x, eudrb   =%3d, nbytes = %ld\n",m_ev,(int)n_eudrb,number_of_bytes);
          printf("\theader =0x%lx\n", m_buffer[0]);
          if ( (m_buffer[number_of_bytes/4-2]&0x54000000)==0x54000000) printf("\ttrailer=0x%lx\n", m_buffer[number_of_bytes/4-2]);
          else printf("\ttrailer=x%lx\n", m_buffer[number_of_bytes/4-3]);
          badev=false;
        }
      }
      Timer t_add;
      ev.AppendBlock(blockindex, &m_buffer[0], number_of_bytes);
      t_add.Stop();
      total_bytes += number_of_bytes;
      if (doprint(m_ev)) {
        std::cout << "B" << n_eudrb << ": wait=" << t_wait.uSeconds() << "us, "
                  << "mblt=" << t_mblt.uSeconds() << "us, "
                  << "reset=" << t_reset.uSeconds() << "us, "
                  << "add=" << t_add.uSeconds() << "us, "
                  << "board=" << t_board.uSeconds() << "us, "
                  << "bytes=" << number_of_bytes << "\n";
      }
    }
    //std::cout<< "--" << std::endl;
    return true;
  }
  virtual void OnStopRun() {
    try {
      std::cout << "Stopping Run" << std::endl;
      // EUDRB stop
      juststopped = true;
      while (started) {
        eudaq::mSleep(100);
      }
      juststopped = false;
      
#if 0
      for (size_t i = 0; i < m_boards.size(); ++i) {
        m_boards[i]->ResetBoard();
      }

      std::cout << "Waiting for boards to reset..." << std::endl;
      bool ok = true;
      for (size_t n_eudrb = 0; n_eudrb < m_boards.size(); n_eudrb++) {
        // give the first board ~20 seconds, then only wait 2 extra seconds for each other board
        ok &= m_boards[n_eudrb]->WaitForReady((n_eudrb == 0) ? 20.0 : 2.0);
      }
      std::cout << (ok ? "OK" : "*** Timeout ***") << std::endl;

      m_boards[m_master]->SendStartPulse();
      
#endif

      std::cout << "Sending EORE." << std::endl;
      SendEvent(RawDataEvent::EORE("EUDRB", m_run, m_ev++));
      std::cout << "Setting status." << std::endl;
      SetStatus(eudaq::Status::LVL_OK, "Stopped");
    } catch (const std::exception & e) {
      printf("Caught exception: %s\n", e.what());
      SetStatus(eudaq::Status::LVL_ERROR, "Stop Error");
    } catch (...) {
      printf("Unknown exception\n");
      SetStatus(eudaq::Status::LVL_ERROR, "Stop Error");
    }
    std::cout << "OK" << std::endl;
  }
  virtual void OnTerminate() {
    std::cout << "Terminating..." << std::endl;
    done = true;
  }

  unsigned m_run, m_ev;
  bool done, started, juststopped, m_resetbusy, m_unsync;
  int n_error;
  std::vector<unsigned long> m_buffer;
  std::vector<counted_ptr<EUDRBController> > m_boards;
  //int fdOut;
  int m_idoffset, m_version, m_master;
  std::vector<std::string> m_pedfiles;
  unsigned m_numerr_clocksync, m_num_err_2;
};

int main(int /*argc*/, const char ** argv) {
  eudaq::OptionParser op("EUDAQ EUDRB Producer", "1.0", "The Producer task for reading out EUDRB boards via VME");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:44000", "address",
                                   "The address of the RunControl application");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
                                   "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name (op, "n", "name", "EUDRB", "string",
                                   "The name of this Producer");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    EUDRBProducer producer(name.Value(), rctrl.Value());

    unsigned evtno;
    evtno=0;
    do {
      producer.Process();
      //      usleep(100);
    } while (!producer.done);
    std::cout << "Quitting" << std::endl;
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}

