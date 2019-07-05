#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"

#include "library/telescope_frame.hpp"

class MuPix8RawEvent2StdEventConverter: public eudaq::StdEventConverter{
public:
  bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("MuPix8");
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
    Register<MuPix8RawEvent2StdEventConverter>(MuPix8RawEvent2StdEventConverter::m_id_factory);
}

bool MuPix8RawEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const{
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);

  auto block_n_list = ev->GetBlockNumList();
  std::vector<std::pair<RawHit, double>> hits;
  for(auto &block_n: block_n_list){
    auto block =  ev->GetBlock(block_n);
    TelescopeFrame * tf = new TelescopeFrame();
    if(!tf->from_uint8_t(ev->GetBlock(block_n)))
        EUDAQ_ERROR("Cannot read TelescopeFrame");
    for(uint i =0; i < tf->num_hits();++i)
    {
        RawHit h = tf->get_hit(i,8);
        hits.push_back(std::pair<RawHit,double>(h,double(((tf->timestamp()>>2) & 0xFFFFFC00)+h.timestamp_raw()*8)));// need the timestamp in ns
    }
  }
  eudaq::StandardPlane plane(81, "MuPixLike_DUT", "MuPixLike_DUT");
  plane.SetSizeZS(128,200,uint32_t(hits.size()));

  int i =0;
  for(auto hit : hits)
  {
      plane.SetPixel(uint32_t(i),uint32_t(hit.first.column()),uint32_t(hit.first.row()),hit.second);
      i++;
  }
  d2->AddPlane(plane);
  return true;
}
