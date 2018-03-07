/*
 * mengqing.wu@desy.de @ 2018-03-05
 * based on : https://github.com/Lycoris2017/KPiX-Analysis/blob/master/src/analysis.cxx
 * -- for lycoris telescope funded by AIDA2020 located at DESY/
 */
#include <iostream>
#include <fstream>
#include <unordered_map>

#include "KpixEvent.h"
#include "KpixSample.h"
#include "kpixmap.h"

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
private:
  stringstream                      _tmp;
  KpixSample   *_sample;
  uint         _cycleCount;
  std::vector<bool> _kpixfound;
  
  TFile*                            _rfile;

  // Global histogram definitions:
  std::unordered_map< uint, TH1F*>  _chn_entries_k; // key is kpix, entries for all buckets
 
  // Event-by-event histograms:
  std::unordered_map< uint, TH2F*>  _chn_vs_adc_b0_k; // key is k-kpix
  std::unordered_map< uint, TH2F*>  _chn_vs_adc_b1_k; // key is k-kpix
  std::unordered_map< uint, TH2F*>  _chn_vs_adc_b2_k; // key is k-kpix

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
      std::cout << "[lycorisRootAna] Closing output file = " << _rfile->GetName() << "..." << std::endl;
      _rfile->Close();
    }
  }
  delete _rfile;
  
}


int lycorisRootAnalyzer::Closing (){
  cout << "\n[lycorisRootAna] # of Cycles counting ==> " << _cycleCount << endl;

  for (auto& kv: _chn_entries_k ){
    kv.second->Scale(1./_cycleCount);
  }
  
  
  _rfile->Write();
  cout << "[lycorisRootAna] Write to output file = " << _rfile->GetName() << "..." << endl;
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

      if ( !_kpixfound.at(kpix) ) {
	_kpixfound.at(kpix) = true;
	std::cout<<"[dev] kpix = "<<kpix<<", channel = " <<channel <<", bucket = " <<bucket <<"\n";
	
	_tmp.str("");
	_tmp << "chn_entries_k" << kpix << "_allBucket";
	_chn_entries_k.insert ( { kpix, new TH1F(_tmp.str().c_str(), "Channel Entries; kpix channel addr.; #OfEvts/#acq.cycles", 1024, -0.5, 1023.5) } );
	std::cout<< " success in creating: " << _tmp.str() << std::endl;;

	_tmp.str("");
	_tmp <<  "chn_vs_adc_b0_k" << kpix ;
	_chn_vs_adc_b0_k.insert({kpix, new TH2F(_tmp.str().c_str(), "Channel Entries; kpix channel addr.; Charge (ADC)", 1024, -0.5, 1023.5, 8192, -0.5,8191.5)});
	std::cout<< " success in creating: " << _tmp.str() << std::endl;

	_tmp.str("");
	_tmp <<  "chn_vs_adc_b1_k" << kpix ;
	_chn_vs_adc_b1_k.insert({kpix, new TH2F(_tmp.str().c_str(), "Channel Entries; kpix channel addr.; Charge (ADC)", 1024, -0.5, 1023.5, 8192, -0.5,8191.5)});
	std::cout<< " success in creating: " << _tmp.str() << std::endl;

	_tmp.str("");
	_tmp <<  "chn_vs_adc_b2_k" << kpix ;
	_chn_vs_adc_b2_k.insert({kpix, new TH2F(_tmp.str().c_str(), "Channel Entries; kpix channel addr.; Charge (ADC)", 1024, -0.5, 1023.5, 8192, -0.5,8191.5)});
	std::cout<< " success in creating: " << _tmp.str() << std::endl;
	
      }

      // fill hists:
      _chn_entries_k.at(kpix)->Fill(channel);

      switch (bucket) {
      case 0:
	_chn_vs_adc_b0_k.at(kpix)-> Fill(channel, value);
	break;
	
      case 1:
	_chn_vs_adc_b1_k.at(kpix)-> Fill(channel, value);
	break;
	
      case 2:
	_chn_vs_adc_b2_k.at(kpix)-> Fill(channel, value);
	break;
	
      case 3:
	break;
	
      default:
	cout<< " warn: weird bucket = "<< bucket << endl;
	break;
      }


    }
    
  }

  
  return 1;

}
