#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/PluginManager.hh"

#include <iostream>

namespace eudaq {

  unsigned DataConverterPlugin::GetTriggerID(eudaq::Event const &) const {
    return (unsigned)-1;
  }

  DataConverterPlugin::DataConverterPlugin(std::string subtype) :DataConverterPlugin(Event::str2id("_RAW"),subtype)
  {
    //std::cout << "DEBUG: Registering DataConverterPlugin: " << Event::id2str(m_eventtype.first) << ":" << m_eventtype.second << std::endl;

  }

  DataConverterPlugin::DataConverterPlugin(unsigned type, std::string subtype)
	  : m_eventtype(make_pair(type, subtype)), m_thisCount(m_count)
  {
  //  std::cout << "DEBUG: Registering DataConverterPlugin: " << Event::id2str(m_eventtype.first) << ":" << m_eventtype.second << " unique identifier "<< m_thisCount << " number of instances " <<m_count << std::endl;
    PluginManager::GetInstance().RegisterPlugin(this);
	m_count+=10;
  }
  unsigned DataConverterPlugin::m_count = 0;
}//namespace eudaq
