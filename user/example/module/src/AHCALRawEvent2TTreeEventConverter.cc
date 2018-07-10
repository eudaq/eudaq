#include "eudaq/TTreeEventConverter.hh"
#include "eudaq/RawEvent.hh"

class AHCALRawEvent2TTreeEventConverter: public eudaq::TTreeEventConverter{
public:
  bool Converting(eudaq::EventSPC d1, eudaq::TTreeEventSP d2, eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("CaliceObject");
 };

namespace{
  auto dummy0 = eudaq::Factory<eudaq::TTreeEventConverter>::
    Register<AHCALRawEvent2TTreeEventConverter>(AHCALRawEvent2TTreeEventConverter::m_id_factory);
}

bool AHCALRawEvent2TTreeEventConverter::Converting(eudaq::EventSPC d1, eudaq::TTreeEventSP d2, eudaq::ConfigSPC conf) const{
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  uint32_t id = ev->GetExtendWord();
  size_t nblocks= ev->NumBlocks();
  auto block_n_list = ev->GetBlockNumList();
  //  std:: cout << "blocks " << nblocks << std::endl;
  for(auto &block_n: block_n_list){
    std::vector<uint8_t> block = ev->GetBlock(block_n);
    //    if(block.size() < 2)      EUDAQ_THROW("Unknown data");
    if(block.size() > 2)  {  
      uint8_t x_pixel = block[0];
      uint8_t y_pixel = block[1];
      uint8_t z_pixel = block[2];
      uint8_t a_pixel = block[3];
      uint8_t b_pixel = block[4];
      uint8_t c_pixel = block[5];
      TString temp = "block";
      TString tempz = "blockz";
      TString tempb = "blockb"; 
      if (d2->GetListOfBranches()->FindObject(temp))  d2->SetBranchAddress(temp,&x_pixel);
      else
      d2->Branch(temp,&x_pixel,"x_pixel/b:y_pixel/b");
      if (d2->GetListOfBranches()->FindObject(tempz))  d2->SetBranchAddress(tempz,&z_pixel);
    else
      d2->Branch(tempz,&z_pixel,"z_pixel/b:a_pixel/b");
    if (d2->GetListOfBranches()->FindObject(tempb))  d2->SetBranchAddress(tempb,&b_pixel);
    else
      d2->Branch(tempb,&b_pixel,"b_pixel/b:c_pixel/b");
    }  else { 
      //      std::cout << " Can't convert a block of Size " << block.size() << std::endl;
    }

  }
  return true;
  }
