#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"

class ItsTtcRawEvent2StdEventConverter: public eudaq::StdEventConverter{
public:
  bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("ITS_TTC");
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
    Register<ItsTtcRawEvent2StdEventConverter>(ItsTtcRawEvent2StdEventConverter::m_id_factory);
}

bool ItsTtcRawEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2,
						  eudaq::ConfigSPC conf) const{
  auto raw = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  if (!raw){
    EUDAQ_THROW("dynamic_cast error: from eudaq::Event to eudaq::RawEvent");
  } 
  auto block = raw->GetBlock(0);
  size_t size_of_TTC = block.size() /sizeof(uint64_t);
  const uint64_t *TTC = nullptr;
  if (size_of_TTC) {
    TTC = reinterpret_cast<const uint64_t *>(block.data());
  }
  size_t j = 0;
  for (size_t i = 0; i < size_of_TTC; ++i) {
    uint64_t data = TTC[i];
    switch (data >> 60) {
    case 0xc:{
      uint32_t TYPE = (uint32_t)(data>>52)& 0xf;
      uint32_t L0ID = (uint32_t)(data>>48)& 0xf;
      uint32_t TS = (uint32_t)(data>>32) & 0xffff;
      uint32_t BIT = (uint32_t)data;
      d2->SetTag("PTDC.TYPE", TYPE);
      d2->SetTag("PTDC.L0ID", L0ID);
      d2->SetTag("PTDC.TS", TS);
      d2->SetTag("PTDC.BIT", BIT);
      break;
    }
    case 0xd:{
      uint32_t hsioID = (uint32_t)(data >> 40) & 0xffff;
      uint64_t TLUID = data & 0xffff;
      d2->SetTag("TLU.TLUID", TLUID);
      d2->SetTag("TLU.L0ID", hsioID);
      break;
    }
    case 0xe:{
      uint32_t L0ID = (uint32_t)(data >> 40) & 0xffff;
      uint64_t TDC = data & 0xfffff;
      d2->SetTag("TDC.DATA", TDC);
      d2->SetTag("TDC.L0ID", L0ID);
      break;
    }
    case 0xf:{
      uint64_t timestamp = data & 0x000000ffffffffffULL;
      uint32_t L0ID = (uint32_t)(data >> 40) & 0xffff;
      d2->SetTag("TS.DATA", timestamp);
      d2->SetTag("TS.L0ID", L0ID);
      break;
    }
    case 0x0:
      std::cout<<"[SCTConvertPlugin:] TTC type 0x00: maybe de-sync"<<std::endl;
      break;
    default:
      EUDAQ_WARN("unknown data type");
      break;
    }
  }
  return true;
}
