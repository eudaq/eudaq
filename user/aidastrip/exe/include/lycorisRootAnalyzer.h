/*
 * mengqing.wu@desy.de @ 2018-03-05
 * based on : https://github.com/Lycoris2017/KPiX-Analysis/blob/master/src/analysis.cxx
 * -- for lycoris telescope funded by AIDA2020 located at DESY/;
 * -- currently only work to map 1 single kpix on the sensor.
 */
#include <iostream>
#include <fstream>
#include <unordered_map>

#include "KpixEvent_v1.h"
#include "KpixSample_v1.h"
#include "KpixMap.h"

#include "TFile.h"
#include "TH1F.h"
#include "TH2F.h"

using namespace std;

//---header started----//
class lycorisRootAnalyzer{
public:
  lycorisRootAnalyzer(std::string outfile_path);
  ~lycorisRootAnalyzer();
  int Analyzing (KpixEvent& CycleEvent, uint eudaqEventN);
  int Closing ();
  
  void Setdebug(){_debug=true;};
  
private:
  bool               _debug;
  
  stringstream       _tmp;
  KpixSample         *_sample;
  uint               _cycleCount;
  std::vector<bool>  _kpixfound;
  
  TFile*             _rfile;
  unordered_map<uint, uint> _kpix2strip;
  
  // Global histogram definitions:
  std::unordered_map< uint, TH1F*>  _chn_entries_k; // key is kpix, entries for all buckets
  std::unordered_map< uint, TH1F*>  _strip_entries_k;
  
  // Event-by-event histograms:
  std::unordered_map< uint, TH2F*> _chn_vs_adc_b;
    
  std::unordered_map< uint, TH2F*>  _chn_vs_adc_b0_k; // key is k-kpix
  std::unordered_map< uint, TH2F*>  _chn_vs_adc_b1_k; // key is k-kpix
  std::unordered_map< uint, TH2F*>  _chn_vs_adc_b2_k; // key is k-kpix
  std::unordered_map< uint, TH2F*>  _chn_vs_adc_b3_k; // key is k-kpix

  std::unordered_map< uint, TH1F*>  _strip_entries_b0_k;
  std::unordered_map< uint, TH1F*>  _strip_entries_b1_k;
  //std::unordered_map< uint, TH1F*>  _strip_entries_b2_k;
  
  std::unordered_map< uint, TH2F*>  _strip_vs_adc_b0_k;
  std::unordered_map< uint, TH2F*>  _strip_vs_adc_b1_k; 

};
//---header finished----//

lycorisRootAnalyzer::lycorisRootAnalyzer(std::string outfile_path){
  // constructor
  if ( outfile_path.empty() ) outfile_path = "lycorisAna_Test.root";
  string outfile_type = outfile_path.substr(outfile_path.find_last_of(".")+1);
  if ( outfile_type != "root" ) outfile_path+=".root";
    
  _rfile = new TFile(outfile_path.c_str(), "recreate");

  _kpixfound = std::vector<bool>(32, false);
  _cycleCount = 0;
  _tmp.str("");
  _debug = false;
  
  _kpix2strip = map_kpix_to_strip();
  
}

lycorisRootAnalyzer::~lycorisRootAnalyzer(){ 
  /* deconstructor
   * -- please do not run deconstructor immediately after the Closing(),
   *    you may encounter unexpected pointer problem
   */
  
  for (auto& kv: _chn_entries_k ){
    delete kv.second;
  }
  
  if ( _rfile ){
    if ( _rfile->IsOpen()) {
      std::cout << "[lycorisRootAna] Closing output file = " << _rfile->GetName() << " ..." << std::endl;
      _rfile->Close();
    }
  }
  delete _rfile;
  
}

int lycorisRootAnalyzer::Closing (){
  cout << "\n[lycorisRootAna] # of Cycles counting ==> " << _cycleCount << endl;

  for (auto& kv: _chn_entries_k )
    kv.second->Scale(1./_cycleCount);
  for (auto& sv: _strip_entries_k) sv.second->Scale(1./_cycleCount);
  for (auto& sv0: _strip_entries_b0_k) sv0.second->Scale(1./_cycleCount);
  
  // debug for the duplicate channel to strip id:
  if (_debug){
    for (auto& dd: _strip_entries_k){
      int nbins = dd.second->GetSize();
      TAxis *xaxis = dd.second->GetXaxis();
      for (int nbin = 1; nbin<nbins-1; nbin++){
	auto strip_evts=dd.second->GetBinContent(nbin);
	if (strip_evts>4) {
	  auto strip_ID=xaxis->GetBinCenter(nbin);
	  cout<< "[debug] stripId = "<< strip_ID
	      << ", entries = "<<strip_evts<<endl;
	}
      }
      
    }
  }
    
    
  _rfile->Write();
  cout << "[lycorisRootAna] Write to output file = " << _rfile->GetName() << " ..." << endl;
  _rfile->Close();
  
  return 1;
}

int lycorisRootAnalyzer::Analyzing (KpixEvent& CycleEvent, uint eudaqEventN){

  _cycleCount++;
  
  uint kpix, channel, bucket, value, tstamp;
  KpixSample::SampleType type;

      
  if (eudaqEventN==0){
    std::cout << "[RootAna] kpixEvent.evtNum = " << CycleEvent.eventNumber()
	      << "\n\t kpixEvent.timestamp = "<< CycleEvent.timestamp() 
	      << "\n\t kpixEvent.count = "<< CycleEvent.count() << std::endl;
  }


  /* read kpix sample -> which is particle event */
  for (uint xx=0; xx<CycleEvent.count(); xx++){
    //// Get sample
    _sample  = CycleEvent.sample(xx);  // check event subtructure
    if (_sample->getEmpty()) continue; // if empty jump over
    
    kpix    = _sample->getKpixAddress();
    channel = _sample->getKpixChannel();
    bucket  = _sample->getKpixBucket();
    type    = _sample->getSampleType();
    value   = _sample->getSampleValue(); // adc value
    tstamp  = _sample->getSampleTime();
    
    if (type == KpixSample::Data) {

      //-- Prepare Hists:
      if ( !_kpixfound.at(kpix) ) {
	_kpixfound.at(kpix) = true;
	std::cout<<"[dev] kpix = "<<kpix<<", channel = " <<channel <<", bucket = " <<bucket <<"\n";
	
	_tmp.str("");
	_tmp << "chn_entries_k" << kpix << "_allBucket";
	_chn_entries_k.insert ( { kpix, new TH1F(_tmp.str().c_str(), "Channel Entries; kpix channel addr.; #OfEvts/#acq.cycles", 1024, -0.5, 1023.5) } );
	std::cout<< " success in creating: " << _tmp.str() << std::endl;

	_tmp.str("");
	_tmp << "strip_entries_k" << kpix << "_allBucket";
	_strip_entries_k.insert ({kpix, new TH1F(_tmp.str().c_str(), "Strip entries; Strip ID; #OfEvts/#acq.cycles", 920, 0.5, 920.5)});
	std::cout<< " success in creating: " << _tmp.str() << std::endl;
	
	// bucket 0:
	_tmp.str("");
	_tmp <<  "chn_vs_adc_b0_k" << kpix ;
	_chn_vs_adc_b0_k.insert({kpix, new TH2F(_tmp.str().c_str(), "Channel Entries; kpix channel addr.; Charge (ADC)", 1024, -0.5, 1023.5, 8192, -0.5,8191.5)});
	std::cout<< " success in creating: " << _tmp.str() << std::endl;

	_tmp.str("");
	_tmp <<  "strip_entries_b0_k" << kpix ;
	_strip_entries_b0_k.insert({kpix, new TH1F(_tmp.str().c_str(), "Strip entries; Strip ID; #OfEvts/#acq.cycles", 920, 0.5, 920.5)});
	std::cout<< " success in creating: " << _tmp.str() << std::endl;

	_tmp.str("");
	_tmp << "strip_vs_adc_b0_k" << kpix;
	_strip_vs_adc_b0_k.insert({kpix, new TH2F(_tmp.str().c_str(), "Strip vs ADC; Strip ID; Charge (ADC)", 920, 0.5, 920.5, 8192, -0.5, 8191.5)});
	
	// bucket 1
	_tmp.str("");
	_tmp <<  "chn_vs_adc_b1_k" << kpix ;
	_chn_vs_adc_b1_k.insert({kpix, new TH2F(_tmp.str().c_str(), "Channel vs ADC; kpix channel addr.; Charge (ADC)", 1024, -0.5, 1023.5, 8192, -0.5,8191.5)});
	std::cout<< " success in creating: " << _tmp.str() << std::endl;

	_tmp.str("");
	_tmp <<  "strip_entries_b1_k" << kpix ;
	_strip_entries_b1_k.insert({kpix, new TH1F(_tmp.str().c_str(), "Strip entries; Strip ID; #OfEvts/#acq.cycles", 920, 0.5, 920.5)});
	std::cout<< " success in creating: " << _tmp.str() << std::endl;
	
	_tmp.str("");
	_tmp << "strip_vs_adc_b1_k" << kpix;
	_strip_vs_adc_b1_k.insert({kpix, new TH2F(_tmp.str().c_str(), "Strip vs ADC; Strip ID; Charge (ADC)", 920, 0.5, 920.5, 8192, -0.5, 8191.5)});
	
	// bucket 2
	_tmp.str("");
	_tmp <<  "chn_vs_adc_b2_k" << kpix ;
	_chn_vs_adc_b2_k.insert({kpix, new TH2F(_tmp.str().c_str(), "Channel vs ADC; kpix channel addr.; Charge (ADC)", 1024, -0.5, 1023.5, 8192, -0.5,8191.5)});
	std::cout<< " success in creating: " << _tmp.str() << std::endl;

	// bucket 3
	_tmp.str("");
	_tmp <<  "chn_vs_adc_b3_k" << kpix ;
	_chn_vs_adc_b2_k.insert({kpix, new TH2F(_tmp.str().c_str(), "Channel vs ADC; kpix channel addr.; Charge (ADC)", 1024, -0.5, 1023.5, 8192, -0.5,8191.5)});
	std::cout<< " success in creating: " << _tmp.str() << std::endl;

      }

      //-- fill hists:
      _chn_entries_k.at(kpix)->Fill(channel);
      _strip_entries_k.at(kpix)->Fill( _kpix2strip.at(channel) );
	
      switch (bucket) {
      case 0:
	_chn_vs_adc_b0_k.at(kpix)-> Fill(channel, value);
	_strip_vs_adc_b0_k.at(kpix)-> Fill(_kpix2strip.at(channel), value);
	_strip_entries_b0_k.at(kpix)-> Fill( _kpix2strip.at(channel) );
	break;
	
      case 1:
	_chn_vs_adc_b1_k.at(kpix)-> Fill(channel, value);
	_strip_vs_adc_b1_k.at(kpix)-> Fill(_kpix2strip.at(channel), value);
	_strip_entries_b1_k.at(kpix)-> Fill( _kpix2strip.at(channel) );
	break;
	
      case 2:
	_chn_vs_adc_b2_k.at(kpix)-> Fill(channel, value);
	break;
	
      case 3:
	_chn_vs_adc_b3_k.at(kpix)-> Fill(channel, value);

	break;
	
      default:
	cout<< " warn: weird bucket = "<< bucket << endl;
	break;
      }


    }
    
  }

  
  return 1;

}
