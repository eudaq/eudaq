#include "eudaq/FileReader.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"
#include "eudaq/DetectorEvent.hh"
#include "eudaq/RawDataEvent.hh"

#include "TFile.h"
#include "TTree.h"
#include "TRandom.h"
#include "TString.h"



#include <iostream>
#include <fstream>

using eudaq::StandardEvent;
using eudaq::from_string;
using eudaq::to_string;
using eudaq::split;
using namespace std;

// Book variables for the Event_to_TTree conversion 
static bool do_ttree    = false; // static variable to see if the tree is open already for writing  
TFile *tfile            = 0 ; // book the pointer to a file (to store the otuput)
TTree *ttree            = 0 ; // book the tree (to store the needed event info)
Int_t id_plane          = 0 ; // plane id, where the hit is 
Int_t id_hit            = 0 ; // the hit id (within a plane)  
Double_t  id_x          = 0 ; // the hit position along  X-axis  
Double_t  id_y          = 0 ; // the hit position along  Y-axis  
unsigned i_tlu          = 0 ; // a trigger id
unsigned i_run          = 0 ; // a run  number 
unsigned i_event        = 0 ; // an event number 
//
unsigned long long int i_time_stamp = 0 ; // the time stampe 
static int           i_first      = 0 ; // the event range to read (first)
static int           i_last       = 0 ; // the event range to read (last)
static int           i_curr       = 0 ; // the event range to read (last)
//

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

    // always do this 
    if (do_process || do_display || do_dump || do_zs || do_ttree) {
        

        // do this if an event range is given
        if (do_display) {
            
            std::cout << dev << std::endl;
            
            if (do_dump) {

                unsigned num = 0;
                
                for (size_t i = 0; i < dev.NumEvents(); ++i) {
                    
                    const eudaq::RawDataEvent * rev = dynamic_cast<const eudaq::RawDataEvent *>(dev.GetEvent(i));
                    
                    if (rev && rev->GetSubType() == "EUDRB" && rev->NumBlocks() > 0) {
                        
                        ++num;
                        for (unsigned block = 0; block < rev->NumBlocks(); ++block) {
                            
                            std::cout << "#### Producer " << num << ", block " << block << " (ID " << rev->GetID(block) << ") ####" << std::endl;
                            const std::vector<unsigned char> & data = rev->GetBlock(block);
                            
                            for (size_t i = 0; i+3 < data.size(); i += 4) {
                                
                                std::cout << std::setw(4) << i << " " << eudaq::hexdec(eudaq::getbigendian<unsigned long>(&data[i])) << std::endl;
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
//                for (size_t ipix = 0; ipix < cds.size(); ++ipix) {
                //          if (ipix < 10) std::cout << ", " << plane.m_pix[0][ipix] << ";" << cds[ipix]
//                }
                //std::cout << (bad ? "***" : "") << std::endl;

                if (bad) std::cout << "***" << std::endl;
                
                if (do_zs) {
                    
                    for (size_t ipix = 0; ipix < 20 && ipix < cds.size(); ++ipix) {
                        
                        std::cout << ipix << ": " << eudaq::hexdec(cds[ipix]) << std::endl;
                    }

                    //   std::cout << "  Plane: " << plane << std::endl;
                    //   for (size_t p = 0; p < plane.m_pix[0].size(); ++p) {
                    //     static const char tab = '\t';
                    //     std::cout << "    " << ndata << tab << boardnum << tab << plane.m_x[p] << tab << plane.m_y[p] << tab << plane.m_pix[0][p] << std::endl;
                    //   }
                }
                boardnum++;
            }
        }

        // if the tree is open do this
    
//        cout <<   do_ttree <<endl; 
        if( do_ttree ){
            
            const StandardEvent & sev = eudaq::PluginManager::ConvertToStandard(dev);
            
//            cout << "Standard Event: " << sev << endl;
            
            for (size_t iplane = 0; iplane < sev.NumPlanes(); ++iplane) {
                
                const eudaq::StandardPlane & plane = sev.GetPlane(iplane);
                std::vector<double> cds = plane.GetPixels<double>();
                
                for (size_t ipix = 0; ipix < cds.size(); ++ipix) {

                    //          if (ipix < 10) std::cout << ", " << plane.m_pix[0][ipix] << ";" << cds[ipix]

                    id_plane = plane.ID();          
                    id_hit = ipix;
                    id_x = plane.GetX(ipix);
                    id_y = plane.GetY(ipix);
                    i_time_stamp =  sev.GetTimestamp();
                    //          printf("%#x \n", i_time_stamp);  
                    i_tlu = plane.TLUEvent();   
                    i_run = sev.GetRunNumber();
                    i_event = sev.GetEventNumber();                  
                    ttree->Fill(); 
                }               
            }  
       }
        
    }
  return do_display;
}

void StripAllDirs(TString &fstring){
// 
// Removes everything before the last slash '/' (including the slash
// 
    fstring.Remove(0,fstring.Last('/')+1);

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
    
    try {
        
        op.Parse(argv);
        EUDAQ_LOG_LEVEL("INFO");
        std::vector<unsigned> displaynumbers = parsenumbers(do_data.Value());
        //for (unsigned i = 0; i < displaynumbers.size(); ++i) std::cout << "+ " << displaynumbers[i] << std::endl;

        bool showlast = std::find(displaynumbers.begin(), displaynumbers.end(), (unsigned)-1) != displaynumbers.end();

        if(!displaynumbers.empty()){ 
            int n_size = displaynumbers.size();
            i_first = displaynumbers[0];
            i_last  = displaynumbers[n_size-1];
//            cout << i_first  << endl;
//            cout << i_last  << endl;
        }

        counted_ptr<eudaq::DetectorEvent> lastevent;

        if(do_event_to_ttree.IsSet() && do_ttree == false ){
 
            EUDAQ_INFO("Converting the inputfile into a TTree " );
          
            eudaq::FileReader reader(op.GetArg(0), ipat.Value(), sync.IsSet());
            TString foutput = reader.Filename();           
            StripAllDirs(foutput);
            if(foutput.Contains(".raw")) foutput.ReplaceAll("raw" , "root");
            if(foutput.Contains(".root")) {
                foutput.ReplaceAll(".root","" );
                foutput.Append("_" + to_string(i_first) );
                foutput.Append("_" + to_string(i_last) );
                foutput.Append(".root" );   
            }
            
            EUDAQ_INFO("Preparing the outputfile: " +  to_string(foutput.Data()) );

            if(!foutput.Contains(".root")) foutput = "output.root";
                
            do_ttree = true;
            tfile = new TFile(foutput,"RECREATE");
            ttree = new TTree("tree","a simple Tree with simple variables");
            
            ttree->Branch("id_plane",&id_plane,"id_plane/I");
            ttree->Branch("id_hit",&id_hit,"id_hit/I");
            ttree->Branch("id_x",&id_x,"id_x/D");
            ttree->Branch("id_y",&id_y,"id_y/D");
            ttree->Branch("i_time_stamp",&i_time_stamp,"i_time_stamp/LLU");
            ttree->Branch("i_tlu",&i_tlu,"i_tlu/I");
            ttree->Branch("i_run",&i_run,"i_run/I");
            ttree->Branch("i_event",&i_event,"i_event/I");
  
            //fill the tree
/*
        for (Int_t i=0;i<10000;i++) {
            id_plane      = i;
            id_x         = i;
            id_y         = i;
            i_time_stamp = i;
            i_event      = i;
            i_tlu        = i;
            ttree->Fill();
        }

        //save the Tree header. The file will be automatically closed
        //when going out of the function scope

        ttree->Write();
        */
        
        }
    
        
        for (size_t i = 0; i < op.NumArgs(); ++i) {
                         
            eudaq::FileReader reader(op.GetArg(i), ipat.Value());
            EUDAQ_INFO("Reading: " + reader.Filename());

//            cout << i << " " << reader.Filename()  << endl;

            unsigned ndata = 0, ndatalast = 0, nnondet = 0, nbore = 0, neore = 0;
            
            do {
          
                const eudaq::DetectorEvent & dev = reader.Event();
//                i_size  =  dev.NumEvents();    

                if (dev.IsBORE()) {
                    
                    nbore++;
          
                    if (nbore > 1) {
                        
                        EUDAQ_WARN("Multiple BOREs (" + to_string(nbore) + ")");
                    }
          

                    if (do_bore.IsSet()) {
              
                        std::cout << dev << std::endl;
                    }
          
                    eudaq::PluginManager::Initialize(dev);
        
                } else if (dev.IsEORE()) {
            
                    neore++;
                    
                    if (neore > 1) {
              
                        EUDAQ_WARN("Multiple EOREs (" + to_string(neore) + ")");
                    }
          
                    if (do_eore.IsSet()) {          

                        std::cout << dev << std::endl;
                    }
        
                } else if( !do_data.IsSet() || (i_curr >= i_first && i_curr < i_last) ){
                    
                
                    ndata++;
                    // TODO: check event number matches ndata

                    bool show = std::find(displaynumbers.begin(), displaynumbers.end(), ndata) != displaynumbers.end();
                    bool proc = do_pall.IsSet() || (show && do_proc.IsSet());
                    bool dump = (do_dump.IsSet());
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
            
                i_curr++; // keep in mind the last event read from the file
            } while (
                    reader.NextEvent() 
                   );
      
            
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

//        cout << "Last event read is # "<< i_last << " (should be by 2 more than Event in the file counting the EORE & BORE)" << endl;
    
    } catch (...) {
        
        return op.HandleMainException();
    }
    
    // finish by writing the ttree
    if( do_ttree ) ttree->Write();
 
    return 0;

}
