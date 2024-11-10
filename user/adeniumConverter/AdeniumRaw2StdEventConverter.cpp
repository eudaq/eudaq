#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"
#include "eudaq/Logger.hh"

class AdeniumRawEvent2StdEventConverter: public eudaq::StdEventConverter{
public:
  bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("AdeniumRawDataEvent");
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
    Register<AdeniumRawEvent2StdEventConverter>(AdeniumRawEvent2StdEventConverter::m_id_factory);
}

bool AdeniumRawEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const{
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  size_t nblocks= ev->NumBlocks();
  auto block_n_list = ev->GetBlockNumList();
  if(nblocks!=1){
      EUDAQ_ERROR("Wrong number of blocks");
  }
  auto block =  ev->GetBlock(0); // this are always 8 bits
  //std::cout << "New event: ********************** "<< d1->GetTriggerN() << std::endl;
  for(uint bit = 0; bit < block.size();bit++){
    auto  word = eudaq::getlittleendian<uint8_t>(&block[0]+bit);
    // first entry == plane id
    auto planeID = word;
    bit++;
    word = eudaq::getlittleendian<uint8_t>(&block[0]+bit);
    uint16_t numHits = 0;
    if(word>>7==1){
    numHits = word <<0x8;
    bit++;
    word = eudaq::getlittleendian<uint8_t>(&block[0]+bit);
    numHits+=word;
    }
    else{
         numHits = word;
    }

    eudaq::StandardPlane plane(planeID, "Adenium", "Adenium");
    plane.SetSizeZS(1024,512,numHits);
    for(auto hitcounter = 0; hitcounter < numHits; hitcounter++){
        auto hit0 = eudaq::getlittleendian<uint8_t>(&block[0]+bit+1);
        auto hit1 = eudaq::getlittleendian<uint8_t>(&block[0]+bit+2);
        auto hit2 = eudaq::getlittleendian<uint8_t>(&block[0]+bit+3);
        bit += 3;
        uint32_t hit_encoded = (hit0<<14)+(hit1<<7)+hit2;
        plane.SetPixel(hitcounter, hit_encoded >> 9, hit_encoded & 0x1FF,0);
        //std::cout << std::hex << hit_encoded << " x =" << (int)(hit_encoded >> 9) << std::endl;
    }
    d2->AddPlane(plane);
  }


  return true;
}
