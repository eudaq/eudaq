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

  //Get first SubEvent of the full Event
  auto cFirstSubEventRaw = std::dynamic_pointer_cast<const eudaq::RawEvent>(pEvent->GetSubEvent(0));
  //Create Standard Plane container 
  std::vector<eudaq::StandardPlane*> cPlanes;  
  //loop over First SubEvent blocks and fill Standard Plane container
  //Get number of SubEvent data blocks
  uint16_t cNFrames = pEvent->GetSubEvents().size();
  uint8_t cNSensors = cFirstSubEventRaw->GetBlockNumList().size();
  for(uint32_t cSensorId = 0; cSensorId < cNSensors; cSensorId++){
    auto *cPlane = new eudaq::StandardPlane(cSensorId+30, "CMSPhase2StdEvent", "CMSPhase2" ); 
    //Extract sensor matrix dimensions
    auto cFirstSubEventRawData = cFirstSubEventRaw->GetBlock(cSensorId);
    uint16_t cNRows = (cFirstSubEventRawData[1] << 8) | (cFirstSubEventRawData[0] << 0);
    uint16_t cNColumns = (cFirstSubEventRawData[3] << 8) | (cFirstSubEventRawData[2] << 0);
    //Set Standard Plane dimensions
    cPlane->SetSizeZS(cNRows, cNColumns, 0, cNFrames); // 3 values per hit, 2 uint8t words per uint16t word, 1 for header
    //Push Standard Plane in container
    cPlanes.push_back(cPlane);
  }//end of loop over First SubEvent blocks
  
  //Loop of over SubEvents (frames) and fill Standard Plane data
  for(uint32_t cFrameId=0; cFrameId < cNFrames ; cFrameId++){
    auto cSubEventRaw = std::dynamic_pointer_cast<const eudaq::RawEvent>(pEvent->GetSubEvent(cFrameId)); 
    for(uint32_t cSensorId = 0; cSensorId < cNSensors; cSensorId++){
      AddFrameToPlane(cPlanes.at(cSensorId), cSubEventRaw->GetBlock(cSensorId), cFrameId, cNFrames ); 
    }//end of loop over SubEvents blocks
  }//end of loop over SubEvents (frames)

  //Add planes to StandardEvent
  for(auto cPlane : cPlanes) pStdEvent->AddPlane(*cPlane); 

  return true;
}

void CMSPhase2RawEvent2StdEventConverter::AddFrameToPlane(eudaq::StandardPlane *pPlane, const std::vector<uint8_t> &pData, uint32_t pFrameId) const
{
  //Get number of hits 
  //#FIXME still need to decide which way to pick but should be the same
  //uint16_t cNHits = pData.size()/6 - 1;
  uint16_t cNHits = ((uint16_t)pData[5] << 8) | ((uint16_t)pData[4] << 0);
  // process data
  for(size_t cHitId = 0; cHitId < cNHits; cHitId++)
  {    
    // column, row, tot, ?, lvl1
    uint16_t cHitRow = getHitVal(pData, cHitId, 0);
    uint16_t cHitColumn = getHitVal(pData, cHitId, 1);
    uint16_t cHitToT = getHitVal(pData, cHitId, 2);
    pPlane->PushPixel(cHitRow, cHitColumn, cHitToT, false, pFrameId);  
  }
}

uint16_t CMSPhase2RawEvent2StdEventConverter::getHitVal(const std::vector<uint8_t>& pData, size_t pHitId, size_t pValueId) const
{
  uint8_t cHeaderShift = 6;
  uint16_t cHitData = cHeaderShift + (pHitId * 6) + (pValueId * 2);
  return (((uint16_t)pData[cHitData + 1] << 8) | ((uint16_t)pData[cHitData] << 0));
}
