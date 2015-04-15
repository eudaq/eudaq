#include "CMSPixelProducer.hh"

#include "dictionaries.h"
#include "eudaq/Logger.hh"

#include <iostream>
#include <ostream>
#include <vector>

using namespace pxar; 
using namespace std; 

std::vector<int32_t> &CMSPixelProducer::split(const std::string &s, char delim, std::vector<int32_t> &elems) {
  std::stringstream ss(s);
  std::string item;
  int32_t def = 0;
  while (std::getline(ss, item, delim)) {
    elems.push_back(eudaq::from_string(item,def));
  }
  return elems;
}

std::vector<int32_t> CMSPixelProducer::split(const std::string &s, char delim) {
  std::vector<int32_t> elems;
  split(s, delim, elems);
  return elems;
}

std::string CMSPixelProducer::prepareFilename(std::string filename, int16_t n) {

  size_t datpos = filename.find(".dat");
  if(datpos != std::string::npos) {
    filename.insert(datpos,string("_C") + std::to_string(n));
  }
  else { filename += string("_C") + std::to_string(n) + string(".dat"); }
  return filename;
}

std::vector<std::pair<std::string,uint8_t> > CMSPixelProducer::GetConfDACs(int16_t i2c) {

  std::string filename;
  // No I2C address indicator is given, assuming filename is correct "as is":
  if(i2c < 0) { filename = m_config.Get("dacFile", ""); }
  // I2C address is given, appending a "_Cx" with x = I2C:
  else { filename = prepareFilename(m_config.Get("dacFile",""),i2c); }

  std::vector<std::pair<std::string,uint8_t> > dacs;
  std::ifstream file(filename);
  size_t overwritten_dacs = 0;

  if(!file.fail()) {
    EUDAQ_INFO(string("Reading DAC settings from file \"") + filename + string("\"."));
    std::cout << "Reading DAC settings from file \"" << filename << "\"." << std::endl;

    std::string line;
    while(std::getline(file, line)) {
      std::stringstream   linestream(line);
      std::string         name;
      int                 dummy, value;
      linestream >> dummy >> name >> value;

      // Check if the first part read was really the register:
      if(dummy == 0) {
	// Rereading with only DAC name and value, no register:
	std::stringstream newstream(line);
	newstream >> name >> value;
      }

      // Convert to lower case for cross-checking with eduaq config:
      std::transform(name.begin(), name.end(), name.begin(), ::tolower);

      // Check if reading was correct:
      if(name.empty()) {
	EUDAQ_ERROR(string("Problem reading DACs from file \"") + filename + string("\": DAC name appears to be empty.\n"));
	throw pxar::InvalidConfig("WARNING: Problem reading DACs from file \"" + filename + "\": DAC name appears to be empty.");
      }

      // Skip the current limits that are wrongly placed in the DAC file sometimes (belong to the DTB!)
      if(name == "vd" || name == "va") { continue; }

      // Check if this DAC is overwritten by directly specifying it in the config file:
      if(m_config.Get(name, -1) != -1) {
	std::cout << "Overwriting DAC " << name << " from file: " << value;
	value = m_config.Get(name, -1);
	std::cout << " -> " << value << std::endl;
	overwritten_dacs++;
      }

      dacs.push_back(make_pair(name, value));
      m_alldacs.append(name + " " + std::to_string(value) + "; ");
    }

    EUDAQ_USER(string("Successfully read ") + std::to_string(dacs.size()) 
	       + string(" DACs from file, ") + std::to_string(overwritten_dacs) + string(" overwritten by config."));
  }
  else {
    std::cout << "Could not open DAC file \"" << string(filename) << "\"." << std::endl;
    EUDAQ_ERROR(string("Could not open DAC file \"") + filename + string("\"."));
    EUDAQ_INFO(string("If DACs from configuration should be used, remove dacFile path."));
    throw pxar::InvalidConfig("Could not open DAC file.");
  }

  return dacs;
}

std::vector<pxar::pixelConfig> CMSPixelProducer::GetConfTrimming(int16_t i2c) {

  std::string filename;
  // No I2C address indicator is given, assuming filename is correct "as is":
  if(i2c < 0) { filename = m_config.Get("trimFile", ""); }
  // I2C address is given, appending a "_Cx" with x = I2C:
  else { filename = prepareFilename(m_config.Get("trimFile",""),i2c); }

  std::vector<pxar::pixelConfig> pixels;
  std::ifstream file(filename);

  if(!file.fail()){
    std::string line;
    while(std::getline(file, line))
      {
	std::stringstream   linestream(line);
	std::string         dummy;
	int                 trim, col, row;
	linestream >> trim >> dummy >> col >> row;
	pixels.push_back(pxar::pixelConfig(col,row,trim));
      }
    m_trimmingFromConf = true;
  }
  else{
    std::cout << "Couldn't read trim parameters from \"" << string(filename) << "\". Setting all to 15." << std::endl;
    EUDAQ_WARN(string("Couldn't read trim parameters from \"") + string(filename) + ("\". Setting all to 15.\n"));
    for(int col = 0; col < 52; col++) {
      for(int row = 0; row < 80; row++) {
	pixels.push_back(pxar::pixelConfig(col,row,15));
      }
    }
    m_trimmingFromConf = false;
  }
  if(m_trimmingFromConf) {
    EUDAQ_USER(string("Trimming successfully read from ") + m_config.Name() + string(": \"") + string(filename) + string("\"\n"));
  }
  return pixels;
} // GetConfTrimming
