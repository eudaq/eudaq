#include "eudaq/FileReader.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"
#include "eudaq/DetectorEvent.hh"
#include "eudaq/RawDataEvent.hh"

#include <iostream>
#include <fstream>

using eudaq::StandardEvent;
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
      long min = from_string(ranges[i].substr(0, j), 0);
      long max = from_string(ranges[i].substr(j+1), 0);
      if (j == 0 && max == 1) {
        result.push_back((unsigned)-1);
      } else if (j == 0 || j == ranges[i].length()-1 || min < 0 || max < min) {
        EUDAQ_THROW("Bad range");
      } else {
        for (long n = min; n <= max; ++n) {
          result.push_back(n);
        }
      }
    }
  }
  return result;
}

bool DoEvent(unsigned /*ndata*/, const eudaq::DetectorEvent & dev, bool do_process, bool do_display, bool do_zs, bool do_dump) {
  if (!do_display) do_zs = false;
  if (do_zs) do_display = false;
  //std::cout << "DEBUG " << ndata << ", " << do_display << ", " << do_zs << std::endl;
  if (do_process || do_display || do_dump || do_zs) {
    if (do_display) {
      std::cout << dev << std::endl;
      // for (size_t i = 0; i < dev.NumEvents(); ++i) {
      //   const eudaq::RawDataEvent * rev = dynamic_cast<const eudaq::RawDataEvent *>(dev.GetEvent(i));
      //   if (rev && rev->GetSubType() == "EUDRB" && rev->NumBlocks() > 0) {
      //     std::cout << "#### Raw Data: ####" << std::endl;
      //     const std::vector<unsigned char> & data = rev->GetBlock(0);
      //     for (size_t i = 0; i+3 < data.size(); i += 4) {
      //       std::cout << std::setw(2) << i << " " << eudaq::hexdec(eudaq::getbigendian<unsigned long>(&data[i])) << std::endl;
      //     }
      //     //break;
      //   }
      // }
    }
    if (do_process) {
      unsigned boardnum = 0;
      const StandardEvent & sev = eudaq::PluginManager::ConvertToStandard(dev);
      if (do_display) std::cout << "Standard Event: " << sev << std::endl;
      for (size_t i = 0; i < sev.NumPlanes(); ++i) {
        const eudaq::StandardPlane & plane = sev.GetPlane(i);
        std::vector<double> cds = plane.GetPixels<double>();
        //std::cout << "DBG " << eudaq::hexdec(plane.m_flags);
        bool bad = false;
        for (size_t i = 0; i < cds.size(); ++i) {
          //if (i < 10) std::cout << ", " << /*plane.m_pix[0][i] << ";" <<*/ cds[i];
        }
        //std::cout << (bad ? "***" : "") << std::endl;
        if (bad) std::cout << "***" << std::endl;
        if (do_zs) {
          for (size_t i = 0; i < 20 && i < cds.size(); ++i) {
            std::cout << i << ": " << eudaq::hexdec(cds[i]) << std::endl;
          }
        //   std::cout << "  Plane: " << plane << std::endl;
        //   for (size_t p = 0; p < plane.m_pix[0].size(); ++p) {
        //     static const char tab = '\t';
        //     std::cout << "    " << ndata << tab << boardnum << tab << plane.m_x[p] << tab << plane.m_y[p] << tab << plane.m_pix[0][p] << std::endl;
        //   }
        }
//       if (do_dump) {
//         std::ofstream file(("board" + to_string(boardnum) + ".dat").c_str());
//         file.write(reinterpret_cast<const char*>(brd.GetData()-8), brd.DataSize()+16);
//       }
        boardnum++;
      }
    }
  }
  return do_display;
}

int main(int /*argc*/, char ** argv) {
  eudaq::OptionParser op("EUDAQ Raw Data file reader", "1.0",
                         "A command-line tool for printing out a summary of a raw data file",
                         1);
  eudaq::Option<std::string> ipat(op, "i", "inpattern", "../data/run$6R.raw", "string", "Input filename pattern");
  eudaq::OptionFlag do_bore(op, "b", "bore", "Display the BORE event");
  eudaq::OptionFlag do_eore(op, "e", "eore", "Display the EORE event");
  eudaq::OptionFlag do_proc(op, "p", "process", "Process data from displayed events");
  eudaq::OptionFlag do_pall(op, "a", "process-all", "Process data from all events");
  eudaq::Option<std::string> do_data(op, "d", "display", "", "numbers", "Event numbers to display (eg. '1-10,99,-1')");
  eudaq::Option<unsigned> do_dump(op, "u", "dumpevent", 0, "number", "Dump event to files (1 per EUDRB)");
  eudaq::OptionFlag do_zs(op, "z", "zsdump", "Print pixels for zs events");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL("INFO");
    std::vector<unsigned> displaynumbers = parsenumbers(do_data.Value());
    //for (unsigned i = 0; i < displaynumbers.size(); ++i) std::cout << "+ " << displaynumbers[i] << std::endl;
    bool showlast = std::find(displaynumbers.begin(), displaynumbers.end(), (unsigned)-1) != displaynumbers.end();
    counted_ptr<eudaq::DetectorEvent> lastevent;
    for (size_t i = 0; i < op.NumArgs(); ++i) {
      eudaq::FileReader reader(op.GetArg(i), ipat.Value());
      EUDAQ_INFO("Reading: " + reader.Filename());
      unsigned ndata = 0, ndatalast = 0, nnondet = 0, nbore = 0, neore = 0;
      do {
        const eudaq::DetectorEvent & dev = reader.Event();
        if (dev.IsBORE()) {
          nbore++;
          if (nbore > 1) {
            EUDAQ_WARN("Multiple BOREs (" + to_string(nbore) + ")");
          }
          if (do_bore.IsSet()) std::cout << dev << std::endl;
          eudaq::PluginManager::Initialize(dev);
        } else if (dev.IsEORE()) {
          neore++;
          if (neore > 1) {
            EUDAQ_WARN("Multiple EOREs (" + to_string(neore) + ")");
          }
          if (do_eore.IsSet()) std::cout << dev << std::endl;
        } else {
          ndata++;
          // TODO: check event number matches ndata
          bool show = std::find(displaynumbers.begin(), displaynumbers.end(), ndata) != displaynumbers.end();
          bool proc = do_pall.IsSet() || (show && do_proc.IsSet());
          bool dump = (do_dump.IsSet() && do_dump.Value() == ndata);
          bool shown = DoEvent(ndata, dev, proc, show, do_zs.IsSet(), dump);
          if (showlast) {
            if (shown) {
              lastevent = 0;
            } else {
              lastevent = counted_ptr<eudaq::DetectorEvent>(new eudaq::DetectorEvent(dev));
              ndatalast = ndata;
            }
          }
        }
      } while (reader.NextEvent());
      if (lastevent.get()) DoEvent(ndatalast, *lastevent.get(), false, true, do_zs.IsSet(), false);
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
