#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"

/*
class DualROCaloRawEvent2StdEventConverter: public eudaq::StdEventConverter{
public:
  bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("DualROCaloEvent");
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
    Register<DualROCaloRawEvent2StdEventConverter>(DualROCaloRawEvent2StdEventConverter::m_id_factory);
}

bool DualROCaloRawEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const{
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  if(!ev){
    std::cout << "Converting:: No event!" << std::endl;
    return false;
  }
  size_t nblocks= ev->NumBlocks();
  auto block_n_list = ev->GetBlockNumList();

  d2->SetDetectorType("DualROCalo");

  
  if(!d2->IsFlagPacket()){
    //d2->SetFlag(d1->GetFlag());
    //d2->SetRunN(d1->GetRunN());
    //d2->SetEventN(d1->GetEventN());
    //d2->SetStreamN(d1->GetStreamN());
    d2->SetTriggerN(d1->GetTriggerN(), d1->IsFlagTrigger());
    //d2->SetTimestamp(d1->GetTimestampBegin(), d1->GetTimestampEnd(), d1->IsFlagTimestamp());
  }
  

  // std::cout<<"Converting:: Getting "<< std::to_string(nblocks) << " Blocks!" << std::endl;
  // std::cout<<"Converting:: List is  "<< std::to_string(block_n_list.size()) << " long!" << std::endl;
  // std::cout<<"Converting:: Event with trigger number = " << std::to_string(ev->GetTriggerN()) << std::endl;
  // std::cout<<"Converting:: Event with time stamp = " << std::to_string(ev->GetTimestampBegin()) << std::endl;
  
  for(auto &block_n: block_n_list){
    //std::cout<<"Converting:: Getting new block with index = " << std::to_string(block_n) << std::endl;
    std::vector<uint8_t> block = ev->GetBlock(block_n);
    //std::cout<<"Converting:: Actually got new block!" << std::endl;
    
    /*
    uint16_t event_size = uint16_t((uint8_t)block[1] << 8 |
                                   (uint8_t)block[0]);
    std::cout<<"Converting:: Accessing new block!" << std::endl;
    if(event_size != block.size())
      EUDAQ_THROW("Unknown data");
    * /
    uint8_t boardID = uint8_t(block[2]);
    //std::cout<<"Converting:: Accessing new block!" << std::endl;
    //std::cout<<"Converting:: block size is = " << std::to_string(block.size()) << std::endl;
    if (block.size()==0){
      std::cout << "EMPTY BLOCK! " << std::endl;
      continue;
    }
    /*
    double trigger_time_stamp;
    memcpy(&trigger_time_stamp, &block[3], sizeof(double));

    uint64_t trigger_id;
    memcpy(&trigger_id, &block[11], sizeof(uint64_t));

    std::vector<uint8_t> channel_mask(&block[19],&block[27]);
    uint8_t num_channels = sizeof(channel_mask)*8;
    unsigned char channel_mask_bits[num_channels];
    int b;
    uint8_t num_active_channels = 0;
    for (b=0; b<num_channels; b++) {
      channel_mask_bits[b] = ((1 << (b % 8)) & (channel_mask[b/8])) >> (b % 8);
      if(channel_mask_bits[b] == 1) num_active_channels++;
    }
    * /
    //std::cout<<"Converting:: Decoded new block!" << std::endl;

    std::vector<uint8_t> hits(block.begin(), block.end());
    

    uint8_t data_size = 6;
    /*
    uint16_t expected_size = num_active_channels*data_size;
    if(hits.size() != expected_size)
      EUDAQ_THROW("Unknown data");
      * /

    //std::cout<<"Converting:: Creating Planes!" << std::endl;
    eudaq::StandardPlane plane_lg(block_n, "DualROCalo", "DualROCalo");
    plane_lg.SetSizeZS(8, 8, 64);
    //eudaq::StandardPlane plane_hg(block_n, "my_Dummy_hg_plane", "my_Dummy_hg_plane");
    //plane_hg.SetSizeZS(8, 8, 64);
    //std::cout<<"Converting:: Created Planes!" << std::endl;
    for(size_t n = 0; n < 64; ++n){
      uint16_t index = n*data_size;

      uint8_t channel_id = uint8_t(block[index]);

      uint16_t lg_adc_value;
      memcpy(&lg_adc_value, &block[index+2], sizeof(uint16_t));

      uint16_t hg_adc_value;
      memcpy(&hg_adc_value, &block[index+4], sizeof(uint16_t));

      //std::cout<<"Converting:: Added Pixels with lg = " << std::to_string(lg_adc_value) <<  std::endl;
      //std::cout<<"Converting:: Added Pixels with hg = " << std::to_string(hg_adc_value) <<  std::endl;

      uint32_t x = n % 8;
      uint32_t y = (uint32_t) n/8;

      //std::cout<<"Converting:: n = " << std::to_string(n)<<" x = " << std::to_string(x)<<" y = " << std::to_string(y) << " adc = " << std::to_string(lg_adc_value) <<  std::endl;

	    plane_lg.PushPixel(x, y, 0);
      //plane_hg.PushPixel(n, 1, hg_adc_value);
    }
    d2->AddPlane(plane_lg);
    //d2->AddPlane(plane_hg);
    //std::cout<<"Converting:: Added Planes!" << std::endl;
  }

  
  return true;
}
*/

class DualROCaloRawEvent2StdEventConverter: public eudaq::StdEventConverter{
  typedef std::vector<uint8_t>::const_iterator datait;
public:
  bool Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("DualROCaloEvent");
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
    Register<DualROCaloRawEvent2StdEventConverter>(DualROCaloRawEvent2StdEventConverter::m_id_factory);
}

bool DualROCaloRawEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const{

  //TODO: number of telescope plane may be less than 6. Decode additional tags
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  if(!ev)
    return false;

    // Identify the detetor type
  d2->SetDetectorType("DualROCalo");


  //d2->SetTimeBegin(d1->GetTimestampBegin());
  //d2->SetTimeEnd(d1->GetTimestampEnd());

  if(!d2->IsFlagPacket()){
    d2->SetFlag(d1->GetFlag());
    d2->SetRunN(d1->GetRunN());
    d2->SetEventN(d1->GetEventN());
    d2->SetStreamN(d1->GetStreamN());
    d2->SetTriggerN(d1->GetTriggerN(), d1->IsFlagTrigger());
    d2->SetTimestamp(d1->GetTimestampBegin(), d1->GetTimestampEnd(), d1->IsFlagTimestamp());
  }

  auto &rawev = *ev;
  /*if (rawev.NumBlocks() < 2 || rawev.GetBlock(0).size() < 20 ||
      rawev.GetBlock(1).size() < 20) {
    EUDAQ_WARN("Ignoring bad event " + std::to_string(rawev.GetEventNumber()));
    return false;
  }*/
  //auto use_all_hits = (conf != nullptr ? bool(conf->Get("use_all_hits",0)) : false);

  const std::vector<uint8_t> &data0 = rawev.GetBlock(0);
  //uint32_t header0 = eudaq::getlittleendian<uint32_t>(&data0[0]);
  //uint32_t header1 = eudaq::getlittleendian<uint32_t>(&data1[0]);
  //uint16_t pivot = eudaq::getlittleendian<uint16_t>(&data0[4]);
  //uint16_t tluid = eudaq::getlittleendian<uint16_t>(&data0[6]);
  datait it0 = data0.begin();
  //uint32_t board = 0;
  eudaq::StandardPlane plane(0, "DualROCalo", "DualROCalo");
  plane.SetSizeZS(8, 8, 0);

  while (it0 < data0.end()) {
    
    uint8_t channel_id = eudaq::getlittleendian<uint8_t>(&(*(it0)));
    uint16_t lg_adc_value = eudaq::getlittleendian<uint16_t>(&(*(it0+2)));
    uint16_t hg_adc_value = eudaq::getlittleendian<uint16_t>(&(*(it0+4)));

    uint8_t n = channel_id;
    uint16_t x = n % 8;
    uint16_t y = (uint16_t) n/8;

    //std::cout << "Converting:: lg value = " << std::to_string(lg_adc_value) << std::endl;
    std::cout<<"Converting:: n = " << std::to_string(n)<<" x = " << std::to_string(x)<<" y = " << std::to_string(y) << " adc = " << std::to_string(lg_adc_value) <<  std::endl;


    plane.PushPixel(x, y, lg_adc_value);

    it0 += 6;
    
    //plane.SetPivotPixel((9216 + pivot + PIVOTPIXELOFFSET) % 9216);
    //DecodeFrame(plane, 0, &it0[8], len0, use_all_hits);
    //DecodeFrame(plane, 1, &it1[8], len1, use_all_hits);
    


  }

  d2->AddPlane(plane);
  std::cout << "Added Plane!" << std::endl;
  //eudaq::mSleep(1000);

  return true;
}

