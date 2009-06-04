#include "EUDRBController.hh"
#include "eudaq/Producer.hh"
#include "eudaq/EUDRBEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Timer.hh"

#include <iostream>
#include <ostream>
#include <cstdio>

#include "EUDRBController.hh"

using eudaq::EUDRBController;
using eudaq::EUDRBEvent;
using eudaq::to_string;
using eudaq::Timer;

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
  void Process() {
    if (!started) {
      eudaq::mSleep(1);
      return;
    }

    static Timer t_total;
    static bool readingstarted = false;
    Timer t_event;
    unsigned long total_bytes=0;

    EUDRBEvent ev(m_run, m_ev+1);

    for (size_t n_eudrb=0; n_eudrb < m_boards.size(); n_eudrb++) {
      Timer t_board;
      bool badev=false;
      Timer t_wait;
      int datasize = m_boards[n_eudrb]->EventDataReady_size();
      t_wait.Stop();
      if (datasize == 0 && n_eudrb == 0) {
        if (juststopped) started = false;
        break;
      }
      if (!readingstarted) {
        readingstarted = true;
        t_total.Restart();
      }

      unsigned long number_of_bytes=datasize*4;
      //printf("number of bytes = %ld\n",number_of_bytes);
      Timer t_mblt, t_reset;
      if (number_of_bytes == 0) {
        printf("Board: %d, Event: %d, empty\n",(int)n_eudrb,m_ev);
        EUDAQ_WARN("Board " + to_string(n_eudrb) +" in Event " + to_string(m_ev) + " empty");
        badev=true;
      } else {
        if (number_of_bytes>1048576) {
          printf("Board: %d, Event: %d, number of bytes = %ld\n",(int)n_eudrb,m_ev,number_of_bytes);
          EUDAQ_WARN("Board " + to_string(n_eudrb) +" in Event " + to_string(m_ev) + " too big: " + to_string(number_of_bytes));
          badev=true;
        }
        t_mblt.Restart();
        m_buffer.resize(((number_of_bytes+7) & ~7) / 4);
        m_boards[n_eudrb]->ReadEvent(m_buffer);
        t_mblt.Stop();
        if (number_of_bytes > 16) ev.SetFlags(eudaq::Event::FLAG_HITS);

        // Reset the BUSY on the MASTER after all MBLTs are done
        t_reset.Restart();
        if ((int)n_eudrb == m_master) { // Master
          m_boards[n_eudrb]->ResetTriggerBusy();
        }
        t_reset.Stop();

        if (/*m_ev<10 || m_ev%100==0 ||*/ badev) {
          printf("event   =0x%x, eudrb   =%3d, nbytes = %ld\n",m_ev,(int)n_eudrb,number_of_bytes);
          printf("\theader =0x%lx\n", m_buffer[0]);
          if ( (m_buffer[number_of_bytes/4-2]&0x54000000)==0x54000000) printf("\ttrailer=0x%lx\n", m_buffer[number_of_bytes/4-2]);
          else printf("\ttrailer=x%lx\n", m_buffer[number_of_bytes/4-3]);
          badev=false;
        }
      }
      Timer t_add;
      ev.AddBoard(n_eudrb, &m_buffer[0], number_of_bytes);
      t_add.Stop();
      total_bytes+=(number_of_bytes+7)&~7;
      if (m_ev % 100 == 0) {
        std::cout << "B" << n_eudrb << ": wait=" << t_wait.uSeconds() << "us, "
                  << "mblt=" << t_mblt.uSeconds() << "us, "
                  << "reset=" << t_reset.uSeconds() << "us, "
                  << "add=" << t_add.uSeconds() << "us, "
                  << "board=" << t_board.uSeconds() << "us, "
                  << "bytes=" << number_of_bytes << "\n";
      }
    }

    if (total_bytes) {
      ++m_ev;
      Timer t_send;
      SendEvent(ev);
      t_send.Stop();
      //if (m_ev<10 || m_ev%100==0) printf("*** Event %d of length %ld sent!\n",m_ev, total_bytes);
      n_error=0;
      std::cout << "EV " << m_ev << ": send=" << t_send.uSeconds() << "us, "
                << "event=" << t_event.uSeconds() << "us, "
                << "running=" << t_total.mSeconds() << "ms, "
                << "bytes=" << total_bytes << "\n" << std::endl;
    }
  }
  virtual void OnConfigure(const eudaq::Configuration & param) {
    SetStatus(eudaq::Status::LVL_OK, "Wait");
    try {
      std::cout << "Configuring (" << param.Name() << ")..." << std::endl;
      int numboards = param.Get("NumBoards", 0);
      m_idoffset = param.Get("IDOffset", 0);
      m_boards.clear();
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
      if (m_master < 0) m_master += numboards;
      for (int n_eudrb = 0; n_eudrb < numboards; ++n_eudrb) {
        m_boards[n_eudrb]->Configure(param, m_master);
      }
      std::cout << "Waiting for boards to reset..." << std::endl;

      bool ok = true;
      for (size_t n_eudrb = 0; n_eudrb < m_boards.size(); n_eudrb++) {
        // give the first board ~20 seconds, then only wait 1 extra second for each other board
        ok &= m_boards[n_eudrb]->WaitForReady(1.0 + 20.0 * (n_eudrb == 0));
      }

      std::cout << (ok ? "OK" : "*** Timeout ***") << std::endl;
      // new loop added by Angelo
//       for (size_t n_eudrb = 0; n_eudrb < boards.size(); n_eudrb++) {
//         unsigned long data = 0x40;
//         if (boards[n_eudrb].zs) data |= 0x20;
//         if (n_eudrb==boards.size()-1) data |= 0x2000;
//         boards[n_eudrb].vmes->Write(0, data);
//         data=0;
//         if (boards[n_eudrb].zs) data |= 0x20;
//         if (n_eudrb==boards.size()-1) data |= 0x2000;
//         boards[n_eudrb].vmes->Write(0, data);
//       }

      for (size_t n_eudrb = 0; n_eudrb < m_boards.size(); n_eudrb++) {
        m_boards[n_eudrb]->ConfigurePedestals(param);
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
      EUDRBEvent ev(EUDRBEvent::BORE(m_run));
      std::string version = to_string(m_boards[0]->Version());
      for (size_t i = 1; i < m_boards.size(); ++i) {
        std::string ver = to_string(m_boards[i]->Version());
        if (ver != version) version = "Mixed";
      }
      ev.SetTag("VERSION", to_string(version));
      std::string det = m_boards[0]->Det();
      for (size_t i = 1; i < m_boards.size(); ++i) {
        if (m_boards[i]->Det() != det) det = "Mixed";
      }
      ev.SetTag("DET", det);
      for (size_t i = 0; i < m_boards.size(); ++i) {
        ev.SetTag("DET" + to_string(i), m_boards[i]->Det());
      }
      std::string mode = m_boards[0]->Mode();
      for (size_t i = 1; i < m_boards.size(); ++i) {
        if (m_boards[i]->Mode() != mode) mode = "Mixed";
      }
      ev.SetTag("MODE", mode);
      for (size_t i = 0; i < m_boards.size(); ++i) {
        ev.SetTag("MODE" + to_string(i), m_boards[i]->Mode());
      }
      ev.SetTag("BOARDS", to_string(m_boards.size()));
      //ev.SetTag("ROWS", to_string(256))
      //ev.SetTag("COLS", to_string(66))
      //ev.SetTag("MATS", to_string(4))
      for (size_t i = 0; i < m_boards.size(); ++i) {
        ev.SetTag("ID" + to_string(i), to_string(i + m_idoffset));
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
  virtual void OnStopRun() {
    try {
      std::cout << "Stopping Run" << std::endl;
      // EUDRB stop
      juststopped = true;
      while (started) {
        eudaq::mSleep(100);
      }
      juststopped = false;
      SendEvent(EUDRBEvent::EORE(m_run, ++m_ev));
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
  virtual void OnReset() {
    SetStatus(eudaq::Status::LVL_OK, "Wait");
    try {
      std::cout << "Reset" << std::endl;
      for (size_t n_eudrb = 0; n_eudrb < m_boards.size(); n_eudrb++) {
        m_boards[n_eudrb]->ResetBoard();
      }
      for (size_t n_eudrb = 0; n_eudrb < m_boards.size(); n_eudrb++) {
        m_boards[n_eudrb]->WaitForReady();
      }
      SetStatus(eudaq::Status::LVL_OK, "Reset");
    } catch (const std::exception & e) {
      printf("Caught exception: %s\n", e.what());
      SetStatus(eudaq::Status::LVL_ERROR, "Reset Error");
    } catch (...) {
      printf("Unknown exception\n");
      SetStatus(eudaq::Status::LVL_ERROR, "Reset Error");
    }
  }

  unsigned m_run, m_ev;
  bool done, started, juststopped;
  int n_error;
  std::vector<unsigned long> m_buffer;
  std::vector<counted_ptr<EUDRBController> > m_boards;
  //int fdOut;
  int m_idoffset, m_version, m_master;
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

