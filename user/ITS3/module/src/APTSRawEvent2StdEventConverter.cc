// Optional parameters to pass in [EventLoaderEUDAQ2] section of Corryvreckan:

// n_samples_baseline (default:1)          Number of samples used to calculate the baseline
// n_signal_samples_after_min (default:0)  Number of samples for signal extraction after minimum. Signal minimum always included
// n_signal_samples_before_min (default:0) Number of samples for signal extraction before minimum

// ---------NOTE: Ignore the WARNING: "Unused configuration keys in section EventLoaderEUDAQ2:..."----------------
#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"
#include <iostream>
#include <vector>
#include <cmath>
#include <stdio.h>
#include <math.h>
#include <stdint.h>

class APTSRawEvent2StdEventConverter:public eudaq::StdEventConverter{
public:
  bool Converting(eudaq::EventSPC rawev,eudaq::StdEventSP stdev,eudaq::ConfigSPC conf) const override;
private:
  static void unscramble(const uint8_t *in,uint16_t *out) {
    const int chmap[]={2,1,5,0,4,8,9,12,13,14,10,15,11,7,6,3};
    for(int iw=0;iw!=16;++iw) {
      out[chmap[iw]]=0;
      for(int ib=0;ib!=16;++ib)
        out[chmap[iw]]|=(in[ib<<1|((iw>>3)&1)]>>(iw&0x7)&1)<<(15-ib);
    }
  }
  struct Config { //Variables for Baseline and signal extraction
    int nb  ;
    int ns_a;
    int ns_b;
  };
  Config& LoadConf(eudaq::ConfigSPC config_) const;
  static std::map<eudaq::ConfigSPC,Config> confs;
};

std::map<eudaq::ConfigSPC,APTSRawEvent2StdEventConverter::Config> APTSRawEvent2StdEventConverter::confs;


#define REGISTER_CONVERTER(name) namespace{auto dummy##name=eudaq::Factory<eudaq::StdEventConverter>::Register<APTSRawEvent2StdEventConverter>(eudaq::cstr2hash(#name));}
REGISTER_CONVERTER(APTS_0)
REGISTER_CONVERTER(APTS_1)

// Starting the loading from the Config

APTSRawEvent2StdEventConverter::Config& APTSRawEvent2StdEventConverter::LoadConf(eudaq::ConfigSPC conf_) const {
  if(confs.find(conf_)!=confs.end()) return confs[conf_];
  Config conf; //getting nb, ns_a and ns_b
  conf.nb   =1;
  conf.ns_a =0;
  conf.ns_b =0;
//   // get parameter from eventloader in conf file
  EUDAQ_INFO("Load configuration for APTS");
  // pass configuration via EUDAQ2 online monitor
  bool mon=conf_?conf_->HasSection("APTS"):false;
  if(mon){
    conf_->SetSection("APTS");
    EUDAQ_INFO("Set section `APTS` for online monitor");
  } else { // look for "identifier set by corry"
    // pass configuration via Corryvreckan
    std::string id=conf_?conf_->Get("identifier",""):""; // set by corry
    if(id!="") {
      size_t p=id.find("APTS_"); // TODO: no idea why the identifier is quoted in the config, may change in future?
      if(p!=std::string::npos) {
        EUDAQ_INFO("Set section `"+id+"` from Corryvreckan");
      }
      else {
        confs[conf_]=std::move(conf);
        return confs[conf_]; // also here: early out: do not try opening the dump file
      }
    }
  }

  if(conf_) {
    std::string name;
    name="n_samples_baseline";
    conf.nb = conf_->Get(name, 1); // get baseline parameter from config. If no baseline specified, take 1
    EUDAQ_DEBUG(name+" set to "+std::to_string(conf.nb));
    name="n_signal_samples_after_min";
    conf.ns_a = conf_->Get(name,0);
    EUDAQ_DEBUG(name+" set to "+std::to_string(conf.ns_a));
    name="n_signal_samples_before_min";
    conf.ns_b = conf_->Get(name,0);
    EUDAQ_DEBUG(name+" set to "+std::to_string(conf.ns_b));
  }


  confs[conf_]=std::move(conf);
  return confs[conf_];
}
//Starting of actual CONVERTING

bool APTSRawEvent2StdEventConverter::Converting(eudaq::EventSPC in,eudaq::StdEventSP out,eudaq::ConfigSPC conf_) const{
  Config &conf=LoadConf(conf_); //load config including nb, ns_a and ns_b
  auto rawev=std::dynamic_pointer_cast<const eudaq::RawEvent>(in);
  if(rawev->GetNumBlock()==0) return false; // TODO: how/can this happen?
  const std::vector<uint8_t> &block=rawev->GetBlock(0);// GET BLOCK OF DATA: one contains timestamp[1], one the data[0]
  size_t n=block.size();
  if(n<40||n%40!=0) {  //check that block is multiple of 40
    EUDAQ_ERROR("Error: Incomplete Data Block. Block size is "+std::to_string(n)+", but should be multiple of 40");
    return false;
    }
  const uint8_t *data=block.data();
  eudaq::StandardPlane plane(rawev->GetDeviceN(),"ITS3DAQ","APTS");// create pxiel matrix, each entry is integral of the pixel waveform
  // BASELINE
  if(n/40<conf.nb) {
    EUDAQ_ERROR("Error: Baseline sample number ("+std::to_string(conf.nb)+") exceeds total number of samples ("+std::to_string(n/40)+")");
    return false;
    }
  float baseline[16]={0.};
  for(int is=0;is!=conf.nb;++is) { //for each baseline point in each pixel: calculate baseline for each pixel from the adc value
    uint16_t adcs[16];
    unscramble(data+is*40,adcs);
    for(int i=0;i!=16;++i) baseline[i]+=adcs[i];
  }
  for(int i=0;i!=16;++i) baseline[i]/=conf.nb;
  // SAMPLING POINT (=minimum)
  int imin=0;
  float vmin=1e6;
  for(int is=conf.nb;is!=n/40-conf.ns_a;++is) {
    uint16_t adcs[16];
    unscramble(data+is*40,adcs); 
    for(int i=0;i<16;++i) {
      float v=adcs[i]-baseline[i];
      if(v<vmin) {vmin=v;imin=is;} //vmin = value in adc of lowest point, imin is number of samplingpoint corresponding to the minimum
    }
  }
  // SIGNAL EXTRACTION (=integral)
  if(n/40<imin+conf.ns_a) {
    EUDAQ_ERROR("Error: Targeted samples for signal extraction after minimum ("+std::to_string(imin+conf.ns_a)+") exceeds total number of samples ("+std::to_string(n/40)+")");
    return false;
    }
  if(0>imin-conf.ns_b) {
    EUDAQ_ERROR("Error: Targeted samples for signal extraction before minimum ("+std::to_string(imin-conf.ns_b)+") is smaller than 0");
    return false;
    }
  float signal[16]={0};
  for(int is=imin-conf.ns_b; is!=imin+conf.ns_a+1; ++is) { //starting ns_b points before minimum (up until ns_a points after minimum) 
    uint16_t adcs[16];
    unscramble(data+is*40,adcs);
    for(int i=0;i!=16;++i) {
      float v=adcs[i]-baseline[i];
      signal[i]+=v;
    }
  }
  for(int i=0;i!=16;++i) signal[i]/=(1+conf.ns_b+conf.ns_a);
  plane.SetSizeRaw(4,4);
  for(int y=0;y!=4;++y) 
    for(int x=0;x!=4;++x)
      plane.SetPixel((y*4)+x,x,y,-signal[y*4+x],(uint64_t)imin*250000);
  out->AddPlane(plane);
  return true;
}
