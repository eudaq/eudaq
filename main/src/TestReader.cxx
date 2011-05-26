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
using namespace std;

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
    
    if (!do_display) do_zs = false; // "data range is not given"); (if event range is not set -> zs = false (do not print pixels), otherwise..)
    if (do_zs) do_display = false;  // "Print pixels for zs events"); (if print pixels is set -> do not do, what you do if event range is given )  :)
    
    //std::cout << "DEBUG " << ndata << ", " << do_display << ", " << do_zs << std::endl;

    if (do_process || do_display || do_dump || do_zs) {

        // do this if an event range is given
        if (do_display) {
            
            std::cout << dev << std::endl;
            
            if (do_dump) {

                unsigned num = 0;
                
                for (size_t i = 0; i < dev.NumEvents(); ++i) {
                    
                    const eudaq::RawDataEvent * rev = dynamic_cast<const eudaq::RawDataEvent *>(dev.GetEvent(i));
                    
                    if (rev && /*rev->GetSubType() == "EUDRB" &&*/ rev->NumBlocks() > 0) {
                        
                        ++num;
                        for (unsigned block = 0; block < rev->NumBlocks(); ++block) {
                            
                            std::cout << "#### Producer " << num << ", block " << block << " (ID " << rev->GetID(block) << ") ####" << std::endl;
                            const std::vector<unsigned char> & data = rev->GetBlock(block);
                            
                            for (size_t i = 0; i+3 < data.size(); i += 4) {
                                
                                std::cout << std::setw(4) << i << " " << eudaq::hexdec(eudaq::getbigendian<unsigned>(&data[i])) << std::endl;
                            }
                            
                            //break;
                        }
                    }
                }
            }
        }
    
        // do this if 
        if (do_process) {
            
            unsigned boardnum = 0;
            const StandardEvent & sev = eudaq::PluginManager::ConvertToStandard(dev);
            
            if (do_display) std::cout << "Standard Event: " << sev << std::endl;
            
            for (size_t iplane = 0; iplane < sev.NumPlanes(); ++iplane) {
                
                const eudaq::StandardPlane & plane = sev.GetPlane(iplane);
                std::vector<double> cds = plane.GetPixels<double>();
                //std::cout << "DBG " << eudaq::hexdec(plane.m_flags);

                bool bad = false;

                if (bad) std::cout << "***" << std::endl;
                
                if (do_zs) {
                    
                    for (size_t ipix = 0; ipix < 20 && ipix < cds.size(); ++ipix) {
                        std::cout << ipix << ": " << eudaq::hexdec(cds[ipix]) << std::endl;
                    }
                }
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
  eudaq::OptionFlag do_dump(op, "u", "dump", "Dump raw data for displayed events");
  eudaq::OptionFlag do_zs(op, "z", "zsdump", "Print pixels for zs events");
  eudaq::OptionFlag sync(op, "s", "synctlu", "Resynchronize subevents based on TLU event number");
  eudaq::OptionFlag do_event_to_ttree(op, "r", "event-to-ttree", "Convert a file into a TTree .root format");
  eudaq::Option<std::string> level(op, "l", "log-level", "INFO", "level",
				     "The minimum level for displaying log messages locally");
  try {
        
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    std::vector<unsigned> displaynumbers = parsenumbers(do_data.Value());
    //for (unsigned i = 0; i < displaynumbers.size(); ++i) std::cout << "+ " << displaynumbers[i] << std::endl;

    bool showlast = std::find(displaynumbers.begin(), displaynumbers.end(), (unsigned)-1) != displaynumbers.end();
      
    counted_ptr<eudaq::Event> lastevent;

    if (do_event_to_ttree.IsSet()) throw eudaq::MessageException("The -r option is deprecated: use \"./Converter.exe -t root\" instead.");
        
    for (size_t i = 0; i < op.NumArgs(); ++i) {
                         
      eudaq::FileReader reader(op.GetArg(i), ipat.Value(), sync.IsSet());
      EUDAQ_INFO("Reading: " + reader.Filename());

//    cout << i << " " << reader.Filename()  << endl;

      unsigned ndata = 0, ndatalast = 0, nnondet = 0, nbore = 0, neore = 0;
            
      do {
        const eudaq::Event & ev = reader.GetEvent();
        if (ev.IsBORE()) {
          nbore++;
          if (nbore > 1) {
            EUDAQ_WARN("Multiple BOREs (" + to_string(nbore) + ")");
          }

          if (do_bore.IsSet()) {
            std::cout << ev << std::endl;
          }
          
          if (const eudaq::DetectorEvent * dev = dynamic_cast<const eudaq::DetectorEvent *>(&ev)) {
            eudaq::PluginManager::Initialize(*dev);
          }
        } else if (ev.IsEORE()) {
          neore++;
          if (neore > 1) {
            EUDAQ_WARN("Multiple EOREs (" + to_string(neore) + ")");
          }
          
          if (do_eore.IsSet()) {          
            std::cout << ev << std::endl;
          }
        
        } else {
          ndata++;
          // TODO: check event number matches ndata

          bool shown = false;
          if (const eudaq::DetectorEvent * dev = dynamic_cast<const eudaq::DetectorEvent *>(&ev)) {
            bool show = std::find(displaynumbers.begin(), displaynumbers.end(), ndata) != displaynumbers.end();
            bool proc = do_pall.IsSet() || (show && do_proc.IsSet());
            bool dump = (do_dump.IsSet());
            shown = DoEvent(ndata, *dev, proc, show, do_zs.IsSet(), dump);
            if (showlast && !shown) {
              lastevent = counted_ptr<eudaq::Event>(new eudaq::DetectorEvent(*dev));
            }
          } else if (const StandardEvent * sev = dynamic_cast<const StandardEvent *>(&ev)) {
            bool show = std::find(displaynumbers.begin(), displaynumbers.end(), ndata) != displaynumbers.end();
            if (show) {
              std::cout << *sev << std::endl;
              shown = true;
            } else {
              if (showlast) lastevent = counted_ptr<eudaq::Event>(new eudaq::StandardEvent(*sev));
            }
          }
        }
            
      } while (reader.NextEvent());
      
      if (lastevent.get()) {
        if (const eudaq::DetectorEvent * dev = dynamic_cast<const eudaq::DetectorEvent *>(lastevent.get())) {
          DoEvent(ndatalast, *dev, false, true, do_zs.IsSet(), false);
        }
      }
      EUDAQ_INFO("Number of data events: " + to_string(ndata));
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
//  cout << "Last event read is # "<< i_last << " (should be by 2 more than Event in the file counting the EORE & BORE)" << endl;
  } catch (...) {      
    return op.HandleMainException();
  }
     
  return 0;

}
