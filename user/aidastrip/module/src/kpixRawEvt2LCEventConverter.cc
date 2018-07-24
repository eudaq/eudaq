#include "eudaq/LCEventConverter.hh"
//#include "eudaq/Logger.hh"
#include "eudaq/RawEvent.hh"

class kpixRawEvt2LCEventConverter: public eudaq::LCEventConverter{
public:
  bool Converting(eudaq::EventSPC d1, eudaq::LCEventSP d2, eudaq::ConfigurationSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("kpixRawEvt");
};
  
namespace{
  auto dummy0 = eudaq::Factory<eudaq::LCEventConverter>::
    Register<kpixRawEvt2LCEventConverter>(kpixRawEvt2LCEventConverter::m_id_factory);
}

bool kpixRawEvt2LCEventConverter::Converting(eudaq::EventSPC d1, eudaq::LCEventSP d2, eudaq::ConfigurationSPC conf) const{
  /* auto d2impl = std::dynamic_pointer_cast<lcio::LCEventImpl>(d2);
  d2impl->setTimeStamp(d1->GetTimestampBegin());
  d2impl->setEventNumber(d1->GetEventNumber());
  d2impl->setRunNumber(d1->GetRunNumber());
  */
  auto rawev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  size_t nblocks = rawev->NumBlocks();

  std::cout<<"[kpixRaw->LCIO] nblocks = "<<nblocks <<std::endl;

  //auto block_n_list = rawev->GetBlockNumList();

  
  /*for (auto &block_n:block_n_list){
    std::vector<uint8_t> block = rawev -> GetBlock(block_n);
    std::cout<< "[kpixRaw->LCIO] I have "
	     << block.size()
	     << " blocks\n";
	     }*/

  
  return true;    
}
