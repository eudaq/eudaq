#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"
//--> ds
#include "Data.h"
#include "XmlVariables.h"
#include "KpixEvent.h"
#include "KpixSample.h"

class kpixRawEvt2StdEventConverter: public eudaq::StdEventConverter{
public:
  bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("KpixRawEvt");
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
    Register<kpixRawEvt2StdEventConverter>(kpixRawEvt2StdEventConverter::m_id_factory);
}

bool kpixRawEvt2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const{

  /* 
   * It loops over [eudaq raw events] == [kpix acq. cycles]
   */
    
  auto rawev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  
  size_t nblocks= rawev->NumBlocks();

  std::cout<< "[dev] nblocks =  " << nblocks << std::endl;
    
  auto block_n_list = rawev->GetBlockNumList();
  
  Data* data;
  KpixEvent    event;
  KpixSample   *sample;
    
  for(auto &block_n: block_n_list){
    
    std::cout<< "[dev] block id listerning ==> " << block_n <<std::endl; ;
    
    std::vector<uint8_t> block = rawev->GetBlock(block_n);

    if (block.size()==0) EUDAQ_THROW("empty data");
    else {
     
      size_t size_of_kpix = block.size()/sizeof(uint32_t);
      std::cout<<"[dev] # of block.size()/sizeof(uint32_t) = " << block.size()/sizeof(uint32_t) << "\n"
	       <<"\t sizeof(uint32_t) = " << sizeof(uint32_t) << std::endl;


      uint32_t *kpixEvt = nullptr;
      if (size_of_kpix)
	kpixEvt = reinterpret_cast<uint32_t *>(block.data());

      event.copy(kpixEvt, size_of_kpix);
      
      std::cout<<"\t Uint32_t  = " << kpixEvt <<"\n"
	       <<"\t evtNum    = " << kpixEvt[0] <<std::endl;

      std::cout << "\t kpixEvent.evtNum = " << event.eventNumber() <<std::endl;
      std::cout << "\t kpixEvent.timestamp = "<< event.timestamp() <<std::endl;
      std::cout << "\t kpixEvent.count = "<< event.count() <<std::endl;


      d2 -> SetEventN(event.eventNumber());
      d2 -> SetTimestampe(event.timestampe());
      
      /* read kpix data */
      //data->copy(kpixEvt, size_of_kpix);
      
    }

    /* parts related to Ex0RawEvt commented out */
    /*    if(block.size() < 2)
      EUDAQ_THROW("Unknown data");
    uint8_t x_pixel = block[0];
    uint8_t y_pixel = block[1];
    std::vector<uint8_t> hit(block.begin()+2, block.end());
    if(hit.size() != x_pixel*y_pixel)
      EUDAQ_THROW("Unknown data");
    eudaq::StandardPlane plane(block_n, "my_ex0_plane", "my_ex0_plane");
    plane.SetSizeZS(hit.size(), 1, 0);
    for(size_t i = 0; i < y_pixel; ++i) {
      for(size_t n = 0; n < x_pixel; ++n){
	plane.PushPixel(n, i , hit[n+i*x_pixel]);
      }
    }
    d2->AddPlane(plane);
  }
    */ 

  }    
    
  return true;
}
