#include "CMSPixelProducer.hh"

#include "dictionaries.h"
#include "eudaq/Logger.hh"

#include <iostream>
#include <ostream>
#include <vector>

using namespace pxar; 
using namespace std; 

std::vector<std::pair<std::string,uint8_t> > CMSPixelProducer::GetConfDACs()
{
  std::vector<std::pair<std::string,uint8_t> >  dacs; 
  RegisterDictionary * dict = RegisterDictionary::getInstance();
  std::vector<std::string> DACnames =  dict -> getAllROCNames();
  std::cout << " looping over DAC names \n ";
  m_dacsFromConf = true;

  for (std::vector<std::string>::iterator idac = DACnames.begin(); idac != DACnames.end(); ++idac) {
    if(m_config.Get(*idac, -1) == -1){
      EUDAQ_EXTRA(string("Roc DAC ") + *idac + string(" not defined in config file ") + m_config.Name() +
		 string(". Using default values.\n"));
      std::cout << "WARNING: Roc DAC " << *idac << " not defined in config file " << m_config.Name() 
		<< ". Using default values." << std::endl;
      m_dacsFromConf = false;
    }
    else{
      uint8_t dacVal = m_config.Get(*idac, 0);
      std::cout << *idac << " " << (int)dacVal << std::endl;
      dacs.push_back(make_pair(*idac, dacVal));
    }
  }
  if(m_dacsFromConf) {
    EUDAQ_USER(string("All DACs successfully read from ") + m_config.Name());
  }
  return dacs;
}

std::vector<pxar::pixelConfig> CMSPixelProducer::GetConfTrimming(std::string filename)
{
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
    std::cout << "Couldn't read trim parameters from " << string(filename) << ". Setting all to 15." << std::endl;
    EUDAQ_WARN(string("Couldn't read trim parameters from ") + string(filename) + (". Setting all to 15.\n"));
    for(int col = 0; col < 52; col++) {
      for(int row = 0; row < 80; row++) {
	pixels.push_back(pxar::pixelConfig(col,row,15));
      }
    }
    m_trimmingFromConf = false;
  }
  if(m_trimmingFromConf) {
    EUDAQ_USER(string("Trimming successfully read from ") + m_config.Name() + string(": ") + string(filename) + string("\n"));
  }
  return pixels;
} // GetConfTrimming
