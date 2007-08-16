#include "tlu/TLUController.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Time.hh"
#include "eudaq/Utils.hh"
#include <unistd.h>
#include <sstream>
#include <csignal>

using namespace tlu;

static sig_atomic_t g_done = 0;

void ctrlchandler(int) {
  g_done = 1;
}

int main(int /*argc*/, char ** argv) {
  eudaq::OptionParser op("TLU Control Utility", "1.0", "A comand-line tool for controlling the Trigger Logic Unit");
  eudaq::Option<std::string> fname(op, "f", "bitfile", "TLU_Toplevel.bit", "filename",
                                   "The bitfile containing the TLU firmware to be loaded");
  eudaq::Option<int>         trigg(op, "t", "trigger", 0, "msecs",
                                   "The interval in milliseconds for internally generated triggers (0 = off)");
  eudaq::Option<int>         dmask(op, "d", "dutmask", 2, "mask",
                                   "The mask for enabling the DUT connections");
  eudaq::Option<int>         vmask(op, "v", "vetomask", 0, "mask",
                                   "The mask for vetoing external triggers");
  eudaq::Option<int>         amask(op, "a", "andmask", 255, "mask",
                                   "The mask for coincidence of external triggers");
  eudaq::Option<int>         omask(op, "o", "ormask", 0, "mask",
                                   "The mask for ORing of external triggers");
  try {
    op.Parse(argv);
    std::cout << "Using options:\n"
              << "Bit file name = " << fname.Value() << "\n"
              << "Trigger interval = " << trigg.Value() << "\n"
              << "DUT Mask = " << dmask.Value() << "\n"
              << "Veto Mask = " << vmask.Value() << "\n"
              << "And Mask = " << amask.Value() << "\n"
              << "Or Mask = " << omask.Value() << "\n"
              << std::endl;
    signal(SIGINT, ctrlchandler);
    TLUController TLU(fname.Value());
    TLU.SetTriggerInterval(trigg.Value());
    TLU.SetDUTMask(dmask.Value());
    TLU.SetVetoMask(vmask.Value());
    TLU.SetAndMask(amask.Value());
    TLU.SetOrMask(omask.Value());
    std::cout << "TLU Firmware version:" << TLU.GetFirmwareID() << std::endl;
    //sleep(1);

    eudaq::Time starttime(eudaq::Time::Current());
    TLU.Start();
    std::cout << "TLU Started!" << std::endl;
    unsigned long long lasttime = 0;
    unsigned long total = 0;
    while (!g_done) {
      TLU.Update();
      //std::cout << "hello " << TLU.NumEntries() << std::endl;
      std::cout << (TLU.NumEntries() ? "\n" : ".") << std::flush;
      size_t i=0;
      for (i = 0; i < TLU.NumEntries(); ++i) {
        //std::cout << "test " << 1 << std::endl;
        unsigned long long t = TLU.GetEntry(i).Timestamp();
        //std::cout << "test " << 2 << std::endl;
        long long d = t-lasttime;
        std::cout << "  " << TLU.GetEntry(i)
                  << ", diff=" << d << (d <= 0 ? "  ***" : "")
                  << std::endl;
        lasttime = t;
      }
      total+=i;
      eudaq::Time elapsedtime(eudaq::Time::Current() - starttime);
      double hertz = total / elapsedtime.Seconds();
      std::cout << "Time: " << elapsedtime.Formatted("%s.%3") << " s, Hertz: " << hertz << std::endl;
      //usleep(100000);
      sleep(1);
    }
    printf("Quitting...\n");
    TLU.Stop();
    //sleep(1);
    TLU.Update();
    //TLU.Print();
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
