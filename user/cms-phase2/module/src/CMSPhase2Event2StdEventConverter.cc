#include "CMSPhase2Event2StdEventConverter.hh"

using namespace eudaq;

namespace {
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
  Register<CMSPhase2RawEvent2StdEventConverter>(CMSPhase2RawEvent2StdEventConverter::m_id_factory);
}

bool CMSPhase2RawEvent2StdEventConverter::Converting(eudaq::EventSPC pEvent, eudaq::StandardEventSP pStdEvent, eudaq::ConfigurationSPC conf) const
{

  // No event
  if(!pEvent || pEvent->GetNumSubEvent() < 1) {
    return false;
  }
  //Create one StandardPlane for each block of data
  auto cSubEventRaw = std::dynamic_pointer_cast<const eudaq::RawEvent>(pEvent->GetSubEvent(0));
  
  std::vector<eudaq::StandardPlane*> cPlanes;  
  uint32_t  cNFrames = pEvent->GetSubEvents().size();
  for(uint32_t cBlockId = 0; cBlockId<cSubEventRaw->GetBlockNumList().size(); cBlockId++){
    eudaq::StandardPlane *cPlane = new eudaq::StandardPlane(cBlockId+30, "CMSPhase2StdEvent", "CMSPhase2" ); 
    cPlane->SetSizeZS(1016, 2, 0, cNFrames); // 3 values per hit, 2 uint8t words per uint16t word, 1 for header
    cPlanes.push_back(cPlane);
  }
  for(uint32_t cFrameId=0; cFrameId<cNFrames ; cFrameId++){
    cSubEventRaw = std::dynamic_pointer_cast<const eudaq::RawEvent>(pEvent->GetSubEvent(cFrameId));

    // Ste trigger ID tag is only set on the subevents containing the different trigger multiplicities
    pStdEvent->SetTriggerN(std::stol(cSubEventRaw->GetTag("TLU_TRIGGER_ID", "0")));

    for(uint32_t cBlockId=0; cBlockId < cSubEventRaw->GetBlockNumList().size(); cBlockId++){
      AddFrameToPlane(cPlanes.at(cBlockId), cSubEventRaw->GetBlock(cBlockId), cFrameId, cNFrames ); 
    }
  }
  for(auto cPlane : cPlanes ){
    pStdEvent->AddPlane(*cPlane);
  } 

  return true;
}

void CMSPhase2RawEvent2StdEventConverter::AddFrameToPlane(eudaq::StandardPlane *pPlane, const std::vector<uint8_t> &data, uint32_t pFrame, uint32_t pNFrames) const{

  // get width and height
  uint32_t width = (((uint32_t)data[1]) << 8) | (uint32_t)data[0];
  uint32_t height = (((uint32_t)data[3]) << 8) | (uint32_t)data[2];

  // set size
  uint32_t nhits = data.size()/6 - 1;

  // process data
  for(size_t i = 0; i < nhits; i++){    
    // column, row, tot, ?, lvl1
    pPlane->PushPixel(getHitVal(data, i, 0), getHitVal(data, i, 1), getHitVal(data, i, 2), false, pFrame);  
  }
}

uint16_t CMSPhase2RawEvent2StdEventConverter::getHitVal(const std::vector<uint8_t>& data, size_t hit_id, size_t value_id) const
{
  uint32_t word_index = 6 + hit_id*6 + value_id*2;
  return (((uint16_t)data[word_index + 1]) << 8) | (uint16_t)data[word_index];
}
