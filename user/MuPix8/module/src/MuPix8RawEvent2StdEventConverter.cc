#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"

#include "library/telescope_frame.hpp"

class MuPixTypeEvent2StdEventConverter: public eudaq::StdEventConverter{
public:
  bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const override;
};


namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
    Register<MuPixTypeEvent2StdEventConverter>(eudaq::cstr2hash("MuPix8"));

  auto dummy1 = eudaq::Factory<eudaq::StdEventConverter>::
    Register<MuPixTypeEvent2StdEventConverter>(eudaq::cstr2hash("ATLASpix"));
  // add further sensors here
}


bool MuPixTypeEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const{
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);

  int m_cols, m_rows, m_ID;
  // defines the params here:
  uint32_t type = ev->GetType();
  if(type == eudaq::cstr2hash("MuPix8"))
  { m_ID = 8; m_cols = 128; m_rows = 200;}
  if(type == eudaq::cstr2hash("ATLASPix"))
  { m_ID = 66; m_cols = 128; m_rows = 400;}

  size_t nblocks= ev->NumBlocks();
  auto block_n_list = ev->GetBlockNumList();
  for(auto &block_n: block_n_list){
    auto block =  ev->GetBlock(block_n);
    TelescopeFrame * tf = new TelescopeFrame();
    if(!tf->from_uint8_t(ev->GetBlock(block_n)))
        EUDAQ_ERROR("Cannot read TelescopeFrame");


    eudaq::StandardPlane plane(block_n, "MuPixLike_DUT", "MuPixLike_DUT");
    plane.SetSizeZS(m_cols,m_rows,tf->num_hits());
    for(uint i =0; i < tf->num_hits();++i)
    {
        RawHit h = tf->get_hit(i,m_ID);
        plane.SetPixel(i,h.column(),h.row(),h.timestamp_raw());
    }
    d2->AddPlane(plane);
  }
  return true;
}
