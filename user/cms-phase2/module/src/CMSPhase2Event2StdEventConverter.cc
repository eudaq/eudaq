#include "CMSPhase2Event2StdEventConverter.hh"

using namespace eudaq;

namespace {
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
  Register<CMSPhase2RawEvent2StdEventConverter>(CMSPhase2RawEvent2StdEventConverter::m_id_factory);
}

bool CMSPhase2RawEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const
{

  // No event
  if(!d1 || d1->NumBlocks() < 1) {
    return false;
  }

  auto ev_raw = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  auto block_n_list = ev_raw->GetBlockNumList();
  for(auto &bn: block_n_list){
    d2->AddPlane(ConvertPlane(ev_raw->GetBlock(bn), bn+30));// for phase2 lets keep offset 30
  }
  return true;
}

eudaq::StandardPlane CMSPhase2RawEvent2StdEventConverter::ConvertPlane(const std::vector<uint8_t> & data, uint32_t id) const
{
  // create event
  eudaq::StandardPlane plane(id, "CMSPhase2StdEvent", "CMSPhase2");

  // get width and height
  uint32_t width = (((uint32_t)data[1]) << 8) | (uint32_t)data[0];
  uint32_t height = (((uint32_t)data[3]) << 8) | (uint32_t)data[2];

  // set size
  uint32_t nhits = data.size()/6 - 1;
  plane.SetSizeZS(width, height, nhits); // 3 values per hit, 2 uint8t words per uint16t word, 1 for header

  // process data
  for(size_t i = 0; i < nhits; i++){    
    // column, row, tot, ?, lvl1
    plane.PushPixel(getHitVal(data, i, 0), getHitVal(data, i, 1), getHitVal(data, i, 2), false, 0);  
  }

  // return
  return plane;
}

uint16_t CMSPhase2RawEvent2StdEventConverter::getHitVal(const std::vector<uint8_t>& data, size_t hit_id, size_t value_id) const
{
  uint32_t word_index = 6 + hit_id*6 + value_id*2;
  return (((uint16_t)data[word_index + 1]) << 8) | (uint16_t)data[word_index];
}
