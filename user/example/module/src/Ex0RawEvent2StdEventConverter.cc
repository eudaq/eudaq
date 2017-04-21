#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawDataEvent.hh"

class Ex0RawEvent2StdEventConverter: public eudaq::StdEventConverter{
public:
  bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("my_Ex0");
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
    Register<Ex0RawEvent2StdEventConverter>(Ex0RawEvent2StdEventConverter::m_id_factory);
}

bool Ex0RawEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const{
  auto ev = std::dynamic_pointer_cast<const eudaq::RawDataEvent>(d1);
  size_t nblocks= ev->NumBlocks();
  auto block_n_list = ev->GetBlockNumList();
  for(auto &block_n: block_n_list){
    std::vector<uint8_t> block = ev->GetBlock(block_n);
    std::vector<bool> channels;
    eudaq::uchar2bool(block.data(), block.data() + block.size(), channels);
    eudaq::StandardPlane plane(block_n, "my_ex0_plane", "my_ex0_plane");
    plane.SetSizeZS(channels.size(), 1, 0);
    uint32_t x = 0;
    for(size_t i = 0; i < channels.size(); ++i) {
      if(channels[i] == true) {
	plane.PushPixel(x, 1 , 1);
      }
      ++x;
    }
    d2->AddPlane(plane);
  }
  return true;
}
