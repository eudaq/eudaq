#include "DataConverterPlugin.hh"
#include "PluginManager.hh"

#include <iostream>


namespace eudaq {
  
  template class DLLEXPORT Factory<DataConverterPlugin>;
  template DLLEXPORT std::map<uint32_t, typename Factory<DataConverterPlugin>::UP_BASE (*)()>&
  Factory<DataConverterPlugin>::Instance<>();
  
  unsigned DataConverterPlugin::GetTriggerID(eudaq::Event const & ev) const {
    return ev.GetEventN();
  }

  DataConverterPlugin::DataConverterPlugin(std::string subtype) :DataConverterPlugin(Event::str2id("_RAW"), subtype) {
  }

  DataConverterPlugin::DataConverterPlugin(unsigned type, std::string subtype)
    : m_eventtype(make_pair(type, subtype)), m_thisCount(m_count) {
    m_count += 10;
  }

  void DataConverterPlugin::Initialize(eudaq::Event const &, eudaq::Configuration const &) {

  }

  void DataConverterPlugin::GetLCIORunHeader(lcio::LCRunHeader &, eudaq::Event const &, eudaq::Configuration const &) const {

  }

  bool DataConverterPlugin::GetLCIOSubEvent(lcio::LCEvent & /*result*/, eudaq::Event const & /*source*/) const {
    return false;
  }

  bool DataConverterPlugin::GetStandardSubEvent(StandardEvent & /*result*/, eudaq::Event const & /*source*/) const {
    return false;
  }


  EventSP DataConverterPlugin::GetSubEvent(EventSP ev, size_t NumberOfROF) {
    return nullptr;
  }


  size_t DataConverterPlugin::GetNumSubEvent(const eudaq::Event& pac) {
    return 1;
  }

  unsigned DataConverterPlugin::m_count = 0;

}//namespace eudaq
