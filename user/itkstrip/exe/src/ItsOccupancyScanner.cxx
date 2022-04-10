#include "eudaq/OptionParser.hh"
#include "eudaq/FileReader.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/StdEventConverter.hh"
#include "eudaq/Event.hh"
//#include "TEST.hh"

#include <iostream>
#include <fstream> 

#include <iomanip>
#include <vector>
#include <map>
#include <deque>

#include <stdlib.h>

int main(int /*argc*/, const char **argv) {

  eudaq::OptionParser op("EUDAQ Command Line Re-synchroniser for ITkStrip", "0.1", "ITKStrip Resynchroniser");
  eudaq::Option<std::string> file_input(op, "i", "input", "", "string", "input file (eg. run000001.raw)");
  eudaq::OptionFlag debugPrint(op, "d", "debug", "print lots of information");
  eudaq::Option<uint32_t> planeId(op, "p", "planeid", 29, "uint32_t", "Plane ID to be checked");
  eudaq::Option<uint32_t> lowerX(op, "s", "lowerx", 0, "uint32_t", "X coordinate at which to start occupancy measure, default 0");
  eudaq::Option<uint32_t> upperX(op, "e", "upperx", 1280, "uint32_t", "X coordinate at which to stop occupancy measure (not inclusive), default 1280");
  eudaq::Option<uint32_t> ysize(op, "y", "ysize", 1, "uint32_t", "Number of y coordinates expected, default 1");


  EUDAQ_LOG_LEVEL("INFO");
  try{
    op.Parse(argv);
  }
  catch (...) {
    return op.HandleMainException();
  }

  std::string infile_path = file_input.Value();
  if (infile_path.length() == 0) {
  	std::cout << "Please define an input file, if not sure how, try --help" << std::endl;
  	return -1;
  }
  std::string type_in = infile_path.substr(infile_path.find_last_of(".")+1);
  if(type_in=="raw")
    type_in = "native";

  bool debugOutput = debugPrint.Value();
  unsigned int xcut_low = lowerX.Value();
  unsigned int xcut_high = upperX.Value();
  unsigned int search_plane = planeId.Value();

  if (xcut_high <= xcut_low) {
  	std::cout << "Check your cuts" << std::endl;
  	return -1;
  }

  eudaq::FileReaderUP reader;
  reader = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash(type_in), infile_path);
  
  unsigned long long int nhits = 0;
  unsigned int pixelCount= xcut_high - xcut_low;
  pixelCount *= ysize.Value();
  std::cout << "Assumed Plane y dimension: " << ysize.Value() << std::endl;
  unsigned int eventCount=0;
  bool y_size_checked = false;
  unsigned int histogram[1280*4];
  for (unsigned int i=0; i< 1280*4; i++) {
  	histogram[i] = 0;
  }
  while(1){
    auto ev = reader->GetNextEvent();
    if(!ev)
      break;
  
    auto evstd = eudaq::StandardEvent::MakeShared();
    eudaq::StdEventConverter::Convert(ev, evstd, nullptr);

    uint32_t ev_n = ev->GetEventN();
    std::vector<eudaq::EventSPC> subev_col = evstd->GetSubEvents();
    if (debugOutput) std::cout << "Number of Planes: " << evstd->NumPlanes() << std::endl;
    for(size_t i_plane = 0; i_plane < evstd->NumPlanes(); i_plane++) {
      auto plane = evstd->GetPlane(i_plane);
      if (debugOutput) std::cout << "Found plane: " << plane.ID() << std::endl;
      if (search_plane == plane.ID()) {
        if (debugOutput) std::cout << "Using plane: " << plane.ID() << " with size " << plane.YSize() << std::endl;
        for(unsigned int i = 0; i < plane.HitPixels(); i++) {
          auto col = static_cast<int>(plane.GetX(i));
          auto row = static_cast<int>(plane.GetY(i));
          auto raw = static_cast<int>(plane.GetPixel(i)); // generic pixel raw value (could be ToT, ADC, ...)
          if (raw != 0) {
            histogram[col]++;
          }
          if (debugOutput) std::cout << "Found Hit: " << col << " " << row << " " << raw << std::endl;
          if ((raw != 0) && (col>=xcut_low) && (col<xcut_high)) {
            if (debugOutput) std::cout << "Hit counted!" << std::endl;
            nhits++;
          }
        }
      }
    }
    eventCount++;
  }

  for (unsigned int i=xcut_low; i<xcut_high; i++) {
  	std::cout << i << "\t" << std::setprecision(9) << double(histogram[i])/double(eventCount) << std::endl;
  }

  double occupancy = double(nhits)/double(pixelCount)/double(eventCount);

  //std::cout << "Total Occupancy: " << std::setprecision(9) << occupancy << " from nHits: " << nhits << " in nEvts: " << eventCount << " over nPixels: " << pixelCount << std::endl;
  return 0;
}


/*
Make a map matching algorithm, something that assumes the latter event index in an RLE block is the correct global ID and thus speaks for both blocks.
Based on that, two blocks should have a matching subset of events. Giving that blocks are unique within a run (per device), the overlap of two blocks should be unique within the run
Means pair-wise comparison should deliver a non-overlapping set of blocks.
*/