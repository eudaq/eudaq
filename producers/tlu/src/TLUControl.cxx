#include "tlu/TLUController.hh"
#include "tlu/USBTracer.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Exception.hh"

#ifndef WIN32
#include <unistd.h>
#endif

#include <string>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <csignal>
#include <memory>

using namespace tlu;
using eudaq::to_string;
using eudaq::hexdec;

static sig_atomic_t g_done = 0;
#ifdef WIN32
static const std::string TIME_FORMAT="%M:%S.%3";
#else
static const std::string TIME_FORMAT="%s.%3";
#endif
#ifdef WIN32
ZESTSC1_ERROR_FUNC ZestSC1_ErrorHandler=NULL;  // set to NULL so that this function will not be called. it seems that this is only requiered on WINDOWS
char *ZestSC1_ErrorStrings[]={"dummy","dummy1"}; // needs to have some dummy strings but for now i dont know where they will be used again. 
#endif
void ctrlchandler(int) {
  g_done += 1;
}

int main(int /*argc*/, char ** argv) {
  unsigned pmtvcntl_values[TLU_PMTS];

  eudaq::OptionParser op("TLU Control Utility", "1.1", "A comand-line tool for controlling the Trigger Logic Unit");
  eudaq::Option<std::string> fname(op, "f", "bitfile", "", "filename",
                                   "The bitfile containing the TLU firmware to be loaded");
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
  eudaq::Option<std::vector<std::string> > ipsel(op, "i", "dutinputs", "value", ",",
                                   "Selects the DUT inputs (values: RJ45,LEMO,HDMI,NONE)");
  eudaq::Option<int>         emode(op, "e", "error-handler", 2, "value",
                                   "Error handler (0=abort, >0=number of tries before exception)");
  eudaq::Option<int>         fwver(op, "r", "fwversion", 0, "value",
                                   "Firmware version to load (0=auto)");
  eudaq::Option<int>         wait(op, "w", "wait", 1000, "ms",
                                  "Time to wait between updates in milliseconds");
  eudaq::Option<int>         strobeperiod(op, "p", "strobeperiod", 1000, "cycles",
                                  "Period for timing strobe in clock cycles");
  eudaq::Option<int>         strobelength(op, "l", "strobelength", 100, "cycles",
                                  "Length of 'on' time for timing strobe in clock cycles");
  eudaq::Option<int>         enabledutveto(op, "b", "dutveto", 0, "mask",
                                  "Mask for enabling veto of triggers ('backpressure') by rasing DUT_CLK");
  eudaq::Option<int>         lvpowervctrl(op, "pw", "powervctrl", (int)PMT_VCNTL_DEFAULT, "mV", "PMT Vcntl, range from 0 to 1000 [or 2000]");
  eudaq::Option<int>         pmtvcntl(op, "pv", "pmtvcntl", -1, "mV", "PMT Vcntl (will override \"pw\" value if set to 0 or greater)");
  eudaq::Option<int>         pmtvcntl_1(op, "p1", "pmtvcntl1", -1, "mV", "PMT 1 Vcntl (will override \"pv\" value if set to 0 or greater)");
  eudaq::Option<int>         pmtvcntl_2(op, "p2", "pmtvcntl2", -1, "mV", "PMT 2 Vcntl (will override \"pv\" value if set to 0 or greater)");
  eudaq::Option<int>         pmtvcntl_3(op, "p3", "pmtvcntl3", -1, "mV", "PMT 3 Vcntl (will override \"pv\" value if set to 0 or greater)");
  eudaq::Option<int>         pmtvcntl_4(op, "p4", "pmtvcntl4", -1, "mV", "PMT 4 Vcntl (will override \"pv\" value if set to 0 or greater)");
  eudaq::Option<int>         pmtvcntlmod(op, "pm", "pmtvcntlmod", 0, "value", "0: unmodified (<= 1000mV); 1: modified Vcntl range 0 to 2000mV");

  eudaq::OptionFlag          nots(op, "n", "notimestamp", "Do not read out timestamp buffer");
  eudaq::OptionFlag          quit(op, "q", "quit", "Quit after configuring TLU");
  eudaq::OptionFlag          pause(op, "u", "wait-for-user", "Wait for user input before starting triggers");
  eudaq::Option<std::string> sname(op, "s", "save-file", "", "filename",
                                   "The filename to save trigger numbers and timestamps");
  eudaq::Option<std::string> trace(op, "z", "trace-file", "", "filename",
                                   "The filename to save a trace of all usb accesses,\n"
                                   "prepend - for only errors, or + for all data (including block transfers)");
  try {
    op.Parse(argv);
    for (size_t i = TLU_LEMO_DUTS; i < ipsel.NumItems(); ++i) {
      if (TLUController::DUTnum(ipsel.Item(i)) != TLUController::IN_RJ45) throw eudaq::OptionException("Invalid DUT input selection");
    }
    unsigned pmt_override_set = 0;
    for(int i = 0; i < TLU_PMTS; i++)
    {
	 pmtvcntl_values[i] = pmtvcntl.Value() == -1 ? lvpowervctrl.Value() : pmtvcntl.Value();
    }

    if(pmtvcntl_1.Value() != -1) { pmtvcntl_values[0] = pmtvcntl_1.Value(); pmt_override_set = 1; }
    if(pmtvcntl_2.Value() != -1) { pmtvcntl_values[1] = pmtvcntl_2.Value(); pmt_override_set = 1; }
    if(pmtvcntl_3.Value() != -1) { pmtvcntl_values[2] = pmtvcntl_3.Value(); pmt_override_set = 1; }
    if(pmtvcntl_4.Value() != -1) { pmtvcntl_values[3] = pmtvcntl_4.Value(); pmt_override_set = 1; }

    unsigned pmt_max_val;
    if(pmtvcntlmod.Value() == 0)
    {
        pmt_max_val = 1000;
    }
    else
    {
	pmt_max_val = 2000;
    }

    for(int i = 0; i < TLU_PMTS; i++)
    {
	if(pmtvcntl_values[i] > pmt_max_val)
	{
	    if(pmtvcntlmod.Value() == 0)
	    {
		throw eudaq::OptionException("Invalid PMT Vcntl value (> 1000mV)");
	    }
	    else
	    {
		throw eudaq::OptionException("Invalid PMT Vcntl value (> 2000mV)");
	    }
	}
    }
    std::cout << "Using options:\n"
              << "TLU version = " << fwver.Value() << (fwver.Value() == 0 ? " (auto)" : "") << "\n"
              << "Bit file name = '" << fname.Value() << "'" << (fname.Value() == "" ? " (auto)" : "") << "\n"
              << "Hand shake mode = " << hsmode.Value() << "\n"
              << "Trigger interval = " << trigg.Value() << "\n"
              << (trigg.Value() > 0 ? " ms (" + to_string(1e3/trigg.Value()) + " Hz)" : std::string()) << "\n"
              << "DUT Mask  = " << hexdec(dmask.Value(), 2) << "\n"
              << "Veto Mask = " << hexdec(vmask.Value(), 2) << "\n"
              << "And Mask  = " << hexdec(amask.Value(), 2) << "\n"
              << "Or Mask   = " << hexdec(omask.Value(), 2) << "\n"
              << "DUT inputs = " << to_string(ipsel.Value()) << "\n"
              << "Strobe period = " << hexdec(strobeperiod.Value(), 6) << "\n"
              << "Strobe length = " << hexdec(strobelength.Value(), 6) << "\n"
              << "Enable DUT Veto = " << hexdec(enabledutveto.Value(), 2) << "\n"
              << "PMT Vcntl Mod = " << pmtvcntlmod.Value() << " (0: standard [<= 1000mV], else: modified for 2000mV range)" << std::endl
              << "PMT 1 Vcntl = " << pmtvcntl_values[0] << " mV" << std::endl
              << "PMT 2 Vcntl = " << pmtvcntl_values[1] << " mV" << std::endl
              << "PMT 3 Vcntl = " << pmtvcntl_values[2] << " mV" << std::endl
              << "PMT 4 Vcntl = " << pmtvcntl_values[3] << " mV" << std::endl
              << "Save file = '" << sname.Value() << "'" << (sname.Value() == "" ? " (none)" : "") << "\n"
              << std::endl;
    std::shared_ptr<std::ofstream> sfile;
    if (sname.Value() != "") {
      sfile =std::make_shared<std::ofstream>(sname.Value().c_str());
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
    TLUController TLU(emode.Value());
    TLU.SetVersion(fwver.Value());
    TLU.SetFirmware(fname.Value());
    TLU.Configure();
    //TLU.FullReset();
    TLU.SetHandShakeMode(hsmode.Value()); //$$ change
    TLU.SetTriggerInterval(trigg.Value());
    if (ipsel.NumItems() > (unsigned)TLU_LEMO_DUTS) ipsel.Resize(TLU_LEMO_DUTS);
    for (size_t i = 0; i < ipsel.NumItems(); ++i) {
      unsigned mask = 1U << i;
      if (i == ipsel.NumItems() - 1) {
        for (size_t j = i+1; j < (unsigned)TLU_LEMO_DUTS; ++j) {
          mask |= 1U << j;
        }
      }
      TLU.SelectDUT(ipsel.Item(i), mask, false);
    }
    TLU.SetDUTMask(dmask.Value());
    TLU.SetVetoMask(vmask.Value());
    TLU.SetAndMask(amask.Value());
    TLU.SetOrMask(omask.Value());
    TLU.SetStrobe(strobeperiod.Value() , strobelength.Value());
    TLU.SetEnableDUTVeto(enabledutveto.Value());
    if(pmt_override_set)
    {
	TLU.SetPMTVcntl(pmtvcntl_values);
    }
    else
    {
	TLU.SetPMTVcntl(pmtvcntl_values[0]);  // Since all 4 are the same (saves I/O to TLU)
    }

    TLU.ResetTimestamp(); // also sets strobe running (if enabled) 

    std::cout << "TLU Version = " << TLU.GetVersion() << "\n"
              << "TLU Serial number = " << eudaq::hexdec(TLU.GetSerialNumber(), 4) << "\n"
              << "Firmware file = " << TLU.GetFirmware() << "\n"
              << "Firmware version = " << TLU.GetFirmwareID() << "\n"
              << "Library version = " << TLU.GetLibraryID() << "\n"
              << std::endl;

    if (TLU.GetFirmwareID() != TLU.GetLibraryID()) {
      std::cout << "Warning: *** Firmware version does not match Library version ***\n" << std::endl;
    }
    if (quit.IsSet()) return 0;
    eudaq::Timer totaltime, lasttime;

    if (pause.IsSet()) {
      std::cerr << "Press enter to start triggers." << std::endl;
      std::getchar();
    }
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
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}

