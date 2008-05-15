#include "tlu/TLUController.hh"
#include "tlu/USBTracer.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Time.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Exception.hh"
#include "eudaq/counted_ptr.hh"
#include <unistd.h>
#include <string>
#include <fstream>
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
  eudaq::Option<int>         emode(op, "e", "error-handler", 2, "value",
                                   "Error handler (0=abort, >0=number of tries before exception)");
  eudaq::Option<std::string> sname(op, "s", "save-file", "", "filename",
                                   "The filename to save trigger numbers and timestamps");
  eudaq::Option<std::string> trace(op, "z", "trace-file", "", "filename",
                                   "The filename to save a trace of all usb accesses,\n"
				   "prepend - for only errors, or + for all data (including block transfers)");
  try {
    op.Parse(argv);
    std::cout << "Using options:\n"
              << "Bit file name = " << fname.Value() << "\n"
              << "Trigger interval = " << trigg.Value() << "\n"
              << "DUT Mask = " << dmask.Value() << "\n"
              << "Veto Mask = " << vmask.Value() << "\n"
              << "And Mask = " << amask.Value() << "\n"
              << "Or Mask = " << omask.Value() << "\n"
              << "Save file = " << (sname.Value() == "" ? std::string("(none)") : sname.Value()) << "\n"
              << std::endl;
    counted_ptr<std::ofstream> sfile;
    if (sname.Value() != "") {
      sfile = new std::ofstream(sname.Value().c_str());
      if (!sfile->is_open()) EUDAQ_THROW("Unable to open file: " + sname.Value());
    }
    signal(SIGINT, ctrlchandler);
    if (trace.Value() != "") {
      std::string fname = trace.Value();
      if (fname[0] == '-') {
	tlu::setusbtracelevel(1);
	fname = std::string(fname, 1);
      } else if (fname[0] == '+') {
	tlu::setusbtracelevel(3);
	fname = std::string(fname, 1);
      } else {
	tlu::setusbtracelevel(2);
      }	
      tlu::setusbtracefile(fname);
    }
    TLUController TLU(fname.Value(), emode.Value());
    //TLU.FullReset();
    TLU.SetTriggerInterval(trigg.Value());
    TLU.SetDUTMask(dmask.Value());
    TLU.SetVetoMask(vmask.Value());
    TLU.SetAndMask(amask.Value());
    TLU.SetOrMask(omask.Value());
    std::cout << "TLU Firmware version: " << TLU.GetFirmwareID()
              << " (library " << TLU.GetLibraryID() << ")" << std::endl;

    eudaq::Time starttime(eudaq::Time::Current());
    TLU.Start();
    std::cout << "TLU Started!" << std::endl;

    unsigned long total = 0;
    while (!g_done) {
      TLU.Update();
      std::cout << std::endl;
      TLU.Print();
      if (sfile.get()) {
        for (size_t i = 0; i < TLU.NumEntries(); ++i) {
          *sfile << TLU.GetEntry(i).Eventnum() << "\t" << TLU.GetEntry(i).Timestamp() << std::endl;
        }
      }
      total += TLU.NumEntries();
      eudaq::Time elapsedtime(eudaq::Time::Current() - starttime);
      double hertz = total / elapsedtime.Seconds();
      std::cout << "Time: " << elapsedtime.Formatted("%s.%3") << " s, Hertz: " << hertz << std::endl;
      usleep(50000);
      //TLU.ResetUSB(); // DEBUG
    }
    std::cout << "Quitting..." << std::endl;
    TLU.Stop();
    TLU.Update();
    //TLU.Print();
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
