// Optional parameters to pass in [EventLoaderEUDAQ2] section of Corryvreckan:

// sample_baseline (default:0)  Sample used to calculate the baseline
// n_signal_samples_after_min (default:0)  Number of samples for signal extraction after minimum. Signal minimum always included
// n_signal_samples_before_min (default:0) Number of samples for signal extraction before minimum
// minimum_sampling_frame (default:n_samples_baseline+1) Define minimum of the range where we look for the sampling frame
// maximum_sampling_frame (default:200) Define maximum of the range where we look for the sampling frame
// estimate_noise (default:false) If true the converter will set the pixel values using a frame where the signal is not expected

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
  static const int npixels = 16;
  static const int frame_size_in_byte = 40;
  static const int frame_to_ps = 250000;
  static void unscramble(const uint8_t *in,uint16_t *out) {
    const int chmap[]={2,1,5,0,4,8,9,12,13,14,10,15,11,7,6,3};
    for(int iw=0;iw!=16;++iw) {
      out[chmap[iw]]=0;
      for(int ib=0;ib!=16;++ib)
        out[chmap[iw]]|=(in[ib<<1|((iw>>3)&1)]>>(iw&0x7)&1)<<(15-ib);
    }
  }
  struct Config { //Variables for Baseline and signal extraction
    int device_n;
    int sample_baseline;
    int minimum_sampling_frame;
    int maximum_sampling_frame;
    int n_signal_samples_after_min;
    int n_signal_samples_before_min;
    bool estimate_noise;
  };
  Config& LoadConf(eudaq::ConfigSPC config_) const;
  static std::map<eudaq::ConfigSPC,Config> confs;
};

std::map<eudaq::ConfigSPC,APTSRawEvent2StdEventConverter::Config> APTSRawEvent2StdEventConverter::confs;


#define REGISTER_CONVERTER(name) namespace{auto dummy##name=eudaq::Factory<eudaq::StdEventConverter>::Register<APTSRawEvent2StdEventConverter>(eudaq::cstr2hash(#name));}
REGISTER_CONVERTER(APTS_0)
REGISTER_CONVERTER(APTS_1)
REGISTER_CONVERTER(APTS_2)
REGISTER_CONVERTER(APTS_3)
REGISTER_CONVERTER(APTS_4)
REGISTER_CONVERTER(APTS_5)
REGISTER_CONVERTER(APTS_6)
REGISTER_CONVERTER(APTS_7)
REGISTER_CONVERTER(APTS_8)
REGISTER_CONVERTER(APTS_9)

// Starting the loading from the Config

APTSRawEvent2StdEventConverter::Config& APTSRawEvent2StdEventConverter::LoadConf(eudaq::ConfigSPC conf_) const {
  if(confs.find(conf_)!=confs.end()) return confs[conf_];
  Config conf; //getting sample_baseline, minimum_sampling_frame, maximum_sampling_frame, n_signal_samples_after_min, n_signal_samples_before_min, and estimate_noise
  conf.device_n                    = -1; // fallback
  conf.sample_baseline             = 0;
  conf.minimum_sampling_frame      = 2;
  conf.maximum_sampling_frame      = 200;
  conf.n_signal_samples_after_min  = 0;
  conf.n_signal_samples_before_min = 0;
  conf.estimate_noise              = false;

  // get parameter from eventloader in conf file
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
      if(p==std::string::npos) {
        conf.device_n = -2; // other plane is looked for, bail out early to not waste time in full decoding
        confs[conf_]=std::move(conf);
        return confs[conf_]; // also here: early out: do not try opening the dump file
      }
      else {
        p+=5;
        conf.device_n=std::stoi(id.substr(p));
        EUDAQ_INFO("Set config `"+id+"` from Corryvreckan");
      }
    }
  }

  if(conf_) {
    std::string name;
    name="sample_baseline";
    conf.sample_baseline = conf_->Get(name, 1); 
    EUDAQ_DEBUG(name+" set to "+std::to_string(conf.sample_baseline)); // get the sample used to compute the baseline
    name="minimum_sampling_frame";
    conf.minimum_sampling_frame = conf_->Get(name, conf.sample_baseline +1 ); // get minimum value of the sampling from from config. If no value is specified, take conf.nb + 1
    EUDAQ_DEBUG(name+" set to "+std::to_string(conf.minimum_sampling_frame));
    name="maximum_sampling_frame";
    conf.maximum_sampling_frame = conf_->Get(name, 200); // get maximum value of the sampling from from config. If no value is specified, take 200
    EUDAQ_DEBUG(name+" set to "+std::to_string(conf.maximum_sampling_frame));
    name="n_signal_samples_after_min";
    conf.n_signal_samples_after_min = conf_->Get(name, 0);
    EUDAQ_DEBUG(name+" set to "+std::to_string(conf.n_signal_samples_after_min));
    name="n_signal_samples_before_min";
    conf.n_signal_samples_before_min = conf_->Get(name, 0);
    EUDAQ_DEBUG(name+" set to "+std::to_string(conf.n_signal_samples_before_min));
    name="estimate_noise";
    std::string estimation = conf_->Get("estimate_noise","false");
    conf.estimate_noise = estimation == "true";
    EUDAQ_DEBUG(name+" set to "+std::to_string(conf.estimate_noise));

    if(conf.minimum_sampling_frame>conf.maximum_sampling_frame) {
      EUDAQ_THROW("Error: The lowest value allowed for the sampling frame ("+std::to_string(conf.minimum_sampling_frame)+") exceeds the highest ("+std::to_string(conf.maximum_sampling_frame)+")");
    }
    
    if(conf.sample_baseline > conf.minimum_sampling_frame-conf.n_signal_samples_before_min && conf.sample_baseline < conf.minimum_sampling_frame+conf.n_signal_samples_after_min) {
      EUDAQ_THROW("Error: The value used for the baseline estimation ("+std::to_string(conf.minimum_sampling_frame)+") is used between the frame used for computing the signal");
    }

  }


  confs[conf_]=std::move(conf);
  return confs[conf_];
}
//Starting of actual CONVERTING

bool APTSRawEvent2StdEventConverter::Converting(eudaq::EventSPC in,eudaq::StdEventSP out,eudaq::ConfigSPC conf_) const{
  Config &conf=LoadConf(conf_); //load config including nb, ns_a and ns_b
  if(conf.device_n==-2) return false;
  auto rawev=std::dynamic_pointer_cast<const eudaq::RawEvent>(in);
  if(conf.device_n>=0 && conf.device_n!=rawev->GetDeviceN()) return false;
  if(rawev->GetNumBlock()==0) return false; // TODO: how/can this happen?
  const std::vector<uint8_t> &block=rawev->GetBlock(0);// GET BLOCK OF DATA: one contains timestamp[1], one the data[0]
  size_t n=block.size();
  if(n<frame_size_in_byte||n%frame_size_in_byte!=0) {  //check that block is multiple of frame_size_in_byte
    EUDAQ_ERROR("Error: Incomplete Data Block. Block size is "+std::to_string(n)+", but should be multiple of "+std::to_string(frame_size_in_byte));
    return false;
    }
  const uint8_t *data=block.data();
  eudaq::StandardPlane plane(rawev->GetDeviceN(),"ITS3DAQ","APTS");// create pxiel matrix, each entry is integral of the pixel waveform
  if(n/frame_size_in_byte<conf.minimum_sampling_frame) {
    EUDAQ_ERROR("Error: The minimum value allowed for the sampling frame ("+std::to_string(conf.minimum_sampling_frame)+") exceeds total number of samples ("+std::to_string(n/frame_size_in_byte)+")");
    return false;
  }

  if(conf.maximum_sampling_frame>n/frame_size_in_byte-conf.n_signal_samples_after_min) {
    EUDAQ_ERROR("Error: The highest value allowed for the sampling frame ("+std::to_string(conf.maximum_sampling_frame)+") exceeds total number of samples ("+std::to_string(n/frame_size_in_byte)+") - the number of frames used after the sampling frame ("+std::to_string(conf.n_signal_samples_after_min)+")");
    return false;
  }

  // BASELINE
  if(n/frame_size_in_byte<conf.sample_baseline) {
    EUDAQ_ERROR("Error: The frame used for the baseline estimation ("+std::to_string(conf.sample_baseline)+") exceeds total number of samples ("+std::to_string(n/frame_size_in_byte)+")");
    return false;
  }
  
  uint16_t baseline[npixels];
  unscramble(data+conf.sample_baseline*frame_size_in_byte, baseline);

  // SAMPLING POINT (=minimum)
  int imin=0;
  float vmin=1e6;
  for(int is=conf.minimum_sampling_frame;is!=conf.maximum_sampling_frame+1;++is) {
    uint16_t adcs[npixels];
    unscramble(data+is*frame_size_in_byte,adcs); 
    for(int i=0;i<16;++i) {
      float v=adcs[i]-baseline[i];
      if(v<vmin) {vmin=v;imin=is;} //vmin = value in adc of lowest point, imin is number of samplingpoint corresponding to the minimum
    }
  }
  // SIGNAL EXTRACTION (=integral)
  if(n/frame_size_in_byte<imin+conf.n_signal_samples_after_min) {
    EUDAQ_ERROR("Error: Targeted samples for signal extraction after minimum ("+std::to_string(imin+conf.n_signal_samples_after_min)+") exceeds total number of samples ("+std::to_string(n/frame_size_in_byte)+")");
    return false;
    }
  if(0>imin-conf.n_signal_samples_before_min) {
    EUDAQ_ERROR("Error: Targeted samples for signal extraction before minimum ("+std::to_string(imin-conf.n_signal_samples_before_min)+") is smaller than 0");
    return false;
    }
  float signal[npixels]={0};

  int is_in = imin-conf.n_signal_samples_before_min;  //starting ns_b points before minimum (up until ns_a points after minimum)
  int is_fin = imin+conf.n_signal_samples_after_min+1;
  if(conf.estimate_noise){ //save the pixel values before the frames used for the estimation of the baseline
    is_in = 2*conf.sample_baseline-imin-conf.n_signal_samples_after_min;
    is_fin = 2*conf.sample_baseline-imin+conf.n_signal_samples_before_min+1;
  }

  for(int is=is_in; is!=is_fin; ++is) {
    uint16_t adcs[npixels];
    unscramble(data+is*frame_size_in_byte,adcs);
    for(int i=0;i!=npixels;++i) {
      float v=adcs[i]-baseline[i];
      signal[i]+=v;
    }
  }
  
  for(int i=0;i!=npixels;++i) signal[i]/=(1+conf.n_signal_samples_before_min+conf.n_signal_samples_after_min);
  plane.SetSizeRaw(4,4);
  for(int y=0;y!=4;++y) 
    for(int x=0;x!=4;++x)
      plane.SetPixel((y*4)+x,x,y,-signal[y*4+x],(uint64_t)imin*frame_to_ps);
  out->AddPlane(plane);
  return true;
}
