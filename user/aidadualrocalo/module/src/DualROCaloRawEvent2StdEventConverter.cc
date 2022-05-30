#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"

class DualROCaloRawEvent2StdEventConverter: public eudaq::StdEventConverter{
public:
  bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("DummyRaw");
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
    Register<DualROCaloRawEvent2StdEventConverter>(DualROCaloRawEvent2StdEventConverter::m_id_factory);
}

bool DualROCaloRawEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const{
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  size_t nblocks= ev->NumBlocks();
  auto block_n_list = ev->GetBlockNumList();
  for(auto &block_n: block_n_list){
    std::vector<uint8_t> block = ev->GetBlock(block_n);
    /*
    uint16_t event_size = uint16_t((uint8_t)block[1] << 8 |
                                   (uint8_t)block[0]);
    if(event_size != block.size())
      EUDAQ_THROW("Unknown data");
    
    uint8_t boardID = uint8_t(block[2]);
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

    std::vector<uint8_t> hits(block.begin()+27, block.end());
    */

    uint8_t data_size = 6;
    /*
    uint16_t expected_size = num_active_channels*data_size;
    if(hits.size() != expected_size)
      EUDAQ_THROW("Unknown data");
      */
    eudaq::StandardPlane plane_lg(block_n, "my_Dummy_lg_plane", "my_Dummy_lg_plane");
    plane_lg.SetSizeZS(block.size(), 1, 0);
    eudaq::StandardPlane plane_hg(block_n, "my_Dummy_hg_plane", "my_Dummy_hg_plane");
    plane_hg.SetSizeZS(block.size(), 1, 0);
    for(size_t n = 0; n < block.size(); ++n){
      uint16_t index = n*data_size;

      uint8_t channel_id = uint8_t(block[index]);

      uint16_t lg_adc_value;
      memcpy(&lg_adc_value, &block[index+2], sizeof(uint16_t));

      uint16_t hg_adc_value;
      memcpy(&hg_adc_value, &block[index+4], sizeof(uint16_t));

	    plane_lg.PushPixel(n, 1, lg_adc_value);
      plane_hg.PushPixel(n, 1, hg_adc_value);
    }
    d2->AddPlane(plane_lg);
    d2->AddPlane(plane_hg);
  }
  return true;
}