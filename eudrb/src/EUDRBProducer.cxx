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

inline bool doprint(int n) {
  if (n < 10) return true;
  if (n < 100 && n%10 == 0) return true;
  if (n < 1000 && n%100 == 0) return true;
  if (n%1000 == 0) return true;
  return false;
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
      m_version(-1)
    {
    }
  virtual void OnConfigure(const eudaq::Configuration & param) {
    SetStatus(eudaq::Status::LVL_OK, "Wait");
    try {
      std::cout << "Configuring (" << param.Name() << ")..." << std::endl;
      int numboards = param.Get("NumBoards", 0);
      m_idoffset = param.Get("IDOffset", 0);
      m_resetbusy = param.Get("ResetBusy", 1);
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
        m_boards.push_back(counted_ptr<EUDRBController>(new EUDRBController(id, slot, version)));
      }
      bool unsync = param.Get("Unsynchronized", 0);
      std::cout << "Running in " << (unsync ? "UNSYNCHRONIZED" : "synchronized") << " mode" << std::endl;
      m_master = param.Get("Master", -1);
      std::cout << "Pausing while TLU gets configured..." << std::endl;
      eudaq::mSleep(1000);
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
      printf("Caught exception: %s\n", e.what());
      SetStatus(eudaq::Status::LVL_ERROR, "Configuration Error");
    } catch (...) {
      printf("Unknown exception\n");
      SetStatus(eudaq::Status::LVL_ERROR, "Configuration Error");
    }
  }
  virtual void OnStartRun(unsigned param) {
    try {
      m_run = param;
      m_ev = 0;
      std::cout << "Start Run: " << param << std::endl;
      // EUDRB startup (activation of triggers etc)
      RawDataEvent ev(RawDataEvent::BORE("EUDRB", m_run));

      std::string version = to_string(m_boards[0]->Version());
      std::string det = m_boards[0]->Det();
      std::string mode = m_boards[0]->Mode();
      for (size_t i = 1; i < m_boards.size(); ++i) {
        std::string ver = to_string(m_boards[i]->Version());
        if (ver != version) version = "Mixed";
        if (m_boards[i]->Det() != det) det = "Mixed";
        if (m_boards[i]->Mode() != mode) mode = "Mixed";
      }
      ev.SetTag("VERSION", to_string(version));
      ev.SetTag("DET", det);
      ev.SetTag("MODE", mode);
      ev.SetTag("BOARDS", to_string(m_boards.size()));
      for (size_t i = 0; i < m_boards.size(); ++i) {
        ev.SetTag("ID" + to_string(i), to_string(i + m_idoffset));
        if (version == "Mixed") ev.SetTag("VERSION" + to_string(i), to_string(m_boards[i]->Version()));
        if (det     == "Mixed") ev.SetTag("DET" + to_string(i), m_boards[i]->Det());
        if (mode    == "Mixed") ev.SetTag("MODE" + to_string(i), m_boards[i]->Mode());
        if (m_pedfiles[i] != "") ev.SetTag("PEDESTAL" + to_string(i), m_pedfiles[i]);
      }
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
    for (size_t i=0; i <= m_boards.size(); ++i) {
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
    return true;
  }
  bool ReadoutEventNew(RawDataEvent & ev) {
    static bool readingstarted = false;
    unsigned long total_bytes=0;
    Timer t_wait;
    if (!m_boards[m_boards.size()-1]->EventDataReady()) {
      if (juststopped) started = false;
      return false;
    }
    t_wait.Stop();
    if (!readingstarted) {
      readingstarted = true;
    }
    for (size_t i=0; i <= m_boards.size(); ++i) {
      if ((int)i == m_master) continue; // don't do master yet
      size_t n_eudrb = i < m_boards.size() ? i : m_master; // make sure master is read out last
      Timer t_board;
      bool badev=false;

      size_t blockindex = ev.AddBlock(m_idoffset+n_eudrb);

      Timer t_mblt, t_reset;

      m_buffer.resize(4);
      m_boards[n_eudrb]->ReadEvent(m_buffer);
      ev.AppendBlock(blockindex, m_buffer);

      unsigned long number_of_bytes = 4 * (m_buffer[0] & 0xFFFFF);
      if (doprint(m_ev)) std::cout << "DEBUG: read leading words, remaining = " << number_of_bytes << std::endl;

      //printf("number of bytes = %ld\n",number_of_bytes);
      if (number_of_bytes == 0) {
        printf("Board: %d, Event: %d, empty\n",(int)n_eudrb,m_ev);
        EUDAQ_WARN("Board " + to_string(n_eudrb) +" in Event " + to_string(m_ev) + " empty");
        badev=true;
      } else {
        //t_mblt.Restart();
        m_buffer.resize(number_of_bytes / 4);
        m_boards[n_eudrb]->ReadEvent(m_buffer);
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
      SendEvent(RawDataEvent::EORE("EUDRB", m_run, m_ev++));
      SetStatus(eudaq::Status::LVL_OK, "Stopped");
    } catch (const std::exception & e) {
      printf("Caught exception: %s\n", e.what());
      SetStatus(eudaq::Status::LVL_ERROR, "Stop Error");
    } catch (...) {
      printf("Unknown exception\n");
      SetStatus(eudaq::Status::LVL_ERROR, "Stop Error");
    }
  }
  virtual void OnTerminate() {
    std::cout << "Terminating..." << std::endl;
    done = true;
  }

  unsigned m_run, m_ev;
  bool done, started, juststopped, m_resetbusy;
  int n_error;
  std::vector<unsigned long> m_buffer;
  std::vector<counted_ptr<EUDRBController> > m_boards;
  //int fdOut;
  int m_idoffset, m_version, m_master;
  std::vector<std::string> m_pedfiles;
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

