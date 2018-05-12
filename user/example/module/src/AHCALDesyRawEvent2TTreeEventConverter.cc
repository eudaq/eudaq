#include "eudaq/TTreeEventConverter.hh"
#include "eudaq/RawEvent.hh"

class AHCALDesyRawEvent2TTreeEventConverter: public eudaq::TTreeEventConverter{
public:
  bool Converting(eudaq::EventSPC d1, eudaq::TTreeEventSP d2, eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("DesyTableRaw");
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::TTreeEventConverter>::
    Register<AHCALDesyRawEvent2TTreeEventConverter>(AHCALDesyRawEvent2TTreeEventConverter::m_id_factory);
}

bool AHCALDesyRawEvent2TTreeEventConverter::Converting(eudaq::EventSPC d1, eudaq::TTreeEventSP d2, eudaq::ConfigSPC conf) const{
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  uint32_t id = ev->GetExtendWord();
  size_t nblocks= ev->NumBlocks();
  auto block_n_list = ev->GetBlockNumList();
  std:: cout << " blocks " << nblocks << std::endl;
  for(auto &block_n: block_n_list){
    std::vector<uint8_t> block = ev->GetBlock(block_n);
    /* 
       if(block.size() < 2)
       EUDAQ_THROW("Unknown data");
       uint8_t x_pixel = block[0];
       uint8_t y_pixel = block[1];
       std::vector<uint8_t> hit(block.begin()+2, block.end());
       std::vector<uint8_t> hitxv;
       TString temp = "block"; 
       if (d2->GetListOfBranches()->FindObject(temp))  d2->SetBranchAddress(temp,&x_pixel);
       else
       d2->Branch(temp,&x_pixel,"x_pixel/b:y_pixel/b");
    */
  }
  return true;
}
