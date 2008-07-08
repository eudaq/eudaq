#include "eudaq/FileSerializer.hh"
#include "eudaq/DetectorEvent.hh"
#include "eudaq/EUDRBEvent.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"

#include <iostream>

using eudaq::from_string;
using eudaq::to_string;

template <typename T>
inline const T * GetSubEvent(const eudaq::DetectorEvent * dev) {
  for (size_t i = 0; i < dev->NumEvents(); i++) {
    const T * sev = dynamic_cast<const T*>(dev->GetEvent(i));
    if (sev) return sev;
  }
  return 0;
}

void Error(const std::string & msg, const eudaq::Event * e) {
  std::cerr << (e ? "Event " + to_string(e->GetEventNumber()) + ": " : "") << msg << std::endl;;
  if (e) std::cerr << *e << std::endl;
}

void Fatal(const std::string & msg, const eudaq::Event * e) {
  Error(msg, e);
  exit(1);
}

int main(int /*argc*/, char ** argv) {
  eudaq::OptionParser op("EUDAQ Raw Data file checker", "1.0",
                         "A command-line tool for checking consistency of raw data file",
                         1);
  eudaq::OptionFlag disp(op, "d", "display",
                         "Display event numbers");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL("INFO");
    for (size_t i = 0; i < op.NumArgs(); ++i) {
      std::string datafile = op.GetArg(i);
      if (datafile.find_first_not_of("0123456789") == std::string::npos) {
        datafile = "../data/run" + to_string(from_string(datafile, 0), 6) + ".raw";
      }
      EUDAQ_INFO("Reading: " + datafile);
      eudaq::FileDeserializer des(datafile);
      counted_ptr<eudaq::Event> ev(eudaq::EventFactory::Create(des));
      eudaq::DetectorEvent * dev = dynamic_cast<eudaq::DetectorEvent*>(ev.get());
      unsigned neore = 0, evtexpect = 0, tluexpect = (unsigned)-1;
      if (!dev) Fatal("Not a detector event", ev.get());
      if (!dev->IsBORE()) Error("First event is not a BORE", dev);
      if (dev->GetEventNumber() != evtexpect) Error("Unexpected event number "
                                                    + to_string(dev->GetEventNumber())
                                                    + ", expect " + to_string(evtexpect),
                                                    dev);
      evtexpect = dev->GetEventNumber() + 1;
      counted_ptr<eudaq::EUDRBDecoder> decoder(new eudaq::EUDRBDecoder(*dev));
      for (size_t i = 0; i < dev->NumEvents(); ++i) {
        if (!dev->GetEvent(i)->IsBORE()) Error("Subevent of BORE is not a BORE", dev);
        if (dev->GetEvent(i)->IsEORE()) Error("Subevent of BORE is an EORE", dev);
      }
      if (disp.IsSet()) std::cout << "Event\tTLU\tEUDRB\t.Local\t.TLU\n";
      while (des.HasData()) {
        ev = eudaq::EventFactory::Create(des);
        dev = dynamic_cast<eudaq::DetectorEvent*>(ev.get());
        if (!dev) Fatal("Not a detector event", ev.get());
        if (dev->IsBORE()) Error("More than one BORE", dev);
        if (dev->GetEventNumber() != evtexpect) Error("Unexpected event number "
                                                      + to_string(dev->GetEventNumber())
                                                      + ", expect " + to_string(evtexpect),
                                                      dev);
        evtexpect = dev->GetEventNumber() + 1;
        if (ev->IsEORE()) {
          neore++;
          if (neore > 1) Error("More then one EORE", dev);
          for (size_t i = 0; i < dev->NumEvents(); ++i) {
            if (!dev->GetEvent(i)->IsEORE()) Error("Subevent of EORE is not an EORE", dev);
            if (dev->GetEvent(i)->IsBORE()) Error("Subevent of EORE is a BORE", dev);
          }
        } else {
          if (neore) Error("Event after EORE", dev);
          for (size_t i = 0; i < dev->NumEvents(); ++i) {
            if (dev->GetEvent(i)->GetEventNumber() != dev->GetEventNumber()) Error("Mismatched Event Number", dev);
          }
          const eudaq::EUDRBEvent * eev = GetSubEvent<eudaq::EUDRBEvent>(dev);
          const eudaq::TLUEvent * tev = GetSubEvent<eudaq::TLUEvent>(dev);
          if (disp.IsSet()) std::cout << dev->GetEventNumber()
                                      << '\t' << (tev ? to_string(tev->GetEventNumber()) : "-")
                                      << '\t' << (eev ? to_string(eev->GetEventNumber()) : "-");
          if (eev) {
            bool sameloc = true, sametlu = true;
            int numsame = 0, numdiff = 0;
            for (size_t b = 0; b < eev->NumBoards(); ++b) {
              const eudaq::EUDRBBoard & brd = eev->GetBoard(b);
              if (tluexpect!= (unsigned)-1 && brd.TLUEventNumber() == tluexpect) {
                numsame++;
              } else if (tluexpect != (unsigned)-1) {
                numdiff++;
              }
              if (b > 0) {
                if (brd.LocalEventNumber() != eev->GetBoard(b-1).LocalEventNumber()) sameloc = false;
                if (brd.TLUEventNumber() != eev->GetBoard(b-1).TLUEventNumber()) sametlu = false;
              }
            }
            if (disp.IsSet()) std::cout << '\t' << eev->GetBoard(0).LocalEventNumber();
            if (!sameloc) {
              if (disp.IsSet()) {
                for (size_t b = 1; b < eev->NumBoards(); ++b) {
                  std::cout << ',' << eev->GetBoard(b).LocalEventNumber();
                }
              }
              Error("Mismatched Local event numbers", dev);
            }
            if (disp.IsSet()) std::cout << '\t' << eev->GetBoard(0).TLUEventNumber();
            if (!sametlu) {
              if (disp.IsSet()) {
                for (size_t b = 1; b < eev->NumBoards(); ++b) {
                  std::cout << ',' << eev->GetBoard(b).TLUEventNumber();
                }
              }
              Error("Mismatched TLU event numbers", dev);
            }
            if (numdiff > 0) {
              std::string msg;
              if (sametlu && eev->GetBoard(0).TLUEventNumber() == 0) {
                msg = "Zero TLU event number, ev=" + to_string(dev->GetEventNumber())
                  + ", expect=" + to_string(tluexpect);
              } else if (sametlu && eev->GetBoard(0).TLUEventNumber() == tluexpect+1) {
                int offset = eev->GetBoard(0).TLUEventNumber() - (dev->GetEventNumber() % 32768);
                msg = "Missed TLU event number, ev=" + to_string(dev->GetEventNumber())
                  + ", offset=" + to_string(offset);
              } else {
                msg = "Unexpected TLU event number, ev=" + to_string(dev->GetEventNumber())
                  + ", expect=" + to_string(tluexpect) + ", values=" + to_string(eev->GetBoard(0).TLUEventNumber());
                if (!sametlu) {
                  for (size_t b = 1; b < eev->NumBoards(); ++b) {
                    msg += "," + to_string(eev->GetBoard(b).TLUEventNumber());
                  }
                }
              }
              Error(msg, 0);
            }
            if (sametlu) {
              if (tluexpect != (unsigned)-1 && eev->GetBoard(0).TLUEventNumber() == 0) {
                tluexpect++;
              } else {
                tluexpect = eev->GetBoard(0).TLUEventNumber() + 1;
              }
            } else if (numsame) {
              tluexpect++;
            } else {
              tluexpect = (unsigned)-1;
            }
            if (tluexpect != (unsigned)-1) tluexpect %= 65536;
          }
          if (disp.IsSet()) std::cout << std::endl;
        }
      }
    }
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
