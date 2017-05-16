#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"

class ItsAbcRawEvent2StdEventConverter: public eudaq::StdEventConverter{
public:
  bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("ItsAbcRaw");
  static const uint32_t PLANE_ID_OFFSET_ABC = 10;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
    Register<ItsAbcRawEvent2StdEventConverter>(ItsAbcRawEvent2StdEventConverter::m_id_factory);
}

bool ItsAbcRawEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2,
						  eudaq::ConfigSPC conf) const{
  auto raw = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  if (!raw){
    EUDAQ_THROW("dynamic_cast error: from eudaq::Event to eudaq::RawEvent");
  }  
  auto block_n_list = raw->GetBlockNumList();
  if(block_n_list.empty()){
    EUDAQ_WARN("No block in ItsAbcRawEvent EventN:TriggerN (" +
	       std::to_string(raw->GetEventN())+":"+std::to_string(raw->GetTriggerN())+")");
  }
  
  for(auto &block_n: block_n_list){
    std::vector<uint8_t> block = raw->GetBlock(block_n);
    std::vector<bool> channels;
    eudaq::uchar2bool(block.data(), block.data() + block.size(), channels);
    eudaq::StandardPlane plane(block_n + PLANE_ID_OFFSET_ABC, "ITS_ABC", "ABC");
    plane.SetSizeZS(channels.size(), 1, 0);    
    for(size_t i = 0; i < channels.size(); ++i) {
      if(channels[i]){
	plane.PushPixel(i, 1 , 1);
      }
    }
    d2->AddPlane(plane);
  }
  return true;
}
