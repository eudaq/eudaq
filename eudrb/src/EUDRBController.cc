#include "EUDRBController.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Timer.hh"

#include <iostream>
#include <fstream>

namespace eudaq {

  namespace {

    static const char * MODE_NAMES[] = { "NONE", "RAW3", "RAW2", "ZS" };
    static const int NUM_MODES = sizeof MODE_NAMES / sizeof * MODE_NAMES;
    static const char * DET_NAMES[] = { "NONE", "MIMOSA5", "MIMOSTAR2", "MIMOTEL", "MIMOSA18", "MIMOSA26" };
    static const int NUM_DETS = sizeof DET_NAMES / sizeof * DET_NAMES;

    template <typename T>
    inline T getpar(const eudaq::Configuration & param, int id, const std::string & name, const T & def) {
      return param.Get("Board" + to_string(id) + "." + name, name, def);
    }
    inline std::string getpar(const eudaq::Configuration & param, int id, const std::string & name, const char * def) {
      return param.Get("Board" + to_string(id) + "." + name, name, std::string(def));
    }

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

  }

  EUDRBController::EUDRBController(int id, int slot, int version)
    : m_id(id),
      m_version(version),
      m_mode(M_NONE),
      m_det(D_NONE)
  {
    m_vmes = VMEFactory::Create(slot << 27, 0x1000000);
    m_vmed = VMEFactory::Create(slot << 27, 0x1000000, VMEInterface::A32, VMEInterface::D32,
                                version >= 3 ? VMEInterface::P2eSST : VMEInterface::PMBLT,
                                version >= 3 ? VMEInterface::SST160 : VMEInterface::SSTNONE);
  }

  std::string EUDRBController::Mode() const {
    return MODE_NAMES[m_mode];
  }

  std::string EUDRBController::Det() const {
    return DET_NAMES[m_det];
  }

  void EUDRBController::Configure(const eudaq::Configuration & param, int master) {
    std::string mode = getpar(param, m_id, "Mode", "");
    m_mode = ModeNum(mode);
    std::string det = getpar(param, m_id, "Det", "");
    m_det = DetNum(det);
    bool unsync = getpar(param, m_id, "Unsynchronized", true);
    bool internaltiming = (m_id == master) || unsync;
    ResetBoard();
    unsigned long data = 0;
    if (m_mode == M_ZS) data |= 0x20;
    if (internaltiming) data |= 0x2000;
    m_vmes->Write(0, data);
    unsigned long mimoconf = 0x48d00000;
    if (m_version == 1) {
      if (m_det != D_MIMOTEL) {
        EUDAQ_THROW("EUDRB Version 1 only reads MIMOTEL (not " + det + ")");
      }
      mimoconf |= (1 << 16);
    } else if (m_version == 2) {
      const int adcdelay = 0x7 & getpar(param, m_id, "AdcDelay", 2);
      const int clkselect = 0xf & getpar(param, m_id, "ClkSelect", 1);
      unsigned long reg01 = 0x00ff2000;
      unsigned long reg23 = 0x0000000a;
      unsigned long reg45 = 0x8040001f | (adcdelay << 8) | (clkselect << 12);
      unsigned long reg6  = 0;
      int pdrd = getpar(param, m_id, "PostDetResetDelay", -1);
      if (m_det == D_MIMOTEL) {
        reg01 |= 65;
        if (pdrd < 0) pdrd = 4;
        reg23 |= (0xfff & pdrd) << 16;
        reg6   = 16896;
        mimoconf |= (1 << 16);
      } else if (m_det == D_MIMOSA18) {
        reg01 |= 255;
        if (pdrd < 0) pdrd = 261;
        reg23 |= (0xfff & pdrd) << 16;
        reg6   = 65535;
        mimoconf |= (3 << 16);
      } else if (m_det == D_MIMOSTAR2) {
        reg01 |= 65;
        if (pdrd < 0) pdrd = 4;
        reg23 |= (0xfff & pdrd) << 16;
        reg6   = 8448;
        mimoconf |= (2 << 16);
      } else if (m_det == D_MIMOSA5) {
        reg01 |= 511;
        if (pdrd < 0) pdrd = 4;
        reg23 |= (0xfff & pdrd) << 16;
        reg6   = 0x3ffff;
      } else {
        EUDAQ_THROW("Unknown detector type: " + to_string(m_det));
      }
      m_vmes->Write(0x20, reg01);
      m_vmes->Write(0x24, reg23);
      m_vmes->Write(0x28, reg45);
      m_vmes->Write(0x2c, reg6);
      m_vmes->Write(0, data);
    } else {
      EUDAQ_THROW("Must set Version = 1 or 2 in config file");
    }
    data = 0xd0000001;
    if (internaltiming) data = 0xd0000000;
    m_vmes->Write(0x10, data);
    eudaq::mSleep(1000);
    int marker1 = getpar(param, m_id, "Marker1", -1);
    int marker2 = getpar(param, m_id, "Marker2", -1);
    if (marker1 >= 0 || marker2 >= 0) {
      if (marker1 < 0) marker1 = marker2;
      if (marker2 < 0) marker2 = marker1;
      std::cout << "Setting board " << m_id << " markers to "
                << eudaq::hexdec(marker1, 2) << ", " << eudaq::hexdec(marker2, 2) << std::endl;
      m_vmes->Write(0x10, 0x48110800 | (marker1 & 0xff));
      eudaq::mSleep(1000);
      m_vmes->Write(0x10, 0x48110700 | (marker2 & 0xff));
      eudaq::mSleep(1000);
    }
    m_vmes->Write(0x10, mimoconf);
  }

  void EUDRBController::ConfigurePedestals(const eudaq::Configuration & param) {
    if (m_mode == M_ZS) {
      int thresh = getpar(param, m_id, "Thresh", 10);
      int ped = getpar(param, m_id, "Ped", 0);
      float mult = getpar(param, m_id, "Mult", 2.0);
      std::string fname = getpar(param, m_id, "PedestalFile", "");
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
        fname = std::string(fname, 0, n) + to_string(m_id) + std::string(fname, n+1);
      }
      if (fname != "") fname = "../pedestal/" + fname;
      pedestal_t peds = (fname == "") ? GeneratePedestals(thresh, ped)
        : ReadPedestals(fname, mult, m_det);
      LoadPedestals(peds);

      std::string msg = "Board " + to_string(m_id) + " pedestals loading";
      if (fname != "") msg += " from file " + fname + " * " + to_string(mult);
      else msg += " values = " + to_string(ped) + ", " + to_string(thresh);
      puts(msg.c_str());
      EUDAQ_INFO(msg);
    }
  }

  void EUDRBController::ResetBoard() {
    unsigned long readdata32 = m_vmes->Read(0);
    m_vmes->Write(0, readdata32 | 0x80000000);
    m_vmes->Write(0, readdata32 &~0x80000000);
  }

  bool EUDRBController::WaitForReady(double timeout) {
    for (eudaq::Timer timer; timer.Seconds() < timeout; /**/) {
      eudaq::mSleep(20);
      if (m_vmes->Read(0) & 0x02000000) {
        return true;
      }
    }
    return false;
  }

  void EUDRBController::ResetTriggerProc() {
    unsigned long readdata32 = m_vmes->Read(0) & ~0x40;
    m_vmes->Write(0, readdata32 | 0x40);
    m_vmes->Write(0, readdata32);
  }

  unsigned long EUDRBController::EventDataReady_size(double timeout) {
    unsigned long int i = 0, readdata32 = 0; //, olddata = 0;
    for (eudaq::Timer timer; timer.Seconds() < timeout; /**/) {
      ++i;
      readdata32 = m_vmes->Read(0x00400004);
      if (readdata32 & 0x80000000) {
        return readdata32 & 0xfffff;
      }
      if (timer.Seconds() > 0.5) {
        eudaq::mSleep(20);
      }
    }
    printf("Ready wait timed out after 1 second (%ld cycles)\n",i);
    return 0;
  }

  void EUDRBController::ReadEvent(std::vector<unsigned long> & buf) {
    m_vmed->Read(0x00400000, buf);
  }

  void EUDRBController::ResetTriggerBusy() {
    unsigned long readdata32 = 0x2000; // for raw mode only
    if (m_mode == M_ZS) readdata32 |= 0x20; // ZS mode
    m_vmes->Write(0, readdata32 | 0x80);
    m_vmes->Write(0, readdata32);
  }

  int EUDRBController::ModeNum(const std::string & mode) {
    for (int i = 0; i < NUM_MODES; ++i) {
      if (eudaq::lcase(mode) == eudaq::lcase(MODE_NAMES[i])) return i;
    }
    return M_NONE;
  }

  int EUDRBController::DetNum(const std::string & det) {
    for (int i = 0; i < NUM_DETS; ++i) {
      if (eudaq::lcase(det) == eudaq::lcase(DET_NAMES[i])) return i;
    }
    return D_NONE;
  }

  void EUDRBController::LoadPedestals(const pedestal_t & peds) {
    printf("Become Master of SRAM\n");
    unsigned long readdata32 = m_vmes->Read(0) & ~0x200;
    m_vmes->Write(0, readdata32 | 0x200);
    for (size_t i = 0; i < peds.size(); ++i) {
      m_vmes->Write(0x800000 + i*4, peds[i]);
    }
    printf("\tRelease Master of SRAM\n");
    m_vmes->Write(0, readdata32);
    // reset once more to be sure
    ResetTriggerProc();
  }

  EUDRBController::pedestal_t EUDRBController::GeneratePedestals(int thresh, int ped) {
    return pedestal_t(1UL<<20, thresh | (ped << 6));
  }

  EUDRBController::pedestal_t EUDRBController::ReadPedestals(const std::string & filename, float sigma, int det) {
    static const unsigned badpix = 0x1f + (1<<11);
    pedestal_t result(1UL<<20, badpix);
    //printf("Fill Matrices on board: %lx\n",address);
    int (*decode_offset)(int,int) = 0;
    if (det == D_MIMOTEL) {
      decode_offset = decode_offset_mimotel;
    } else if (det == D_MIMOSA18) {
      decode_offset = decode_offset_mimosa18;
    } else {
      EUDAQ_THROW("I don't know how to decode '" + to_string(det) + "' pedestal files");
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

}
