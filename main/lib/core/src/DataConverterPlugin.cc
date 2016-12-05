#include "eudaq/DataConverterPlugin.hh"

namespace eudaq {
  
  template class DLLEXPORT Factory<DataConverterPlugin>;
  template DLLEXPORT std::map<uint32_t, typename Factory<DataConverterPlugin>::UP_BASE (*)()>&
  Factory<DataConverterPlugin>::Instance<>();
  
  unsigned DataConverterPlugin::GetTriggerID(eudaq::Event const & ev) const {
    return ev.GetEventN();
  }

  DataConverterPlugin::DataConverterPlugin(std::string subtype){
  }

  void DataConverterPlugin::Initialize(eudaq::Event const &, eudaq::Configuration const &){

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


}//namespace eudaq
