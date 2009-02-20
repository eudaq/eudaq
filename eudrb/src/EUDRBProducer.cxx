#include "eudaq/Producer.hh"
#include "eudaq/EUDRBEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Timer.hh"

#include <iostream>
#include <ostream>
#include <fstream>
#include <cctype>

// stuff for VME/C-related business
#include <sys/ioctl.h>          //ioctl()
#include <unistd.h>             //close() read() write()
#include <sys/types.h>          //open()
#include <sys/stat.h>           //open()
#include <fcntl.h>              //open()
#include <stdlib.h>             //strtol()
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>

#include "VMEInterface.hh"

#define DMABUFFERSIZE   0x100000   //MBLT Buffer Dimension

using eudaq::EUDRBEvent;
using eudaq::to_string;
using eudaq::Timer;

static int decode_offset_mimotel(int x, int y) {
  if (x >= 256 || y >= 256) EUDAQ_THROW("Bad coordinates in pedestal file");
  unsigned subm = x>>6;
  if (subm == 1 || subm == 2) subm = 3-subm; // fix for submatrix ordering
  x %= 64;
  return (x+2) | (y << 9) | (subm << 18); // x+2, because 0 and 1 are dummy pixels
}

static int decode_offset_mimosa18(int x, int y) {
  if (x >= 508 || y >= 512) EUDAQ_THROW("Bad coordinates in pedestal file");
  // Arbitrary submatrix numbering - not the same as Strasbourg's numbers
  unsigned subm = x / 254 + 2*(y / 256);
  if (subm == 1 || subm == 3) subm = 4-subm; // fix for submatrix ordering
  x %= 254;
  y %= 256;
  if (subm == 3) {
    x = 253-x;
  } else if (subm == 2) {
    y = 255-y;
  } else if (subm == 1) {
    x = 253-x;
    y = 255-y;
  }
  x += 2; // for the dummy pixels
  unsigned offset = x | (y << 9) | (subm << 18);
  return offset;
}

class EUDRBProducer : public eudaq::Producer {
public:
  EUDRBProducer(const std::string & name, const std::string & runcontrol)
    : eudaq::Producer(name, runcontrol),
      done(false),
      started(false),
      juststopped(false),
      n_error(0),
      buffer(DMABUFFERSIZE),
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

    for (size_t n_eudrb=0;n_eudrb<boards.size();n_eudrb++) {

      Timer t_board;

      bool badev=false;

      Timer t_wait;
      int datasize = EventDataReady_size(boards[n_eudrb]);
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
        buffer.resize(((number_of_bytes+7) & ~7) / 4);
        boards[n_eudrb].vmed->Read(0x00400000, buffer);
        t_mblt.Stop();
        if (number_of_bytes > 16) ev.SetFlags(eudaq::Event::FLAG_HITS);

        // Reset the BUSY on the MASTER after all MBLTs are done
        t_reset.Restart();
        if (n_eudrb==boards.size()-1) { // Master
          unsigned long readdata32 = 0x2000; // for raw mode only
          if (boards[n_eudrb].zs) readdata32 |= 0x20; // ZS mode
          readdata32 |= 0x80;
          boards[n_eudrb].vmes->Write(0, readdata32);
          readdata32 &= ~0x80;
          boards[n_eudrb].vmes->Write(0, readdata32);
        }
        t_reset.Stop();

        if (/*m_ev<10 || m_ev%100==0 ||*/ badev) {
          printf("event   =0x%x, eudrb   =%3d, nbytes = %ld\n",m_ev,(int)n_eudrb,number_of_bytes);
          printf("\theader =0x%lx\n",buffer[0]);
          if ( (buffer[number_of_bytes/4-2]&0x54000000)==0x54000000) printf("\ttrailer=0x%lx\n",buffer[number_of_bytes/4-2]);
          else printf("\ttrailer=x%lx\n",buffer[number_of_bytes/4-3]);
          badev=false;
        }
      }
      Timer t_add;
      ev.AddBoard(n_eudrb,&buffer[0], number_of_bytes);
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
      m_version = param.Get("Version", -1);
      boards.clear();
      for (int n_eudrb = 0; n_eudrb < numboards; ++n_eudrb) {
        unsigned addr = param.Get("Board" + to_string(n_eudrb) + ".Addr", "Addr", 0);
        std::string mode = param.Get("Board" + to_string(n_eudrb) + ".Mode", "Mode", "");
        std::string det = param.Get("Board" + to_string(n_eudrb) + ".Det", "Det", "MIMOTEL");
        boards.push_back(BoardInfo(addr, mode, det));
      }
      bool unsync = param.Get("Unsynchronized", 0);
      std::cout << "Running in " << (unsync ? "UNSYNCHRONIZED" : "synchronized") << " mode" << std::endl;
      for (size_t n_eudrb = 0; n_eudrb < boards.size(); n_eudrb++) {
        unsigned long data = 0;
        if (boards[n_eudrb].zs) data |= 0x20; // ZS mode
        if (n_eudrb==boards.size()-1 || unsync) data |= 0x2000; // Internal Timing
        boards[n_eudrb].vmes->Write(0, 0x80000000);
        boards[n_eudrb].vmes->Write(0, data);
        unsigned long mimoconf = 0x48d00000;
        if (m_version == 1) {
          if (boards[n_eudrb].det != "MIMOTEL") EUDAQ_THROW("EUDRB Version 1 only reads MIMOTEL (not "
                                                            + boards[n_eudrb].det + ")");
          mimoconf |= (1 << 16);
        } else if (m_version == 2) {
          const int adcdelay = 0x7 & param.Get("Board" + to_string(n_eudrb) + ".AdcDelay", "AdcDelay", 2);
          const int clkselect = 0xf & param.Get("Board" + to_string(n_eudrb) + ".ClkSelect", "ClkSelect", 1);
          unsigned long reg01 = 0x00ff2000;
          unsigned long reg23 = 0x0000000a;
          unsigned long reg45 = 0x8040001f | (adcdelay << 8) | (clkselect << 12);
          unsigned long reg6  = 0;
          int pdrd =  param.Get("Board" + to_string(n_eudrb) + ".PostDetResetDelay", "PostDetResetDelay", -1);
          if (boards[n_eudrb].det == "MIMOTEL") {
            reg01 |= 65;
            if (pdrd < 0) pdrd = 4;
            reg23 |= (0xfff & pdrd) << 16;
            reg6   = 16896;
            mimoconf |= (1 << 16);
          } else if (boards[n_eudrb].det == "MIMOSA18") {
            reg01 |= 255;
            if (pdrd < 0) pdrd = 261;
            reg23 |= (0xfff & pdrd) << 16;
            reg6   = 65535;
            mimoconf |= (3 << 16);
          } else if (boards[n_eudrb].det == "MIMOSTAR2") {
            reg01 |= 65;
            if (pdrd < 0) pdrd = 4;
            reg23 |= (0xfff & pdrd) << 16;
            reg6   = 8448;
            mimoconf |= (2 << 16);
          } else if (boards[n_eudrb].det == "MIMOSA5") {
            reg01 |= 511;
            if (pdrd < 0) pdrd = 4;
            reg23 |= (0xfff & pdrd) << 16;
            reg6   = 0x3ffff;
          } else {
            EUDAQ_THROW("Unknown detector type: " + boards[n_eudrb].det);
          }
          boards[n_eudrb].vmes->Write(0x20, reg01);
          boards[n_eudrb].vmes->Write(0x24, reg23);
          boards[n_eudrb].vmes->Write(0x28, reg45);
          boards[n_eudrb].vmes->Write(0x2c, reg6);
          boards[n_eudrb].vmes->Write(0, data);
        } else {
          EUDAQ_THROW("Must set Version = 1 or 2 in config file");
        }
        data = 0xd0000001;
        if (n_eudrb==boards.size()-1 || unsync) data = 0xd0000000;
        boards[n_eudrb].vmes->Write(0x10, data);
        eudaq::mSleep(1000);
        int marker1 = param.Get("Board" + to_string(n_eudrb) + ".Marker1", "Marker1", -1);
        int marker2 = param.Get("Board" + to_string(n_eudrb) + ".Marker2", "Marker2", -1);
        if (marker1 >= 0 || marker2 >= 0) {
          if (marker1 < 0) marker1 = marker2;
          if (marker2 < 0) marker2 = marker1;
          std::cout << "Setting board " << n_eudrb << " markers to "
                    << eudaq::hexdec(marker1, 2) << ", " << eudaq::hexdec(marker2, 2) << std::endl;
          boards[n_eudrb].vmes->Write(0x10, 0x48110800 | (marker1 & 0xff));
          eudaq::mSleep(1000);
          boards[n_eudrb].vmes->Write(0x10, 0x48110700 | (marker2 & 0xff));
          eudaq::mSleep(1000);
        }
        boards[n_eudrb].vmes->Write(0x10, mimoconf);
        eudaq::mSleep(1000);
      }
      std::cout << "Waiting for boards to reset..." << std::endl;

      for (int i = 0; i < 150; ++i) {
        eudaq::mSleep(100);
        bool ready = true;
        for (size_t n_eudrb = 0; n_eudrb < boards.size(); n_eudrb++) {
          unsigned long data = 0;
          data = boards[n_eudrb].vmes->Read(0);
          if (!(data & 0x02000000)) {
            ready = false;
          }
        }
        if (ready) break;
      }
      std::cout << "OK" << std::endl;
      // new loop added by Angelo
      for (size_t n_eudrb = 0; n_eudrb < boards.size(); n_eudrb++) {
        unsigned long data = 0x40;
        if (boards[n_eudrb].zs) data |= 0x20;
        if (n_eudrb==boards.size()-1) data |= 0x2000;
        boards[n_eudrb].vmes->Write(0, data);
        data=0;
        if (boards[n_eudrb].zs) data |= 0x20;
        if (n_eudrb==boards.size()-1) data |= 0x2000;
        boards[n_eudrb].vmes->Write(0, data);
      }

      for (size_t n_eudrb = 0; n_eudrb < boards.size(); n_eudrb++) {
        // check for Zero Suppression
        if (boards[n_eudrb].zs) {
          int thresh = param.Get("Board" + to_string(n_eudrb) + ".Thresh", "Thresh", 10);
          int ped = param.Get("Board" + to_string(n_eudrb) + ".Ped", "Ped", 0);
          float mult = param.Get("Board" + to_string(n_eudrb) + ".Mult", "Mult", 2.0);
          std::string fname = param.Get("Board" + to_string(n_eudrb) + ".PedestalFile", "PedestalFile", "");
          if (thresh < 0 && fname == "") {
            fname = "ped%.dat";
            EUDAQ_WARN("Config missing pedestal file name, using " + fname);
          }
          if (fname != "" &&
              fname.find_first_not_of("0123456789") == std::string::npos) {
            std::string padding(6-fname.length(), '0');
            fname = "run" + padding + fname + "-ped-db-b%.dat";
          }
          size_t n = fname.find('%');
          if (n != std::string::npos) {
            fname = std::string(fname, 0, n) + to_string(n_eudrb) + std::string(fname, n+1);
          }
          if (fname != "") fname = "../pedestal/" + fname;
          pedestal_t peds = (fname == "") ? GeneratePedestals(thresh, ped)
            : ReadPedestals(fname, mult, boards[n_eudrb].det);
          LoadPedestals(boards[n_eudrb], peds);

          std::string msg = "Board " + to_string(n_eudrb) + " pedestals loading";
          if (fname != "") msg += " from file " + fname + " * " + to_string(mult);
          else msg += " values = " + to_string(ped) + ", " + to_string(thresh);
          puts(msg.c_str());
          EUDAQ_INFO(msg);
        }
      }
      // END of loading pedestal loop

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
      std::string det = boards[0].det;
      for (size_t i = 1; i < boards.size(); ++i) {
        if (boards[i].det != det) det = "Mixed";
      }
      ev.SetTag("VERSION", to_string(m_version));
      ev.SetTag("DET", det);
      for (size_t i = 0; i < boards.size(); ++i) {
        ev.SetTag("DET" + to_string(i), boards[i].det);
      }
      std::string mode = boards[0].mode;
      for (size_t i = 1; i < boards.size(); ++i) {
        if (boards[i].mode != mode) mode = "Mixed";
      }
      ev.SetTag("MODE", mode);
      for (size_t i = 0; i < boards.size(); ++i) {
        ev.SetTag("MODE" + to_string(i), boards[i].mode);
      }
      ev.SetTag("BOARDS", to_string(boards.size()));
      //ev.SetTag("ROWS", to_string(256))
      //ev.SetTag("COLS", to_string(66))
      //ev.SetTag("MATS", to_string(4))
      for (size_t i = 0; i < boards.size(); ++i) {
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
      for (size_t n_eudrb = 0; n_eudrb < boards.size(); n_eudrb++) {
        unsigned long readdata32 = boards[n_eudrb].vmes->Read(0);
        readdata32 |= 0x80000000;
        boards[n_eudrb].vmes->Write(0, readdata32);
        readdata32 = boards[n_eudrb].vmes->Read(0);
        readdata32&=~(0x80000000);
        boards[n_eudrb].vmes->Write(0, readdata32);
      }
      sleep(8);
      SetStatus(eudaq::Status::LVL_OK, "Reset");
    } catch (const std::exception & e) {
      printf("Caught exception: %s\n", e.what());
      SetStatus(eudaq::Status::LVL_ERROR, "Reset Error");
    } catch (...) {
      printf("Unknown exception\n");
      SetStatus(eudaq::Status::LVL_ERROR, "Reset Error");
    }
  }
  virtual void OnStatus() {
    //std::cout << "Status " << m_status << std::endl;
  }
  virtual void OnUnrecognised(const std::string & cmd, const std::string & param) {
    std::cout << "Unrecognised: (" << cmd.length() << ") " << cmd;
    if (param.length() > 0) std::cout << " (" << param << ")";
    std::cout << std::endl;
    SetStatus(eudaq::Status::LVL_WARN, "Unrecognised command");
  }

  typedef  std::vector<unsigned> pedestal_t;
  pedestal_t GeneratePedestals(int thresh = 10, int ped = 0) const {
    return pedestal_t(1UL<<20, thresh | (ped << 6));
  }
  pedestal_t ReadPedestals(const std::string & filename, float sigma, const std::string & sensor) const {
    static const unsigned badpix = 0x1f + (1<<11);
    pedestal_t result(1UL<<20, badpix);
    //printf("Fill Matrices on board: %lx\n",address);
    int (*decode_offset)(int,int) = 0;
    if (sensor == "MIMOTEL") {
      decode_offset = decode_offset_mimotel;
    } else if (sensor == "MIMOSA18") {
      decode_offset = decode_offset_mimosa18;
    } else {
      EUDAQ_THROW("I don't know how to decode " + sensor + " pedestal files");
    }
    std::ifstream file(filename.c_str());
    if (file.is_open()) {
      //printf("Opened file %s\n",filename.c_str());
      std::string line;
      while (std::getline(file, line)) {
        line = eudaq::trim(line);
        if (line == "") {
          continue;
        } else if (line[0] == '#') {
          std::cout << line << std::endl;
          continue;
        }
        int board,x,y,flag=0;
        float ped, thresh;
        if (sscanf(line.c_str(),"%d %d %d %f %f %d\n",&board,&x,&y,&ped,&thresh,&flag) < 5) {
          EUDAQ_THROW("Error in pedestal file");
        }
        unsigned long offset = decode_offset(x, y);
        int thresh2bit = (int) (thresh*sigma);
        int ped2bit = (int)ped & 0x1f;
        if (thresh2bit > 31) thresh2bit = 31;
        if (thresh2bit < -32) thresh2bit = -32;
        if (ped2bit > 31) ped2bit = 31;
        if (ped2bit < -32) ped2bit = -32;
        unsigned long newdata32 = (thresh2bit & 0x3f) | ((ped2bit & 0x3f) << 6);

        if (flag) newdata32 = badpix; // mask bad pixels as good as you can (high thresh and very low ped)

        result[offset] = newdata32;
      }
    } else {
      EUDAQ_THROW("Unable top open pedestal file " + filename);
    }
    //printf("\tdone!\n");
    return result;
  }

  struct BoardInfo {
    BoardInfo(unsigned long addr, const std::string & mode, const std::string & det)
      : vmes(VMEFactory::Create(addr, 0x1000000)),
        vmed(VMEFactory::Create(addr, 0x1000000, VMEInterface::A32, VMEInterface::D64, VMEInterface::PMBLT)),
        mode(mode), det(det), zs(mode == "ZS") {}
    VMEptr vmes, vmed;
    //unsigned long BaseAddress;
    std::string mode, det;
    bool zs;
  };

  unsigned long EventDataReady_size(const BoardInfo & brd) {
    unsigned long int i = 0, readdata32 = 0; //, olddata = 0;
    for (eudaq::Timer timer; timer.Seconds() < 1.0; /**/) {
      ++i;
      readdata32 = brd.vmes->Read(0x00400004);
      if (readdata32 & 0x80000000) {
        return readdata32 & 0xfffff;
      }
      if (timer.Seconds() > 0.5) {
        eudaq::mSleep(20);
      }
//       if (readdata32 != olddata) {
//         printf("W %ld, r=%ld (%08lx)\n", i, readdata32, readdata32);
//         olddata = readdata32;
//       }
    }
    printf("Ready wait timed out after 1 second (%ld cycles)\n",i);
    return 0;
  }

  void LoadPedestals(const BoardInfo & board, const pedestal_t & peds) {
    unsigned long readdata32, newdata32;
    // here we put in the uploading of pedestals:
    // VME is master of SRAM
    printf("Become Master of SRAM\n");
    readdata32 = board.vmes->Read(0);
    newdata32=readdata32|0x200;
    board.vmes->Write(0, newdata32);
    for (size_t i = 0; i < peds.size(); ++i) {
      board.vmes->Write(0x800000 + i*4, peds[i]);
    }
    // Release master of SRAM
    printf("\tRelease Master of SRAM\n");
    readdata32 = board.vmes->Read(0);
    newdata32=readdata32 & ~0x200;
    board.vmes->Write(0, newdata32);
    // reset once more to be sure
    /* read address first and only set the reset bit */
    readdata32 = board.vmes->Read(0);
    newdata32=readdata32 | 0x40;
    board.vmes->Write(0, newdata32);
    board.vmes->Write(0, readdata32);
    //EUDAQ_INFO("Board " + to_string(n_eudrb) + " done!");
  }
  unsigned m_run, m_ev;
  bool done, started, juststopped;
  int n_error;
  std::vector<unsigned long> buffer;
  std::vector<BoardInfo> boards;
  //int fdOut;
  int m_idoffset, m_version;
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
