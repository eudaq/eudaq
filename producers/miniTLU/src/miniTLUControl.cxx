#include "tlu/miniTLUController.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Exception.hh"
#include <memory>


#ifndef WIN32
#include <unistd.h>
#endif

#include <string>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <csignal>

using namespace tlu;
using eudaq::to_string;
using eudaq::hexdec;

static sig_atomic_t g_done = 0;
#ifdef WIN32
static const std::string TIME_FORMAT="%M:%S.%3";
#else
static const std::string TIME_FORMAT="%s.%3";
#endif
void ctrlchandler(int) {
  g_done += 1;
}

int main(int /*argc*/, char ** argv) {
  float vthresh_values[TLU_PMTS];

  eudaq::OptionParser op("TLU Control Utility", "1.1", "A comand-line tool for controlling the AIDA mini-TLU Trigger Logic Unit");

  eudaq::Option<std::string> ConnectionFile(op, "cf", "connectionFile", "file://connection.xml", "filename",
                                   "IPBus/uHAL connection file describing network connection to TLU");
  eudaq::Option<std::string> DeviceName(op, "dn", "deviceName", "minitlu", "filename",
                                   "IPBus/uHAL connection file describing network connection to TLU");
  eudaq::Option<int>         trigg(op, "t", "trigger", 0, "msecs",
                                   "The interval in milliseconds for internally generated triggers (0 = off)");
  eudaq::Option<int>         hsmode(op, "hm", "handshakemode", 0x3F, "nohandshake",
                                   "Bit mask for handshake mode. Set to zero and in this mode the TLU issues a fixed-length pulse on the trigger line (0 = no hand shake)");
  eudaq::Option<int>         dmask(op, "d", "dutmask", 0, "mask",
                                   "The mask for enabling the DUT connections");
  eudaq::Option<int>         vmask(op, "v", "vetomask", 0, "mask",
                                   "The mask for vetoing external triggers");
  eudaq::Option<int>         amask(op, "a", "andmask", 255, "mask",
                                   "The mask for coincidence of external triggers");
  eudaq::Option<int>         omask(op, "o", "ormask", 0, "mask",
                                   "The mask for ORing of external triggers");

  eudaq::Option<int>         wait(op, "w", "wait", 1000, "ms",
                                  "Time to wait between updates in milliseconds");
  eudaq::Option<int>         strobeperiod(op, "p", "strobeperiod", 1000, "cycles",
                                  "Period for timing strobe in clock cycles");
  eudaq::Option<int>         strobelength(op, "l", "strobelength", 100, "cycles",
                                  "Length of 'on' time for timing strobe in clock cycles");
  eudaq::Option<int>         enabledutveto(op, "b", "dutveto", 0, "mask",
                                  "Mask for enabling veto of triggers ('backpressure') by rasing DUT_CLK");

  // I2C options.....
  eudaq::Option<int>        dacAddr(op, "da" ,"dacAddr" , 0x1F , "I2C address" , "I2C address of DAC controlling discriminator thresholds");
  eudaq::Option<int>        identAddr(op, "id" ,"idAddr" , 0x50 , "I2C address" , "I2C address of EEPROM containing unique identifier");

  // Options to control the threshold voltage for the discriminators
  eudaq::Option<float>         vthresh(op, "thr", "vthresh", -1, "Volts", "All thresholds");
  eudaq::Option<float>         vthresh_1(op, "thr1", "vthresh1", -1, "Volts", "Threshold 1  (will override \"thr\" value if set to 0 or greater)");
  eudaq::Option<float>         vthresh_2(op, "thr2", "vthresh2", -1, "Volts", "Threshold 2  (will override \"thr\" value if set to 0 or greater)");
  eudaq::Option<float>         vthresh_3(op, "thr3", "vthresh3", -1, "Volts", "Threshold 3  (will override \"thr\" value if set to 0 or greater)");
  eudaq::Option<float>         vthresh_4(op, "thr4", "vthresh4", -1, "Volts", "Threshold 4  (will override \"thr\" value if set to 0 or greater)");

  eudaq::OptionFlag          nots(op, "n", "notimestamp", "Do not read out timestamp buffer");
  eudaq::OptionFlag          quit(op, "q", "quit", "Quit after configuring TLU");
  eudaq::OptionFlag          pause(op, "u", "wait-for-user", "Wait for user input before starting triggers");
  eudaq::Option<std::string> sname(op, "s", "save-file", "", "filename",
                                   "The filename to save trigger numbers and timestamps");
  eudaq::OptionFlag          ipbusDebug(op, "z", "ipbusDebug", "Turn on debugging of IPBus transfers");

  try {
    op.Parse(argv);


    // read the discriminator threshold values into an array
    for(int i = 0; i < TLU_PMTS; i++)
    {
	 vthresh_values[i] = vthresh.Value() == -1 ? 0 : vthresh.Value();
    }

    if(vthresh_1.Value() != -1) { vthresh_values[0] = vthresh_1.Value();  }
    if(vthresh_2.Value() != -1) { vthresh_values[1] = vthresh_2.Value();  }
    if(vthresh_3.Value() != -1) { vthresh_values[2] = vthresh_3.Value();  }
    if(vthresh_4.Value() != -1) { vthresh_values[3] = vthresh_4.Value();  }

    // print out the command line options
    std::cout << "Using options:\n"

              << "TLU Connection file = " << ConnectionFile.Value() << "\n"
              << "TLU Device name = '" << DeviceName.Value() << "'" << "\n"

              << "Hand shake mode = " << hsmode.Value() << "\n"
              << "Trigger interval = " << trigg.Value() << "\n"
              << (trigg.Value() > 0 ? " ms (" + to_string(1e3/trigg.Value()) + " Hz)" : std::string()) << "\n"
              << "DUT Mask  = " << hexdec(dmask.Value(), 2) << "\n"
              << "Veto Mask = " << hexdec(vmask.Value(), 2) << "\n"
              << "And Mask  = " << hexdec(amask.Value(), 2) << "\n"
              << "Or Mask   = " << hexdec(omask.Value(), 2) << "\n"
              << "Strobe period = " << hexdec(strobeperiod.Value(), 6) << "\n"
              << "Strobe length = " << hexdec(strobelength.Value(), 6) << "\n"
              << "Enable DUT Veto = " << hexdec(enabledutveto.Value(), 2) << "\n"

              << "Threshold voltage  1  = " << vthresh_values[0] << " V" << std::endl
              << "Threshold voltage  2  = " << vthresh_values[1] << " V" << std::endl
              << "Threshold voltage  3  = " << vthresh_values[2] << " V" << std::endl
              << "Threshold voltage  4  = " << vthresh_values[3] << " V" << std::endl

              << "Save file = '" << sname.Value() << "'" << (sname.Value() == "" ? " (none)" : "") << "\n"
              << std::endl;

    std::shared_ptr<std::ofstream> sfile;
    if (sname.Value() != "") {
      sfile = std::make_shared<std::ofstream>(sname.Value().c_str());
      if (!sfile->is_open()) EUDAQ_THROW("Unable to open file: " + sname.Value());
    }
    signal(SIGINT, ctrlchandler);

    //TODO
    //    if (ipbusDebug.IsSet) {
    //}

    // Instantiate an instance of a miniTLU
    miniTLUController TLU(ConnectionFile.Value() , DeviceName.Value() );

    /*
    //TLU.FullReset();
    TLU.SetHandShakeMode(hsmode.Value()); //$$ change
    TLU.SetTriggerInterval(trigg.Value());

    TLU.SetDUTMask(dmask.Value());
    TLU.SetVetoMask(vmask.Value());
    TLU.SetAndMask(amask.Value());
    TLU.SetOrMask(omask.Value());
    TLU.SetStrobe(strobeperiod.Value() , strobelength.Value());
    TLU.SetEnableDUTVeto(enabledutveto.Value());
    */
    
    TLU.SetInternalTriggerInterval(trigg.Value());

    TLU.InitializeI2C(dacAddr.Value(),identAddr.Value());
    TLU.SetThresholdValue(0, vthresh_values[0]);
    TLU.SetThresholdValue(1, vthresh_values[1]);
    TLU.SetThresholdValue(2, vthresh_values[2]);
    TLU.SetThresholdValue(3, vthresh_values[3]);
  
    //TLU.ResetTimestamp(); // also sets strobe running (if enabled) 

    std::cout << "TLU Serial number = " << eudaq::hexdec(TLU.GetBoardID(), 12) << "\n"
              << "Firmware version = " << TLU.GetFirmwareVersion() << "\n"
              << std::endl;

    //if (TLU.GetFirmwareID() != TLU.GetLibraryID()) {
    //  std::cout << "Warning: *** Firmware version does not match Library version ***\n" << std::endl;
    //}

    if (quit.IsSet()) return 0;
    eudaq::Timer totaltime, lasttime;

    if (pause.IsSet()) {
      std::cerr << "Press enter to start triggers." << std::endl;
      std::getchar();
    }

    /*
      // fill this bit in.....
    TLU.Start();
    std::cout << "TLU Started!" << std::endl;

    unsigned long total = 0;
    while (!g_done) {
      TLU.Update(!nots.IsSet());
      std::cout << std::endl;
      TLU.Print(!nots.IsSet());
      if (sfile.get()) {
        for (size_t i = 0; i < TLU.NumEntries(); ++i) {
          *sfile << TLU.GetEntry(i).Eventnum() << "\t" << TLU.GetEntry(i).Timestamp() << std::endl;
        }
      }
      total += TLU.NumEntries();
      double hertz = TLU.NumEntries() / lasttime.Seconds();
      double avghertz = total / totaltime.Seconds();
      lasttime.Restart();
      std::cout << "Time: " << totaltime.Formatted(TIME_FORMAT) << " s, "
                << "Freq: " << hertz << " Hz, "
                << "Average: " << avghertz << " Hz" << std::endl;
      if (wait.Value() > 0) {
        eudaq::mSleep(wait.Value());
      }
    }
    std::cout << "Quitting..." << std::endl;
    TLU.Stop();
    TLU.Update();
    //TLU.Print();
    */

  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}

