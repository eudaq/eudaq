#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"
#include "CAEN_v1290_Unpacker.h"

const std::string EVENT_TYPE = "CMSHGCal_DWC_RawEvent";

class CMSHGCAL_DWC_RawEvent2StdEventConverter: public eudaq::StdEventConverter {
public:
  bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("CMSHGCal_DWC_RawEvent");
  CMSHGCAL_DWC_RawEvent2StdEventConverter();
  ~CMSHGCAL_DWC_RawEvent2StdEventConverter();
private:
  CAENv1290Unpacker* tdc_unpacker;
  unsigned NHits_forReadout;      //limit the number of converted hits to the first NHits_forReadout
};

namespace {
auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
              Register<CMSHGCAL_DWC_RawEvent2StdEventConverter>(CMSHGCAL_DWC_RawEvent2StdEventConverter::m_id_factory);
}

CMSHGCAL_DWC_RawEvent2StdEventConverter::CMSHGCAL_DWC_RawEvent2StdEventConverter() : eudaq::StdEventConverter() {
  //values could come from a config...TODO
  tdc_unpacker = new CAENv1290Unpacker(16);
  NHits_forReadout = 20;    
}

CMSHGCAL_DWC_RawEvent2StdEventConverter::~CMSHGCAL_DWC_RawEvent2StdEventConverter() { delete tdc_unpacker;}

bool CMSHGCAL_DWC_RawEvent2StdEventConverter::Converting(eudaq::EventSPC rev, eudaq::StdEventSP sev, eudaq::ConfigSPC conf) const {
  // If the event type is used for different sensors
  // they can be differentiated here
  const std::string sensortype = "DWC";
  
  sev->SetTag("cpuTime_dwcProducer_mus", rev->GetTimestampBegin());

  std::vector<tdcData*> unpacked;
  std::map<std::pair<int, int>, uint32_t> time_of_arrivals;

  const unsigned nBlocks = rev->NumBlocks(); //nBocks = number of TDCs
  
  for (unsigned tdc_index = 0; tdc_index < nBlocks; tdc_index++) {
    std::vector<uint8_t> bl = rev->GetBlock(tdc_index);

    //conversion block
    std::vector<uint32_t> Words;
    Words.resize(bl.size() / sizeof(uint32_t));
    std::memcpy(&Words[0], &bl[0], bl.size());

    unpacked.push_back(tdc_unpacker->ConvertTDCData(Words));
  }


  eudaq::StandardPlane full_dwc_plane(0, EVENT_TYPE, sensortype + "_fullTDC");
  full_dwc_plane.SetSizeRaw(nBlocks, 16, 1 + NHits_forReadout);  //nBlocks = number of TDCs, 16=number of channels in the TDC, one frame

  for (unsigned tdc_index = 0; tdc_index < nBlocks; tdc_index++)for (int channel = 0; channel < 16; channel++) {
      full_dwc_plane.SetPixel(tdc_index * 16 + channel, tdc_index, channel, unpacked[tdc_index]->hits[channel].size(), false, 0);
    }

  for (unsigned tdc_index = 0; tdc_index < nBlocks; tdc_index++)for (int channel = 0; channel < 16; channel++) {
      time_of_arrivals[std::make_pair(tdc_index, channel)] = unpacked[tdc_index]->timeOfArrivals[channel];    //it is peculiar that we need that hack in order to access the time of arrivals in a subsequent step
      for (size_t hit_index = 0; hit_index < unpacked[tdc_index]->hits[channel].size(); hit_index++) {
        if (hit_index >= NHits_forReadout) break;
        full_dwc_plane.SetPixel(tdc_index * 16 + channel, tdc_index, channel, unpacked[tdc_index]->hits[channel][hit_index], false, 1 + hit_index);
      }
    }
  sev->AddPlane(full_dwc_plane);

  //plane with the trigger time stamps
  eudaq::StandardPlane timestamp_dwc_plane(0, EVENT_TYPE, sensortype + "_triggerTimestamps");
  timestamp_dwc_plane.SetSizeRaw(nBlocks, 1, 1);
  for (unsigned tdc_index = 0; tdc_index < nBlocks; tdc_index++)  timestamp_dwc_plane.SetPixel(tdc_index, tdc_index, 1, unpacked[tdc_index]->extended_trigger_timestamp, false, 0);
  sev->AddPlane(timestamp_dwc_plane);


  //Hard coded assignment of channel time stamps to DWC planes for the DQM to guarantee downward compatibility with <=July 2018 DQM
  //note: for the DWC correlation plots, the indexing must start at 0!
  if (nBlocks >= 1) {  //at least one TDC
    eudaq::StandardPlane DWCE_plane(0, EVENT_TYPE, sensortype);
    DWCE_plane.SetSizeRaw(4, 1, 1);
    DWCE_plane.SetPixel(0, 0, 0, time_of_arrivals[std::make_pair(0, 0)]);   //left
    DWCE_plane.SetPixel(1, 0, 0, time_of_arrivals[std::make_pair(0, 1)]);   //right
    DWCE_plane.SetPixel(2, 0, 0, time_of_arrivals[std::make_pair(0, 2)]);   //up
    DWCE_plane.SetPixel(3, 0, 0, time_of_arrivals[std::make_pair(0, 3)]);   //down
    sev->AddPlane(DWCE_plane);

    eudaq::StandardPlane DWCD_plane(1, EVENT_TYPE, sensortype);
    DWCD_plane.SetSizeRaw(4, 1, 1);
    DWCD_plane.SetPixel(0, 0, 0, time_of_arrivals[std::make_pair(0, 4)]);   //left
    DWCD_plane.SetPixel(1, 0, 0, time_of_arrivals[std::make_pair(0, 5)]);   //right
    DWCD_plane.SetPixel(2, 0, 0, time_of_arrivals[std::make_pair(0, 6)]);   //up
    DWCD_plane.SetPixel(3, 0, 0, time_of_arrivals[std::make_pair(0, 7)]);   //down
    sev->AddPlane(DWCD_plane);

    eudaq::StandardPlane DWCA_plane(2, EVENT_TYPE, sensortype);
    DWCA_plane.SetSizeRaw(4, 1, 1);
    DWCA_plane.SetPixel(0, 0, 0, time_of_arrivals[std::make_pair(0, 8)]);   //left
    DWCA_plane.SetPixel(1, 0, 0, time_of_arrivals[std::make_pair(0, 9)]);   //right
    DWCA_plane.SetPixel(2, 0, 0, time_of_arrivals[std::make_pair(0, 10)]);   //up
    DWCA_plane.SetPixel(3, 0, 0, time_of_arrivals[std::make_pair(0, 11)]);   //down
    sev->AddPlane(DWCA_plane);

    eudaq::StandardPlane DWCExt_plane(3, EVENT_TYPE, sensortype);
    DWCExt_plane.SetSizeRaw(4, 1, 1);
    DWCExt_plane.SetPixel(0, 0, 0, time_of_arrivals[std::make_pair(0, 12)]);   //left
    DWCExt_plane.SetPixel(1, 0, 0, time_of_arrivals[std::make_pair(0, 13)]);   //right
    DWCExt_plane.SetPixel(2, 0, 0, time_of_arrivals[std::make_pair(0, 14)]);   //up
    DWCExt_plane.SetPixel(3, 0, 0, time_of_arrivals[std::make_pair(0, 15)]);   //down
    sev->AddPlane(DWCExt_plane);
  }


  //Hard coded assignment of channel time stamps to DWC planes for the DQM to guarantee downward compatibility with <=July 2018 DQM
  //note: for the DWC correlation plots, the indexing must start at 0!
  if (nBlocks >= 2) {  //at least two TDCs
    eudaq::StandardPlane DWCE_plane_2(4, EVENT_TYPE, sensortype);
    DWCE_plane_2.SetSizeRaw(4, 1, 1);
    DWCE_plane_2.SetPixel(0, 0, 0, time_of_arrivals[std::make_pair(1, 0)]);   //left
    DWCE_plane_2.SetPixel(1, 0, 0, time_of_arrivals[std::make_pair(1, 1)]);   //right
    DWCE_plane_2.SetPixel(2, 0, 0, time_of_arrivals[std::make_pair(1, 2)]);   //up
    DWCE_plane_2.SetPixel(3, 0, 0, time_of_arrivals[std::make_pair(1, 3)]);   //down
    sev->AddPlane(DWCE_plane_2);

    eudaq::StandardPlane DWCD_plane_2(5, EVENT_TYPE, sensortype);
    DWCD_plane_2.SetSizeRaw(4, 1, 1);
    DWCD_plane_2.SetPixel(0, 0, 0, time_of_arrivals[std::make_pair(1, 4)]);   //left
    DWCD_plane_2.SetPixel(1, 0, 0, time_of_arrivals[std::make_pair(1, 5)]);   //right
    DWCD_plane_2.SetPixel(2, 0, 0, time_of_arrivals[std::make_pair(1, 6)]);   //up
    DWCD_plane_2.SetPixel(3, 0, 0, time_of_arrivals[std::make_pair(1, 7)]);   //down
    sev->AddPlane(DWCD_plane_2);

    eudaq::StandardPlane DWCA_plane_2(6, EVENT_TYPE, sensortype);
    DWCA_plane_2.SetSizeRaw(4, 1, 1);
    DWCA_plane_2.SetPixel(0, 0, 0, time_of_arrivals[std::make_pair(1, 8)]);   //left
    DWCA_plane_2.SetPixel(1, 0, 0, time_of_arrivals[std::make_pair(1, 9)]);   //right
    DWCA_plane_2.SetPixel(2, 0, 0, time_of_arrivals[std::make_pair(1, 10)]);   //up
    DWCA_plane_2.SetPixel(3, 0, 0, time_of_arrivals[std::make_pair(1, 11)]);   //down
    sev->AddPlane(DWCA_plane_2);

    eudaq::StandardPlane DWCExt_plane_2(7, EVENT_TYPE, sensortype);
    DWCExt_plane_2.SetSizeRaw(4, 1, 1);
    DWCExt_plane_2.SetPixel(0, 0, 0, time_of_arrivals[std::make_pair(1, 12)]);   //left
    DWCExt_plane_2.SetPixel(1, 0, 0, time_of_arrivals[std::make_pair(1, 13)]);   //right
    DWCExt_plane_2.SetPixel(2, 0, 0, time_of_arrivals[std::make_pair(1, 14)]);   //up
    DWCExt_plane_2.SetPixel(3, 0, 0, time_of_arrivals[std::make_pair(1, 15)]);   //down
    sev->AddPlane(DWCExt_plane_2);
  }


  for (size_t i = 0; i < unpacked.size(); i++) delete unpacked[i];
  unpacked.clear();

  // Indicate that data was successfully converted
  return true;

}
