#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"
#include <iostream>


class ALPIDERawEvent2StdEventConverter:public eudaq::StdEventConverter{
public:
  bool Converting(eudaq::EventSPC rawev,eudaq::StdEventSP stdev,eudaq::ConfigSPC conf_) const override;
private:
  void Dump(const std::vector<uint8_t> &data,size_t i) const;
  struct Config {
    int device_n;
  };
  Config& LoadConf(eudaq::ConfigSPC config_) const;
  static std::map<eudaq::ConfigSPC,Config> confs;
};

#define REGISTER_CONVERTER(name) namespace{auto dummy##name=eudaq::Factory<eudaq::StdEventConverter>::Register<ALPIDERawEvent2StdEventConverter>(eudaq::cstr2hash(#name));}
REGISTER_CONVERTER(ALPIDE_plane_0)
REGISTER_CONVERTER(ALPIDE_plane_1)
REGISTER_CONVERTER(ALPIDE_plane_2)
REGISTER_CONVERTER(ALPIDE_plane_3)
REGISTER_CONVERTER(ALPIDE_plane_4)
REGISTER_CONVERTER(ALPIDE_plane_5)
REGISTER_CONVERTER(ALPIDE_plane_6)
REGISTER_CONVERTER(ALPIDE_plane_7)
REGISTER_CONVERTER(ALPIDE_plane_8)
REGISTER_CONVERTER(ALPIDE_plane_9)
REGISTER_CONVERTER(ALPIDE_plane_10)
REGISTER_CONVERTER(ALPIDE_plane_11)
REGISTER_CONVERTER(ALPIDE_plane_12)
REGISTER_CONVERTER(ALPIDE_plane_13)
REGISTER_CONVERTER(ALPIDE_plane_14)
REGISTER_CONVERTER(ALPIDE_plane_15)
REGISTER_CONVERTER(ALPIDE_plane_16)
REGISTER_CONVERTER(ALPIDE_plane_17)
REGISTER_CONVERTER(ALPIDE_plane_18)
REGISTER_CONVERTER(ALPIDE_plane_19)

ALPIDERawEvent2StdEventConverter::Config& ALPIDERawEvent2StdEventConverter::LoadConf(eudaq::ConfigSPC conf_) const {
  if(confs.find(conf_)!=confs.end()) return confs[conf_];
  EUDAQ_DEBUG("Load configuration for ALPIDE");
  Config conf;
  conf.device_n = -1; // decode all fallback (used in online monitor)

  // pass configuration via Corryvreckan EUDAQ2EventLoader
  std::string id=conf_?conf_->Get("identifier",""):""; // set by corry
  if(id!="") {
    size_t p=id.find("ALPIDE_"); // TODO: no idea why the identifier is quoted in the config, may change in future?
    if(p==std::string::npos) {
      conf.device_n = -2; // other plane is looked for, bail out early to not waste time in full decoding
    }
    else {
      p+=7;
      conf.device_n=std::stoi(id.substr(p));
      EUDAQ_DEBUG(" set device number `"+id+"` from Corryvreckan");
    }
  }
  confs[conf_]=std::move(conf);
  return confs[conf_];
}

std::map<eudaq::ConfigSPC,ALPIDERawEvent2StdEventConverter::Config> ALPIDERawEvent2StdEventConverter::confs;

bool ALPIDERawEvent2StdEventConverter::Converting(eudaq::EventSPC in,eudaq::StdEventSP out,eudaq::ConfigSPC conf_) const{
  Config &conf=LoadConf(conf_);
  if(conf.device_n==-2) return false; // Corry event loader is looking for another plane
  auto rawev=std::dynamic_pointer_cast<const eudaq::RawEvent>(in);
  std::vector<uint8_t> data=rawev->GetBlock(0); // FIXME: copy?
  if(conf.device_n>=0 && conf.device_n!=rawev->GetDeviceN()) return false;
  eudaq::StandardPlane plane(rawev->GetDeviceN(),"ITS3DAQ","ALPIDE");
  plane.SetSizeZS(1024,512,0,1); // 0 hits so far + 1 frame
  size_t i=0;
  size_t n=data.size();
  if (!(data[i]==0xAA && data[i+1]==0xAA && data[i+2]==0xAA && data[i+3]==0xAA)) {
    EUDAQ_WARN("BAD DATA. Skipping raw event."); // TODO
    return false;
  }
  uint64_t tev=0;
  uint32_t iev=0;
  for (int j=0;j<4;++j) iev|=((uint32_t)data[i+4+j])<<(j*8);
  for (int j=0;j<8;++j) tev|=((uint64_t)data[i+8+j])<<(j*8);
  tev*=12500; // 80Mhz clks to 1ps

  // forcing corry to fall back on trigger IDs
  out->SetTimeBegin(0);
  out->SetTimeEnd(0);
  out->SetTriggerN(iev);

  i+=16;
  uint8_t reg;
  if((data[i]&0xF0)==0xE0) {// chip empty frame
    i+=4;
  } else if((data[i]&0xF0)==0xA0) {// chip header
    i+=2;
    while(i<n-4) {
      uint8_t data0=data[i];
      if((data0&0xC0)==0x00) {// data long
      	uint32_t d=reg<<14|(data0&0x3F)<<8|data[i+1];
       	uint16_t x=d>>9&0x3FE|(d^d>>1)&0x1;
       	uint16_t y=d>>1&0x1FF;
       	plane.PushPixel(x,y,1,tev); // column, row, charge, time
       	uint8_t data2=data[i+2];
       	d+=1;
       	while(data2) {
       	  if(data2&1) {
            x=d>>9&0x3FE|(d^d>>1)&0x1;
            y=d>>1&0x1FF;
            plane.PushPixel(x,y,1,tev); // column, row, charge, time
          }
          data2>>=1;
          d+=1;
        }
        i+=3;
        } else if((data0&0xC0)==0x40) {// data short
          uint32_t d=reg<<14|(data0&0x3F)<<8|data[i+1];
          uint16_t x=d>>9&0x3FE|(d^d>>1)&0x1;
          uint16_t y=d>>1&0x1FF;
          plane.PushPixel(x,y,1,tev); // column, row, charge, time
          i+=2;
        } else if((data0&0xE0)==0xC0) {// region header
          reg=data0&0x1F;
          i+=1;
        } else if((data0&0xF0)==0xB0) {// chip trailer
          i+=1;
          i=(i+3)/4*4;
          break;
        } else if(data0==0xFF) {// IDLE (why?)
          i+=1;
        } else if (data0==0xAA) {
          EUDAQ_WARN("BAD WORD. An event header now? Skipping raw event.");
          Dump(data,i);
          return false;
        } else {
          EUDAQ_WARN("BAD WORD. Skipping raw event.");
          Dump(data,i);
          return false;
        }
      }
    }
  else {
    EUDAQ_WARN("BAD WORD. No event start? Skipping raw event.");
    Dump(data,i);
    return false;
  }
  if (!(data[i]==0xBB && data[i+1]==0xBB && data[i+2]==0xBB && data[i+3]==0xBB)) {
    EUDAQ_WARN("BAD WORD. Bad/no event trailer? Skipping raw event.");
    Dump(data,i);
    return false;
  }
  out->AddPlane(plane);
  return true;
}

void ALPIDERawEvent2StdEventConverter::Dump(const std::vector<uint8_t> &data,size_t i) const {
  char buf[100];
  EUDAQ_WARN("Raw event dump:");
  for (size_t j=0;j<data.size();++j) {
    if (i==j)
      sprintf(buf,"%06zX: %02X <-- problem around here?",j,data[j]);
    else
      sprintf(buf,"%06zX: %02X",j,data[j]);
    EUDAQ_WARN(buf);
  }
}
