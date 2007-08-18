#include "eudaq/FileSerializer.hh"
#include "eudaq/DetectorEvent.hh"
#include "eudaq/EUDRBEvent.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"

#include <iostream>

using eudaq::from_string;
using eudaq::to_string;
using eudaq::split;

std::vector<unsigned> parsenumbers(const std::string & s) {
  std::vector<unsigned> result;
  std::vector<std::string> ranges = split(s, ",");
  for (size_t i = 0; i < ranges.size(); ++i) {
    size_t j = ranges[i].find('-');
    if (j == std::string::npos) {
      unsigned v = from_string(ranges[i], 0);
      result.push_back(v);
    } else {
      unsigned min = from_string(ranges[i].substr(0, j), 0);
      unsigned max = from_string(ranges[i].substr(j+1), 0);
      if (j == 0 && max == 1) {
        result.push_back((unsigned)-1);
      } else if (j == 0 || j == ranges[i].length()-1 || max < min) {
        EUDAQ_THROW("Bad range");
      } else {
        for (unsigned n = min; n <= max; ++n) {
          result.push_back(n);
        }
      }
    }
  }
  return result;
}

bool DoEvent(unsigned ndata, eudaq::DetectorEvent * dev, bool do_process, bool do_display, bool do_dump, eudaq::EUDRBDecoder * decoder) {
  if (do_process || do_display || do_dump) {
    if (do_display) {
      std::cout << *dev << std::endl;
    }
    unsigned boardnum = 0;
    for (size_t i = 0; i < dev->NumEvents(); ++i) {
      eudaq::Event * subev = dev->GetEvent(i);
      eudaq::EUDRBEvent * eudev = dynamic_cast<eudaq::EUDRBEvent*>(subev);
      if (eudev) {
        if (!decoder) EUDAQ_ERROR("Missing EUDRB BORE, cannot decode events");
        if (do_display) std::cout << "EUDRB Event:" << std::endl;
        for (size_t j = 0; j < eudev->NumBoards(); ++j) {
          eudaq::EUDRBBoard & brd = eudev->GetBoard(j);
          boardnum++;
          if (do_display) std::cout << " Board " << j << ":\n" << brd;
          if (do_process && decoder) {
            try {
              decoder->GetArrays<short, short>(brd);
            } catch (const eudaq::Exception & e) {
              std::cout << "Error in event " << ndata << ": " << e.what() << std::endl;
            }
          }
          if (do_dump) {
            std::ofstream file(("board" + to_string(boardnum) + ".dat").c_str());
            file.write(reinterpret_cast<const char*>(brd.GetData()-8), brd.DataSize()+16);
          }
        }
      }
    }
  }
  return do_display;
}

int main(int /*argc*/, char ** argv) {
  eudaq::OptionParser op("EUDAQ Raw Data file reader", "1.0",
                         "A command-line tool for printing out a summary of a raw data file",
                         1);
  eudaq::OptionFlag do_bore(op, "b", "bore", "Display the BORE event");
  eudaq::OptionFlag do_eore(op, "e", "eore", "Display the EORE event");
  eudaq::OptionFlag do_proc(op, "p", "process", "Process data from displayed events");
  eudaq::OptionFlag do_pall(op, "a", "process-all", "Process data from all events");
  eudaq::Option<std::string> do_data(op, "d", "display", "", "numbers", "Event numbers to display (eg. '1-10,99,-1')");
  eudaq::Option<unsigned> do_dump(op, "u", "dumpevent", 0, "number", "Dump event to files (1 per EUDRB)");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL("INFO");
    std::vector<unsigned> displaynumbers = parsenumbers(do_data.Value());
    //for (unsigned i = 0; i < displaynumbers.size(); ++i) std::cout << "+ " << displaynumbers[i] << std::endl;
    bool showlast = std::find(displaynumbers.begin(), displaynumbers.end(), (unsigned)-1) != displaynumbers.end();
    counted_ptr<eudaq::Event> lastevent;
    for (size_t i = 0; i < op.NumArgs(); ++i) {
      std::string datafile = op.GetArg(i);
      if (datafile.find_first_not_of("0123456789") == std::string::npos) {
        datafile = "data/run" + to_string(from_string(datafile, 0), 6) + ".raw";
      }
      EUDAQ_INFO("Reading: " + datafile);
      eudaq::FileDeserializer des(datafile);
      unsigned ndata = 0, ndatalast = 0, nnondet = 0, nbore = 0, neore = 0;
      counted_ptr<eudaq::EUDRBDecoder> decoder;
      while (des.HasData()) {
        counted_ptr<eudaq::Event> ev(eudaq::EventFactory::Create(des));
        eudaq::DetectorEvent * dev = dynamic_cast<eudaq::DetectorEvent*>(ev.get());
        if (ev->IsBORE()) {
          nbore++;
          if (nbore > 1) {
            EUDAQ_WARN("Multiple BOREs (" + to_string(nbore) + ")");
          }
          if (do_bore.IsSet()) std::cout << *ev << std::endl;
          for (size_t i = 0; i < dev->NumEvents(); ++i) {
            if (eudaq::EUDRBEvent * eudev = dynamic_cast<eudaq::EUDRBEvent*>(dev->GetEvent(i))) {
              if (eudev->IsBORE()) {
                decoder = new eudaq::EUDRBDecoder(*eudev);
                break;
              }
            }
          }
        } else if (ev->IsEORE()) {
          neore++;
          if (neore > 1) {
            EUDAQ_WARN("Multiple EOREs (" + to_string(neore) + ")");
          }
          if (do_eore.IsSet()) std::cout << *ev << std::endl;
        } else if (!dev) {
          nnondet++;
          EUDAQ_WARN("Not a DetectorEvent(" + to_string(nnondet) + ")");
          std::cout << *ev << std::endl;
        } else {
          ndata++;
          // TODO: check event number matches ndata
          bool show = std::find(displaynumbers.begin(), displaynumbers.end(), ndata) != displaynumbers.end();
          bool proc = do_pall.IsSet() || (show && do_proc.IsSet());
          bool dump = (do_dump.IsSet() && do_dump.Value() == ndata);
          bool shown = DoEvent(ndata, dev, proc, show, dump, decoder.get());
          if (showlast) {
            if (shown) {
              lastevent = 0;
            } else {
              lastevent = ev;
              ndatalast = ndata;
            }
          }
        }
      }
      if (lastevent.get()) DoEvent(ndatalast, dynamic_cast<eudaq::DetectorEvent*>(lastevent.get()), false, true, false, decoder.get());
      EUDAQ_INFO("Number of data events: " + to_string(ndata));
      if (nnondet) std::cout << "Warning: Non-DetectorEvents found: " << nnondet << std::endl;
      if (!nbore) {
        std::cout << "Warning: No BORE found" << std::endl;
      } else if (nbore > 1) {
        std::cout << "Warning: Multiple BOREs found: " << nbore << std::endl;
      }
      if (!neore) {
        std::cout << "Warning: No EORE found, possibly truncated file." << std::endl;
      } else if (neore > 1) {
        std::cout << "Warning: Multiple EOREs found: " << nbore << std::endl;
      }
      if (nnondet || (nbore != 1) || (neore > 1)) std::cout << "Probably corrupt file." << std::endl;
    }
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
