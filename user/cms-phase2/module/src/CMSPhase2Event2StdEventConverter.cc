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

  pStdEvent->SetTriggerN(pEvent->GetSubEvent(0)->GetTriggerN());
  //Get first SubEvent of the full Event
  auto cFirstSubEventRaw = std::dynamic_pointer_cast<const eudaq::RawEvent>(pEvent->GetSubEvent(0));
  //Create Standard Plane container 
  std::vector<eudaq::StandardPlane*> cPlanes;  
  //loop over First SubEvent blocks and fill Standard Plane container
  //Get number of SubEvent data blocks
  uint16_t cNFrames = pEvent->GetSubEvents().size();
  uint8_t cNSensors = cFirstSubEventRaw->GetBlockNumList().size();
  uint16_t cNStubRows, cNStubColumns;
  for(uint32_t cSensorId = 0; cSensorId < cNSensors; cSensorId++){
    auto *cHitPlane = new eudaq::StandardPlane(cSensorId+30, "CMSPhase2StdEvent", "CMSPhase2" ); 
    //Extract sensor matrix dimensions
    auto cFirstSubEventRawData = cFirstSubEventRaw->GetBlock(cSensorId);
    uint16_t cNHitColumns = (cFirstSubEventRawData[1] << 8) | (cFirstSubEventRawData[0] << 0);
    uint16_t cNHitRows = (cFirstSubEventRawData[3] << 8) | (cFirstSubEventRawData[2] << 0);
/*
    std::cout << "NHitRows : " << +cNHitRows << std::endl;
    std::cout << "NHitCol : " << +cNHitColumns << std::endl;
*/

    //Set Standard Plane dimensions
    cHitPlane->SetSizeZS(cNHitColumns, cNHitRows, 0, cNFrames); // 3 values per hit, 2 uint8t words per uint16t word, 1 for header
    //Push Standard Plane in container
    cPlanes.push_back(cHitPlane);

    //we'll assume that Sensor 0 is always the seed layer and adapt for it in the producer
    //set dimension of Stub plane for correlation    
    if(cSensorId == 0)
    {
      cNStubColumns = cNHitColumns;
      cNStubRows = cNHitRows;
    }
  }//end of loop over First SubEvent blocks

  //Stub seed plane for stub correlation
  auto *cStubSeedPlane = new eudaq::StandardPlane(40, "CMSPhase2StdEvent", "CMSPhase2");
  cStubSeedPlane->SetSizeZS(cNStubColumns, cNStubRows, 0, cNFrames);
  cPlanes.push_back(cStubSeedPlane);
  
  //Loop of over SubEvents (frames) and fill Standard Plane data
  for(uint32_t cFrameId=0; cFrameId < cNFrames ; cFrameId++){
    auto cSubEventRaw = std::dynamic_pointer_cast<const eudaq::RawEvent>(pEvent->GetSubEvent(cFrameId)); 
    for(uint32_t cSensorId = 0; cSensorId < cNSensors; cSensorId++){
      //std::cout << "SensorId      : " << +cSensorId << std::endl;
      AddFrameToPlane(cPlanes.at(cSensorId), cSubEventRaw->GetBlock(cSensorId), cFrameId); 
    }//end of loop over SubEvents blocks

    //Adding tags to the standard event
    const std::map<std::string, std::string> cRawTags = cSubEventRaw->GetTags();
    for(auto item : cRawTags) {
      pStdEvent->SetTag(item.first, item.second);
    }
/*
    //Extract stubs from tags, convert and fill stub seed plane
    auto cSubEventTags = cSubEventRaw->GetTags();
    for(auto cTag : cSubEventTags)
    {
      //use stub position tag as an access point for all the other stub information
      if(cTag.first.find("stub_pos") != std::string::npos)
      {
        std::cout << "found stub position" << std::endl;
        //split the tag name by underscore
        std::vector<std::string> cTagNameSplit;
	boost::split(cTagNameSplit, cTag.first, [](char c){ return c = '_';});
        std::cout << "splitting name" << std::endl;
        std::cout << "getting hybrid id" << std::endl;
	uint8_t cHybridId = std::stoi(cTagNameSplit[2]);
        std::cout << "getting chip id" << std::endl;
        uint8_t cChipId = std::stoi(cTagNameSplit[3]);
        std::cout << "getting stub id" << std::endl;
        uint8_t cStubId = std::stoi(cTagNameSplit[4]);
        //get stub position
        std::cout << "looking for stub position" << std::endl;
        uint16_t cStubPosition = std::stoi(cTag.second);
        //get stub bend
        std::cout << "looking for stub bend" << std::endl;
        char cStubBendTagName[100];
        std::sprintf(cStubBendTagName, "stub_bend_%02d_%02d_%02d", cHybridId, cChipId, cStubId);
        uint16_t cStubBend = std::stoi(cSubEventTags[cStubBendTagName]);
        std::cout << "found stub bend" << std::endl;
        //get stub row
        std::cout << "looking for stub row" << std::endl;
        char cStubRowTagName[100];
        std::sprintf(cStubRowTagName, "stub_row_%02d_%02d_%02d", cHybridId, cChipId, cStubId);
        uint16_t cStubRow = std::stoi(cSubEventTags[cStubRowTagName]);
        std::cout << "found stub row" << std::endl;
        //get stub center
        std::cout << "looking for stub center" << std::endl;
        char cStubCenterTagName[100];
        std::sprintf(cStubCenterTagName, "stub_center_%02d_%02d_%02d", cHybridId, cChipId, cStubId);
        uint16_t cStubCenter = std::stoi(cSubEventTags[cStubCenterTagName]);
        std::cout << "found stub row" << std::endl;

        //Compute global position
        uint16_t cSeedColumn = (cHybridId % 2 == 0) ? (cNStubColumns - cStubPosition) : (cStubPosition - 1);
        uint16_t cSeedRow = (cHybridId % 2 != 0) ? (cNStubRows - cStubRow) : (cStubRow - 1);
        cStubSeedPlane->PushPixel(cSeedColumn, cSeedRow, 1, false, cFrameId);
        std::cout << "pushed stub" << std::endl;
      }
    }
    */
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
  for(uint16_t cHitId = 0; cHitId < cNHits; cHitId++)
  {    
    // column, row, tot, ?, lvl1
    uint16_t cHitColumn = getHitVal(pData, cHitId, 0);
    uint16_t cHitRow = getHitVal(pData, cHitId, 1);
    uint16_t cHitToT = getHitVal(pData, cHitId, 2);

    /*
    std::cout << "Hit col : " << +cHitColumn << std::endl;
    std::cout << "Hit row : " << +cHitRow << std::endl;
    std::cout << "Hit tot : " << +cHitToT << std::endl;
    std::cout << " ------ " << std::endl;
    */

    pPlane->PushPixel(cHitColumn, cHitRow, cHitToT, false, pFrameId);  
  }
}

uint16_t CMSPhase2RawEvent2StdEventConverter::getHitVal(const std::vector<uint8_t>& pData, size_t pHitId, size_t pValueId) const
{
  uint16_t cHeaderShift = 6;
  uint16_t cHitData = cHeaderShift + (pHitId * 6) + (pValueId * 2);
  return ((uint16_t)(pData[cHitData + 1] << 8) | (uint16_t)(pData[cHitData] << 0));
}
