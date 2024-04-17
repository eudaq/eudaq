#include "eudaq/RawEvent.hh"
#include "eudaq/StdEventConverter.hh"
#include <cmath>
#include <iostream>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <vector>
#include <array>  
#include <nlohmann/json.hpp>
// Scope data format
// length: 400023
// Data structure
// + scope header: 22 (C1:WF DAT1,#900040000)
// + origianl data: 400,000 
// + scope trailer: 1 (\n)

class OPAMPRawEvent2StdEventConverter : public eudaq::StdEventConverter {
public:
  bool Converting(eudaq::EventSPC rawev,eudaq::StdEventSP stdev,eudaq::ConfigSPC conf) const override;
private:
  static const int npixels = 16;  
  static const int frame_size_in_byte = 40;    // DAQboard ADC frame
  static void unscramble(const uint8_t *in,uint16_t *out) {
    const int chmap[]={2,1,5,0,4,8,9,12,13,14,10,15,11,7,6,3};
    for(int iw=0;iw!=16;++iw) {
      out[chmap[iw]]=0;
      for(int ib=0;ib!=16;++ib)
        out[chmap[iw]]|=(in[ib<<1|((iw>>3)&1)]>>(iw&0x7)&1)<<(15-ib);
    }
  }
  template <typename T1, typename T2>
  static float WaveformAverage(const T1 *wf, int n_samples = 500, int first_sample = 0) {
    float avg = 0;
    for (int i = first_sample; i < first_sample + n_samples; i++) {
      avg += static_cast<T2>(wf[i]);
    } 
    return avg/n_samples;
  }
  
  static int getIndex(std::vector<float> v, int K)
  {
    int index = 9999;
    auto it = std::find(v.begin(), v.end(), K);
    if (it != v.end()) {
        index = it - v.begin();
    }
    return index;
  }

  // t0 finder algorithm based on derivative
  static std::vector<float> derivative_rec (const uint8_t *ch_data, int npoints = 700, int nframes = 40000){
    std::vector<float> der;
    std::vector<int8_t> ch_data_int8;
    for(int i=0; i<nframes; i++) ch_data_int8.push_back(static_cast<int8_t>(ch_data[i])); // convert uint8_t to int8_t
    float sp{0}, sm{0};
    for(int i=0; i<nframes; i++){
        der.push_back(sp-sm);
        if (i <= npoints){
            sp = sp + ch_data_int8[2*i+-1] + ch_data_int8[2*i] - ch_data_int8[i];
            sm = sm + ch_data_int8[i-1];
        }
        else if (i > npoints and (i+npoints)<=(nframes-1)){
            sp = sp + ch_data_int8[i+npoints] - ch_data_int8[i];
            sm = sm + ch_data_int8[i-1] - ch_data_int8[i-npoints-1];
        }
        else if ((i+npoints) > (nframes-1)){
            sp = sp - ch_data_int8[i];
            sm = sm + ch_data_int8[i-1] - ch_data_int8[2*i-nframes] - ch_data_int8[2*i-nframes-1];
        }
    } 
    return der;
  }

  static int find_edge (const uint8_t *ch_data, int thr = 600, int npoints = 700){
    int leftIndex = -999;
    std::vector<float> der = derivative_rec(ch_data, npoints);
    int val = *std::min_element(der.begin(), der.end());
    if (val < -thr)
        leftIndex = getIndex(der,val);
    return leftIndex;
  }

  template <typename T1, typename T2>
  static int GetMinSample(const T1 *ch_data, int osc_data_size, int osc_first_sample) {
    int min_sample = 0;
    auto min_value = static_cast<T2>(ch_data[osc_first_sample]); 
    for (int i = osc_first_sample; i < osc_data_size-2; i++) {
      auto temp_value = static_cast<T2>(ch_data[i]); 
      if (temp_value < min_value) {
        min_value = temp_value;
        min_sample = i;
      }
    }
    return min_sample;
  }
  // Make a function named GetAveragedWaveform to return the averaged waveform
  static std::vector<float> GetAveragedWaveform(const uint8_t *ch_data, int osc_data_size, int n_samples_avg = 2, int osc_first_sample = 23) {
    std::vector<float> averaged_waveform;
    // make a average +- n_samples_avg range
    for (int i = osc_first_sample+n_samples_avg; i < osc_data_size-n_samples_avg; i++) {
      float avg = 0;
      if (n_samples_avg > 0) {
        for (int j = i - n_samples_avg; j < i + n_samples_avg; j++) {
          avg += static_cast<int8_t>(ch_data[j]); 
        }
        avg = avg / (2 * n_samples_avg);
      }
      else
        avg = static_cast<int8_t>(ch_data[i]);
      averaged_waveform.push_back(avg);
    }
    return averaged_waveform;
  }

  static bool HasLGADSignal(const float *ch_data, float threshold, int osc_first_sample) {
    // try to find the minimum and compute the signal amplitude
    // then compare it with the threshold
    // if the signal is below the threshold, return true

    // Find the min sample
    auto min_sample = GetMinSample<float,float>(ch_data, 40000,osc_first_sample);
    float min_value = static_cast<int8_t>(ch_data[min_sample]);
    // Compute the signal amplitude
    auto base_value = WaveformAverage<float, float>(ch_data,400,min_sample-420); // at the moment we're using hardcoded values
    auto signal = base_value - min_value;
    // Compare with the threshold
    if (signal > threshold)
      return true;
    return false;
  }
  struct Config { //Variables for Baseline and signal extraction
    int device_n;
    int sample_baseline_opamp;                // Sample of waveform to be used for baseline calculation in DAQ pixels
    int first_sample_minimum_finder_opamp;    // First sample to be used for minimum finder in DAQ pixels
    int last_sample_minimum_finder_opamp;     // Last sample to be used for minimum finder in DAQ pixels
    int n_samples_after_min_signal_opamp;     // Number of samples after minimum to be used for signal calculation in DAQ pixels
    int n_samples_before_min_signal_opamp;    // Number of samples before minimum to be used for signal calculation in DAQ pixels
    float daq_charge_conversion_factor_opamp; // measured ratio between 1 DAQ ADC unit (nominal value 38.1uV) and 1 scope ADC unit (nominal value 0.625mV) 
    int n_samples_baseline_opamp_scope;               // Number of samples to be used for baseline calculation in scope pixels
    int n_samples_signal_opamp_scope;                 // Number of samples to be used for signal calculation in scope pixels
    int baseline_shifting_from_t0_opamp_scope;        // Number of samples to be shifted from t0 to be used for baseline calculation in scope pixels
    int underline_delay_from_t0_opamp_scope;          // Number of samples to be delayed from t0 to be used for underline calculation in scope pixels
    float threshold_cut_t0_finder_opamp_scope;        // Threshold for t0 finder in scope pixels
    int n_points_t0_finder_opamp_scope;               // Number of points to be used for t0 finder in scope pixels
    float scope_charge_conversion_factor_opamp_scope; // Oscilloscope ADC->mV conversion factor
    double Mn_Ka[npixels];    // Mn Ka mean value of each pixel 
    bool estimate_noise_daq;   // Togle to estimate noise in DAQ pixels
    bool estimate_noise_scope; // Togle to estimate noise in scope pixels
    int noise_t0_sample_scope; // Sample of waveform to be used for the t0 sample for noise estimation in scope pixels
    float threshold_lgad_signal; // Threshold for lgad signal check
    float osc_first_sample;
    bool check_LGAD_signal;
    int n_points_average_waveform;
    int number_pixels_scope;
    int total_electron_from_source;
    std::array<int,3> px_idx_scope = {-1,-1,-1}; //index of the pixels readout via scope
    std::array<int,3> px_x_scope = {-1,-1,-1};   //x-coordinate of the pixels readout via scope
    std::array<int,3> px_y_scope = {-1,-1,-1};   //y-coordinate of the pixels readout via scope
  };
  Config& LoadConf(eudaq::ConfigSPC config_) const;
  static std::map<eudaq::ConfigSPC,Config> confs;
  
};
std::map<eudaq::ConfigSPC,OPAMPRawEvent2StdEventConverter::Config> OPAMPRawEvent2StdEventConverter::confs;

#define REGISTER_CONVERTER(name) namespace{auto dummy##name=eudaq::Factory<eudaq::StdEventConverter>::Register<OPAMPRawEvent2StdEventConverter>(eudaq::cstr2hash(#name));}
REGISTER_CONVERTER(OPAMP_0)
REGISTER_CONVERTER(OPAMP_1)
REGISTER_CONVERTER(OPAMP_2)
REGISTER_CONVERTER(OPAMP_3)
REGISTER_CONVERTER(OPAMP_4)
REGISTER_CONVERTER(OPAMP_5)

OPAMPRawEvent2StdEventConverter::Config& OPAMPRawEvent2StdEventConverter::LoadConf(eudaq::ConfigSPC conf_) const {
  if(confs.find(conf_)!=confs.end()) return confs[conf_];
  Config conf; // Getting parameters for opamp DAQ and scope
  conf.device_n = -1; // Fallback
  // DAQ parameters
  conf.sample_baseline_opamp = 0;                 // Sample of waveform to be used for baseline calculation in DAQ pixels
  conf.first_sample_minimum_finder_opamp = 95;     // First sample to be used for minimum finder in DAQ pixels
  conf.last_sample_minimum_finder_opamp = 110;     // Last sample to be used for minimum finder in DAQ pixels
  conf.n_samples_after_min_signal_opamp = 0;       // Number of samples after minimum to be used for signal calculation in DAQ pixels
  conf.n_samples_before_min_signal_opamp = 0;      // Number of samples before minimum to be used for signal calculation in DAQ pixels
  conf.daq_charge_conversion_factor_opamp = 0.061; //  measured ratio between 1 DAQ ADC unit (nominal value 38.1uV) and 1 scope ADC unit (nominal value 0.625mV) 
  // Scope parameters
  conf.n_samples_signal_opamp_scope = 100;                  // Number of samples to be used for signal calculation in scope pixels
  conf.n_samples_baseline_opamp_scope = 100;               // Number of samples to be used for baseline calculation in scope pixels
  conf.baseline_shifting_from_t0_opamp_scope = 700;        // Number of samples to be shifted from t0 to be used for baseline calculation in scope pixels
  conf.underline_delay_from_t0_opamp_scope = 600;          // Number of samples to be delayed from t0 to be used for underline calculation in scope pixels
  conf.threshold_cut_t0_finder_opamp_scope = 1387;            // Threshold for t0 finder in scope pixels
  conf.n_points_t0_finder_opamp_scope = 700;                // #N integration points to be used for t0 finder in scope pixels
  conf.scope_charge_conversion_factor_opamp_scope = 0.625; // Oscilloscope ADC->mV conversion factor
  conf.osc_first_sample = 23;   // First smaple to be read from scope data, different in single and sequence mode
  conf.number_pixels_scope = 0;
  conf.total_electron_from_source = 1640; // total electrons from the source to calibrate the signal
  // Noise estimation parameters
  conf.estimate_noise_daq    = false; // Togle to estimate noise in DAQ pixels
  conf.estimate_noise_scope  = false; // Togle to estimate noise in scope pixels
  conf.noise_t0_sample_scope = -1;    // Sample of waveform to be used for the t0 sample for noise estimation in scope pixels
  // LGAD parameters
  conf.threshold_lgad_signal = 0.5;   // Threshold for lgad signal check
  conf.check_LGAD_signal = false;
  conf.n_points_average_waveform = 2;
  

  // Initialize the values of Ka
  for (int i=0; i<npixels; i++) {
    conf.Mn_Ka[i] = 1.0;
  }
  // get parameter from eventloader in conf file
  EUDAQ_INFO("Load configuration for OPAMP");
  // pass configuration via EUDAQ2 online monitor
  bool mon=conf_?conf_->HasSection("OPAMP"):false;
  if(mon){
    conf_->SetSection("OPAMP");
    EUDAQ_INFO("Set section `OPAMP` for online monitor");
  } else { // look for "identifier set by corry"
    // pass configuration via Corryvreckan
    std::string id=conf_?conf_->Get("identifier",""):""; // set by corry
    if(id!="") {
      size_t p=id.find("OPAMP_"); // TODO: no idea why the identifier is quoted in the config, may change in future?
      if(p==std::string::npos) {
        conf.device_n = -2; // other plane is looked for, bail out early to not waste time in full decoding
        confs[conf_]=std::move(conf);
        return confs[conf_]; // also here: early out: do not try opening the dump file
      }
      else {
        p+=6;
        conf.device_n=std::stoi(id.substr(p));
        EUDAQ_INFO("Set config `"+id+"` from Corryvreckan");
      }
    }
  }

  if(conf_) { // Reading OPAMP parameters from config file
    std::string name;
    // DAQ parameters
    name="sample_baseline_opamp";
    conf.sample_baseline_opamp = conf_->Get(name, 0); // get baseline parameter for DAQ signals from config. If no baseline specified, take 50
    EUDAQ_DEBUG(name+" set to "+std::to_string(conf.sample_baseline_opamp));
    name="first_sample_minimum_finder_opamp";
    conf.first_sample_minimum_finder_opamp = conf_->Get(name, conf.sample_baseline_opamp +1 ); // get minimum value of the sampling from from config. If no value is specified, take conf.sample_baseline_opamp + 1
    EUDAQ_DEBUG(name+" set to "+std::to_string(conf.first_sample_minimum_finder_opamp));
    name="last_sample_minimum_finder_opamp";
    conf.last_sample_minimum_finder_opamp = conf_->Get(name, 200); // get maximum value of the sampling from from config. If no value is specified, take 200
    EUDAQ_DEBUG(name+" set to "+std::to_string(conf.last_sample_minimum_finder_opamp));
    name="n_samples_after_min_signal_opamp";
    conf.n_samples_after_min_signal_opamp = conf_->Get(name, 0); 
    EUDAQ_DEBUG(name+" set to "+std::to_string(conf.n_samples_after_min_signal_opamp));
    name="n_samples_before_min_signal_opamp";
    conf.n_samples_before_min_signal_opamp = conf_->Get(name, 0);
    EUDAQ_DEBUG(name+" set to "+std::to_string(conf.n_samples_before_min_signal_opamp));
    name="daq_charge_conversion_factor_opamp";
    conf.daq_charge_conversion_factor_opamp = conf_->Get(name, 0.061); // get conversion factor to convert the charge from DAQ to fC
    EUDAQ_DEBUG(name+" set to "+std::to_string(conf.daq_charge_conversion_factor_opamp));
    // Scope parameters
    name="n_samples_baseline_opamp_scope";
    conf.n_samples_baseline_opamp_scope = conf_->Get(name, 100); // get baseline parameter for scope signals from config. If no baseline specified, take 50
    EUDAQ_DEBUG(name+" set to "+std::to_string(conf.n_samples_baseline_opamp_scope));
    name="n_samples_signal_opamp_scope";
    conf.n_samples_signal_opamp_scope = conf_->Get(name, 100); // get number of samples for the scope to compute the signal. If not specified, take 1
    EUDAQ_DEBUG(name+" set to "+std::to_string(conf.n_samples_signal_opamp_scope));
    name="baseline_shifting_from_t0_opamp_scope";
    conf.baseline_shifting_from_t0_opamp_scope = conf_->Get(name, 700);  // get number sample to shift back from t0 to compute the baseline 
    EUDAQ_DEBUG(name+" set to "+std::to_string(conf.baseline_shifting_from_t0_opamp_scope));
    name="underline_delay_from_t0_opamp_scope";
    conf.underline_delay_from_t0_opamp_scope = conf_->Get(name, 600); // get number sample to use as delay from t0 to compute the maximum amplitude 
    EUDAQ_DEBUG(name+" set to "+std::to_string(conf.underline_delay_from_t0_opamp_scope));
    name="threshold_cut_t0_finder_opamp_scope";
    conf.threshold_cut_t0_finder_opamp_scope = conf_->Get(name, 1387); // get threshold to cut the signal to find the t0
    EUDAQ_DEBUG(name+" set to "+std::to_string(conf.threshold_cut_t0_finder_opamp_scope));
    name="n_points_t0_finder_opamp_scope";
    conf.n_points_t0_finder_opamp_scope = conf_->Get(name, 700); // get number of points to use to find the t0
    EUDAQ_DEBUG(name+" set to "+std::to_string(conf.n_points_t0_finder_opamp_scope));
    name="scope_charge_conversion_factor_opamp_scope";
    conf.scope_charge_conversion_factor_opamp_scope = conf_->Get(name, 0.625); // get conversion factor to convert the charge from scope to fC
    EUDAQ_DEBUG(name+" set to "+std::to_string(conf.scope_charge_conversion_factor_opamp_scope));
    name="total_electron_from_source";
    conf.total_electron_from_source = conf_->Get(name, 1640); // get electrons from the source to calibrate the signal
    EUDAQ_DEBUG(name+" set to "+std::to_string(conf.total_electron_from_source));
    name="estimate_noise_daq";
    std::string estimation_daq = conf_->Get("estimate_noise_daq","false");
    conf.estimate_noise_daq = estimation_daq == "true";
    EUDAQ_DEBUG(name+" set to "+std::to_string(conf.estimate_noise_daq));
    name="estimate_noise_scope";
    std::string estimation_scope = conf_->Get("estimate_noise_scope","false");
    conf.estimate_noise_scope = estimation_scope == "true";
    EUDAQ_DEBUG(name+" set to "+std::to_string(conf.estimate_noise_scope));
    name="noise_t0_sample_scope";
    conf.noise_t0_sample_scope = conf_->Get(name, -1); // default value for t0 sample
    EUDAQ_DEBUG(name+" set to "+std::to_string(conf.noise_t0_sample_scope));
    name="threshold_lgad_signal";
    conf.threshold_lgad_signal = conf_->Get(name, 0.5); // default value for t0 sample
    EUDAQ_DEBUG(name+" set to "+std::to_string(conf.threshold_lgad_signal));
    name="osc_first_sample";
    conf.osc_first_sample = conf_->Get(name, 23); // first sample of scope data
    EUDAQ_DEBUG(name+" set to "+std::to_string(conf.osc_first_sample));
    name="check_LGAD_signal";
    conf.check_LGAD_signal = conf_->Get(name, false); // check if LGAD has signal
    EUDAQ_DEBUG(name+" set to "+std::to_string(conf.check_LGAD_signal));
    name="n_points_average_waveform";
    conf.n_points_average_waveform = conf_->Get(name, 2); //number of points to average
    EUDAQ_DEBUG(name+" set to "+std::to_string(conf.n_points_average_waveform));
    name="scope_ch1";
    conf.px_idx_scope[0] =conf_->Get(name, -1);  
    EUDAQ_DEBUG(name+" set to "+std::to_string(conf.px_idx_scope[0]));
    name="scope_ch2";
    conf.px_idx_scope[1] =conf_->Get(name, -1);  
    EUDAQ_DEBUG(name+" set to "+std::to_string(conf.px_idx_scope[1]));
    name="scope_ch3";
    conf.px_idx_scope[2] =conf_->Get(name, -1);  
    EUDAQ_DEBUG(name+" set to "+std::to_string(conf.px_idx_scope[2]));
    
    std::string filename = conf_->Get("calibration_file", ""); //ENERGY CALIBRATION FILE
    if (filename!="") {
      EUDAQ_INFO("Reading calibration file: "+filename);
      std::stringstream infile(filename.c_str());
      std::ifstream input(infile.str().c_str(), std::ifstream::in);
      if (!input.good()) {
        EUDAQ_WARN("Warning: No calibration file found in path: " + filename);
      } else {
        // Parse the file into a JSON object
        nlohmann::json j;
        input >> j;
        for (int idx_pixel = 0; idx_pixel < npixels; idx_pixel++) {
          std::string pixelKey = "pixel " + std::to_string(idx_pixel);
          // Check if the pixel key exists in the JSON
          if (j.contains(pixelKey) && j[pixelKey].contains("Ka_mean")) {
              double ka_mean = j[pixelKey]["Ka_mean"];
              conf.Mn_Ka[idx_pixel] = ka_mean;
          } else {
              std::cerr << "Pixel " << idx_pixel << " not found in the JSON file or it does not have 'Ka_mean'." << std::endl;
          }
        }
      }
    }

    // Initializing pixel coordinates based on px position
    
     for(int i=0; i<3; i++){
      if(conf.px_idx_scope[i]  == 5){
        conf.px_x_scope[i] = 1;
        conf.px_y_scope[i] = 1;
        }

        if(conf.px_idx_scope[i]  == 6){
        conf.px_x_scope[i] = 2;
        conf.px_y_scope[i] = 1;
        }

        if(conf.px_idx_scope[i]  == 9){
        conf.px_x_scope[i] = 1;
        conf.px_y_scope[i] = 2;
        }

        if (conf.px_idx_scope[i] > 0)
          conf.number_pixels_scope++;
      }

    if (conf.first_sample_minimum_finder_opamp > conf.last_sample_minimum_finder_opamp) {
      EUDAQ_THROW("Error: The lowest value allowed for the sampling frame (" +std::to_string(conf.first_sample_minimum_finder_opamp) +") exceeds the highest (" +std::to_string(conf.last_sample_minimum_finder_opamp) + ")");
    }

    if (conf.sample_baseline_opamp > conf.first_sample_minimum_finder_opamp - conf.n_samples_before_min_signal_opamp &&
        conf.sample_baseline_opamp < conf.first_sample_minimum_finder_opamp + conf.n_samples_after_min_signal_opamp) {
      EUDAQ_THROW("Error: The value used for the baseline estimation (" +std::to_string(conf.sample_baseline_opamp) +") is in the frame used for computing the signal");
    }

    if (conf.estimate_noise_scope && conf.noise_t0_sample_scope < conf.baseline_shifting_from_t0_opamp_scope) {
      EUDAQ_THROW("Error: Can't find the noise t0 sample (" +std::to_string(conf.noise_t0_sample_scope) +") before the baseline shifting from t0 (" +std::to_string(conf.baseline_shifting_from_t0_opamp_scope) +")");
    }
  }

  confs[conf_]=std::move(conf);
  return confs[conf_];
}
//Starting of actual CONVERTING

bool OPAMPRawEvent2StdEventConverter::Converting(eudaq::EventSPC in,
                                                 eudaq::StdEventSP out,
                                                 eudaq::ConfigSPC conf_) const {
  Config &conf=LoadConf(conf_);
  if(conf.device_n==-2) return false;
  auto rawev = std::dynamic_pointer_cast<const eudaq::RawEvent>(in);
  if(conf.device_n>=0 && conf.device_n!=rawev->GetDeviceN()) return false;
  if (rawev->GetNumBlock() != 4 && rawev->GetNumBlock() != 6 )
    return false; // TODO: how/can this happen?
  const std::vector<uint8_t> &block = rawev->GetBlock(0);
  size_t n = block.size();
  if (n < frame_size_in_byte || n % frame_size_in_byte != 0){  //check that block is multiple of 40
    EUDAQ_ERROR("Error: Incomplete Data Block. Block size is "+std::to_string(n)+", but should be multiple of 40");
    return false;
  }

  // common part for the same method with APTS-SF
  const uint8_t *data = block.data();
  eudaq::StandardPlane plane(rawev->GetDeviceN(), "ITS3DAQ", "OPAMP");
  if(n/frame_size_in_byte<conf.first_sample_minimum_finder_opamp) {
    EUDAQ_ERROR("Error: The minimum value allowed for the sampling frame ("+std::to_string(conf.first_sample_minimum_finder_opamp)+") exceeds total number of samples ("+std::to_string(n/frame_size_in_byte)+")");
    return false;
  }

  if(conf.last_sample_minimum_finder_opamp>n/frame_size_in_byte-conf.n_samples_after_min_signal_opamp) {
    EUDAQ_ERROR("Error: The highest value allowed for the sampling frame ("+std::to_string(conf.last_sample_minimum_finder_opamp)+") exceeds total number of samples ("+std::to_string(n/frame_size_in_byte)+") - the number of frames used after the sampling frame ("+std::to_string(conf.n_samples_after_min_signal_opamp)+")");
    return false;
  }
  // BASELINE
  if(n/frame_size_in_byte<conf.sample_baseline_opamp) {
    EUDAQ_ERROR("Error: Baseline sample number ("+std::to_string(conf.sample_baseline_opamp)+") exceeds total number of samples ("+std::to_string(n/frame_size_in_byte)+")");
    return false;
  }
  uint16_t baseline[npixels];
  unscramble(data+conf.sample_baseline_opamp*frame_size_in_byte, baseline);

  // SAMPLING POINT (=minimum)
  int imin=0;
  float vmin=1e6;
  for(int is=conf.first_sample_minimum_finder_opamp;is!=conf.last_sample_minimum_finder_opamp+1;++is) {
    uint16_t adcs[npixels];
    unscramble(data+is*frame_size_in_byte,adcs);
    for(int i=0;i<npixels;++i) {
      float v=adcs[i]-baseline[i];
      if(v<vmin) {vmin=v;imin=is;} //vmin = value in adc of lowest point, imin is the number of sampling point corresponding to the minimum
    }
  }
  // SIGNAL EXTRACTION (=integral)
  if(n/frame_size_in_byte<imin+conf.n_samples_after_min_signal_opamp) {
    EUDAQ_ERROR("Error: Targeted samples for signal extraction after minimum ("+std::to_string(imin+conf.n_samples_after_min_signal_opamp)+") exceeds total number of samples ("+std::to_string(n/frame_size_in_byte)+")");
    return false;
    }
  if(0>imin-conf.n_samples_before_min_signal_opamp) {
    EUDAQ_ERROR("Error: Targeted samples for signal extraction before minimum ("+std::to_string(imin-conf.n_samples_before_min_signal_opamp)+") is smaller than 0");
    return false;
    }
  float signal[npixels]={0};
  int is_in = imin-conf.n_samples_before_min_signal_opamp;  //starting ns_b points before minimum (up until ns_a points after minimum)
  int is_fin = imin+conf.n_samples_after_min_signal_opamp+1;
  if(conf.estimate_noise_daq){ //save the pixel values before the frames used for the estimation of the baseline
    is_in = 2*conf.sample_baseline_opamp-imin-conf.n_samples_after_min_signal_opamp;
    is_fin = 2*conf.sample_baseline_opamp-imin+conf.n_samples_before_min_signal_opamp+1;
  }

  for(int is=is_in; is!=is_fin; ++is) {
    uint16_t adcs[npixels];
    unscramble(data+is*frame_size_in_byte,adcs);
    for(int i=0;i!=npixels;++i) {
      float v=adcs[i]-baseline[i];
      signal[i]+=v;
    }
  }
  for(int i=0;i!=npixels;++i) {
      signal[i] /= (1+conf.n_samples_before_min_signal_opamp+conf.n_samples_after_min_signal_opamp);
      signal[i] = (signal[i]*conf.total_electron_from_source)/conf.Mn_Ka[i];
  }
  plane.SetSizeRaw(4,4);
  for(int y=0;y!=4;++y){ 
    for(int x=0;x!=4;++x){
    if(conf.estimate_noise_scope && !conf.estimate_noise_daq) plane.SetPixel((y*4)+x, x, y, std::numeric_limits<float>::max()); // prevent to have a 0 peak from the daq pixels
    else plane.SetPixel((y*4)+x, x, y, -signal[y*4+x]*conf.daq_charge_conversion_factor_opamp*conf.scope_charge_conversion_factor_opamp_scope); // equalized and converted in mV
    }
  }
  // Signal from the scope
  int tot_scope_channels = conf.number_pixels_scope;
  if(conf.check_LGAD_signal) tot_scope_channels = conf.number_pixels_scope + 1;
  std::vector<std::vector<uint8_t>> raw_block_osch; // vector's vector
  for(int i_ch =0; i_ch != tot_scope_channels; i_ch++){
  std::vector<uint8_t> temp_vec = rawev->GetBlock(i_ch+2); // temp vector inside the for loop
  raw_block_osch.push_back(temp_vec); 
  }

  //find t0 
  int osc_t0[conf.number_pixels_scope];
  for (int i = 0; i != conf.number_pixels_scope; ++i){
    if (!conf.estimate_noise_scope){
     osc_t0[i] = find_edge(raw_block_osch[i].data(), conf.threshold_cut_t0_finder_opamp_scope, conf.n_points_t0_finder_opamp_scope); //this is the frame number. The threshold and number of points have been optimized in dedicated study.
    }
    else osc_t0[i]=conf.noise_t0_sample_scope; //set artifically t0 sample in a region in which there's no signal
  }

  // BASELINE -> line
  float osc_baseline[conf.number_pixels_scope];
  bool hasSignalLGAD = false;
  for (int i = 0; i != conf.number_pixels_scope; ++i){
    osc_baseline[i] = WaveformAverage<uint8_t, int8_t>(raw_block_osch[i].data(),conf.n_samples_baseline_opamp_scope,osc_t0[i]-conf.baseline_shifting_from_t0_opamp_scope);
  }

  for (int i = 0; i != conf.number_pixels_scope; ++i){
    if (osc_t0[i] < 0) signal[i] = 0; // without t0 amplitude is meaningless. //use -9999 for noise measurements
    else signal[i] = WaveformAverage<uint8_t, int8_t>(raw_block_osch[i].data(),conf.n_samples_signal_opamp_scope,osc_t0[i]+conf.underline_delay_from_t0_opamp_scope)-osc_baseline[i];
    signal[i] = (signal[i]*conf.total_electron_from_source)/conf.Mn_Ka[conf.px_idx_scope[i]]; //signal converted in electrons
    plane.SetPixel(conf.px_idx_scope[i],conf.px_x_scope[i],conf.px_y_scope[i], -signal[i] * conf.scope_charge_conversion_factor_opamp_scope,(uint64_t)osc_t0[i]); // pixel readout via scope, 0.625mV is the scope amplitude unit 
    if(conf.estimate_noise_daq && !conf.estimate_noise_scope) plane.SetPixel(conf.px_idx_scope[i],conf.px_x_scope[i],conf.px_y_scope[i], std::numeric_limits<float>::max()); // to prevent to have a 0 peak from the scope pixels
  }
  // LGAD Check
  if(conf.check_LGAD_signal) {
    auto newWF = GetAveragedWaveform(raw_block_osch[tot_scope_channels-1].data(), raw_block_osch[tot_scope_channels-1].size(), conf.n_points_average_waveform, conf.osc_first_sample);

    hasSignalLGAD = HasLGADSignal(newWF.data(), conf.threshold_lgad_signal/conf.scope_charge_conversion_factor_opamp_scope, conf.osc_first_sample);
  }
  if(conf.check_LGAD_signal && !hasSignalLGAD) return false;

  out->AddPlane(plane);
  return true;
}