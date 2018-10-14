#include <TROOT.h>
#include "HexagonHistos.hh"
#include "OnlineMon.hh"
#include "TCanvas.h"
#include "TGraph.h"
#include <cstdlib>
#include <sstream>

const int nSCA = 13;

HexagonHistos::HexagonHistos(eudaq::StandardPlane p, RootMonitor *mon)
:_sensor(p.Sensor()), _id(p.ID()), _maxX(p.XSize()),  _maxY(p.YSize()), filling_counter(0), _wait(false),
  _hexagons_occ_selection(NULL), _hexagons_occ_adc(NULL), _hexagons_occ_tot(NULL), _hexagons_occ_toa(NULL), _hexagons_charge(NULL),
  _hit2Dmap(NULL), _BadPixelMap(NULL), _hit1Docc(NULL), _TOAvsChId(NULL),
  _nHits(NULL), _nbadHits(NULL), _nHotPixels(NULL),
  _waveformLG(NULL), _waveformHG(NULL), _waveformNormLG(NULL), _waveformNormHG(NULL),
  _posOfMaxADCinLG(NULL), _posOfMaxADCinHG(NULL),
  _sigAdcLG(NULL), _sigAdcHG(NULL), _pedLG(NULL), _pedHG(NULL),
  _LGvsTOTfast(NULL), _LGvsTOTslow(NULL), _HGvsLG(NULL){


  char out[1024], out2[1024];
  
  _mon = mon;
  
  _runMode = _mon->mon_configdata.getRunMode();
  //std::cout << "HexagonHistos::Sensorname: " << _sensor << " "<< _id<< std::endl;
  //std::cout <<"runMode = "<<_runMode<<std::endl;

  std::string sel("");
  if (_runMode==0) sel="PED";
  else if (_runMode==1) sel="TOA";
  else if (_runMode==2) sel="MIP";
  else sel="PED";
  
  sprintf(out, "%s::%02d, Occupancy based on %s", _sensor.c_str(), _id, sel.c_str());
  sprintf(out2, "h_hexagons_occ_selection_%s_%02d", _sensor.c_str(), _id);
  _hexagons_occ_selection = get_th2poly(out2,out);
  
  sprintf(out, "%s::%02d,  ADC_HG Occupancy", _sensor.c_str(), _id);
  sprintf(out2, "h_hexagons_occ_adc_%s_%02d", _sensor.c_str(), _id);
  _hexagons_occ_adc = get_th2poly(out2,out);

  sprintf(out, "%s::%02d,  TOT (slow) Occupancy", _sensor.c_str(), _id);
  sprintf(out2, "h_hexagons_occ_tot_%s_%02d", _sensor.c_str(), _id);
  _hexagons_occ_tot = get_th2poly(out2,out);

  sprintf(out, "%s::%02d,  TOA (fall) Occupancy", _sensor.c_str(), _id);
  sprintf(out2, "h_hexagons_occ_toa_%s_%02d", _sensor.c_str(), _id);
  _hexagons_occ_toa = get_th2poly(out2,out);


  sprintf(out, "%s::%02d,  ADC HG Charge", _sensor.c_str(), _id);
  sprintf(out2, "h_hexagons_charge_%s_%02d", _sensor.c_str(), _id);
  _hexagons_charge = get_th2poly(out2,out);


  sprintf(out, "%s::%02d, 1D Hit occupancy", _sensor.c_str(), _id);
  sprintf(out2, "h_hit1Docc_%s_%02d", _sensor.c_str(), _id);
  _hit1Docc = new TH1I(out2, out, 256, 0, 256);
  SetHistoAxisLabelx(_hit1Docc, "(SkiRoc ID * 64 ) + ChID");


  sprintf(out, "%s::%02d, Signal at LG", _sensor.c_str(), _id);
  sprintf(out2, "h_sigAdcLG_TS3_%s_%02d", _sensor.c_str(), _id);
  _sigAdcLG = new TH1I(out2, out, 200, -50, 500);
  SetHistoAxisLabelx(_sigAdcLG, "LG (peak) - PED, ADC counts");

  sprintf(out, "%s::%02d, Signal at HG", _sensor.c_str(), _id);
  sprintf(out2, "h_sigAdcHG_TS3_%s_%02d", _sensor.c_str(), _id);
  _sigAdcHG = new TH1I(out2, out, 500, -100, 2600);
  SetHistoAxisLabelx(_sigAdcHG, "HG (peak) - PED, ADC counts");

  
  sprintf(out, "%s::%02d, Pedestal LG", _sensor.c_str(), _id);
  sprintf(out2, "h_pedLG_%s_%02d", _sensor.c_str(), _id);
  _pedLG = new TH1I(out2, out, 100, 0, 350);
  SetHistoAxisLabelx(_pedLG, "LG ADC counts");

  sprintf(out, "%s::%02d, Pedestal HG", _sensor.c_str(), _id);
  sprintf(out2, "h_pedHG_%s_%02d", _sensor.c_str(), _id);
  _pedHG = new TH1I(out2, out, 100, 0, 400);
  SetHistoAxisLabelx(_pedHG, "HG ADC counts");
  
  if (_maxX != -1 && _maxY != -1) {
    sprintf(out, "%s::%02d, Raw Hitmap", _sensor.c_str(), _id);
    sprintf(out2, "h_hit2Dmap_%s_%02d", _sensor.c_str(), _id);
    _hit2Dmap = new TH2I(out2, out, _maxX + 1, 0, _maxX, _maxY + 1, 0, _maxY);
    SetHistoAxisLabels(_hit2Dmap, "SkiRoc ID", "Channel ID");
    
    sprintf(out, "%s::%02d, Suspicious Pixels", _sensor.c_str(), _id);
    sprintf(out2, "h_badpixelmap_%s_%02d", _sensor.c_str(), _id);
    _BadPixelMap = new TH2D(out2, out, _maxX + 1, 0, _maxX, _maxY + 1, 0, _maxY);
    SetHistoAxisLabels(_BadPixelMap, "SkiRoc ID", "Channel ID");
  }

  sprintf(out, "%s::%02d, Number of Hits", _sensor.c_str(), _id);
  sprintf(out2, "h_raw_nHits_%s_%02d", _sensor.c_str(), _id);
  _nHits = new TH1I(out2, out, 50, 0, 50);
  SetHistoAxisLabelx(_nHits, "Number of Hits above ZS");
  //_nHits->SetStats(1);

  sprintf(out, "%s::%02d, Number of Invalid Hits", _sensor.c_str(), _id);
  sprintf(out2, "h_nbadHits_%s_%02d", _sensor.c_str(), _id);
  _nbadHits = new TH1I(out2, out, 50, 0, 50);
  SetHistoAxisLabelx(_nbadHits, "n_{BadHits}");

  sprintf(out, "%s::%02d, Number of Hot Pixels", _sensor.c_str(), _id);
  sprintf(out2, "h_nhotpixels_%s_%02d", _sensor.c_str(), _id);
  _nHotPixels = new TH1I(out2, out, 50, 0, 50);
  SetHistoAxisLabelx(_nHotPixels, "n_{HotPixels}");



  // ---------
  // Waveforms
  // ---------
  sprintf(out, "%s-%02d Waveform LG", _sensor.c_str(), _id);
  sprintf(out2, "h_waveform_LG_%s_%02d", _sensor.c_str(), _id);
  _waveformLG = new TH2I(out2, out, 2*nSCA, 0, nSCA, 100, 0, 1500);
  SetHistoAxisLabels(_waveformLG, "Time Sample of 25 ns", "LG ADC");

  sprintf(out, "%s::%02d, Waveform HG", _sensor.c_str(), _id);
  sprintf(out2, "h_waveform_HG_%s_%02d", _sensor.c_str(), _id);
  _waveformHG = new TH2I(out2, out, 2*nSCA, 0, nSCA, 100, 0, 2600);
  SetHistoAxisLabels(_waveformHG, "Time Sample of 25 ns", "HG ADC");


  sprintf(out, "%s::%02d, Waveform LG Norm", _sensor.c_str(), _id);
  sprintf(out2, "p_waveform_LG_%s_%02d", _sensor.c_str(), _id);
  _waveformNormLG = new TProfile(out2, out, 2*nSCA, 0, nSCA);
  SetHistoAxisLabels(_waveformNormLG, "Time Sample of 25 ns", "Normalized");

  sprintf(out, "%s::%02d, Waveform HG Norm", _sensor.c_str(), _id);
  sprintf(out2, "p_waveform_HG_%s_%02d", _sensor.c_str(), _id);
  _waveformNormHG = new TProfile(out2, out, 2*nSCA, 0, nSCA);
  SetHistoAxisLabels(_waveformNormHG, "Time Sample of 25 ns", "Normalized");


  sprintf(out, "%s::%02d, TS of Maximum at LG", _sensor.c_str(), _id);
  sprintf(out2, "h_posOfMaxADC_LG_%s_%02d", _sensor.c_str(), _id);
  _posOfMaxADCinLG = new TH1I(out2, out, 2*nSCA, 0, nSCA);
  SetHistoAxisLabels(_posOfMaxADCinLG, "Time Sample of 25 ns","Events");

  sprintf(out, "%s::%02d, TS of Maximum at HG", _sensor.c_str(), _id);
  sprintf(out2, "h_posOfMaxADC_HG_%s_%02d", _sensor.c_str(), _id);
  _posOfMaxADCinHG = new TH1I(out2, out, 2*nSCA, 0, nSCA);
  SetHistoAxisLabels(_posOfMaxADCinHG, "Time Sample of 25 ns","Events");

  sprintf(out, "%s::%02d, LG vs TOT (fast)", _sensor.c_str(), _id);
  sprintf(out2, "h_LGvsTOTfast_%s_%02d", _sensor.c_str(), _id);
  _LGvsTOTfast = new TH2I(out2, out, 20, 0, 4100, 60, 0, 2000);
  SetHistoAxisLabels(_LGvsTOTfast, "TOT (fast) ADC", "LG ADC");

  sprintf(out, "%s::%02d, LG vs TOT (slow)", _sensor.c_str(), _id);
  sprintf(out2, "h_LGvsTOTslow_%s_%02d", _sensor.c_str(), _id);
  _LGvsTOTslow = new TH2I(out2, out, 20, 0, 800, 60, 0, 2000);
  SetHistoAxisLabels(_LGvsTOTslow, "TOT (slow) ADC", "LG ADC");

  sprintf(out, "%s::%02d, HG vs LG", _sensor.c_str(), _id);
  sprintf(out2, "h_HGvsLG_%s_%02d", _sensor.c_str(), _id);
  _HGvsLG = new TH2I(out2, out, 200, 0, 2000, 200, 0, 4100);
  SetHistoAxisLabels(_HGvsLG, "LG ADC", "HG ADC");

  sprintf(out, "%s::%02d, TOA vs Channel", _sensor.c_str(), _id);
  sprintf(out2, "h_TOAvsChId_%s_%02d", _sensor.c_str(), _id);
  _TOAvsChId = new TH2I(out2, out, 256, 0, 256, 60, 1000, 3000);
  SetHistoAxisLabels(_TOAvsChId, "(SkiRoc ID * 64 ) + ChID", "TOA (fall), ADC");




  // Parameters that are read-out from configuration file:

  mainFrameTS = _mon->mon_configdata.getMainFrameTS();
  thresh_LG = _mon->mon_configdata.getThreshLG();
  thresh_HG = _mon->mon_configdata.getThreshHG();
  
  //const int tmp_int = _mon->mon_configdata.getDqmColorMap();
  //std::cout<<"DQM value from config file: "<<tmp_int<<std::endl;

  
  Set_SkiToHexaboard_ChannelMap();
}


void HexagonHistos::Fill(const eudaq::StandardPlane &plane, int evNumber) {
  //std::cout<< "FILL with a plane." << std::endl;

  
  int nHit=0, nHot=0, nBad=0;
  
  // Temporary let's just not fill events with too many channels 
  /* if (plane.HitPixels()>250 && _runMode!=0) */
  /*   return; */


  // This one needs to be reset every event:
  if (_hexagons_charge != NULL && plane.HitPixels()>0) {
    //_hexagons_charge->Reset("");
    for (int icell = 0; icell < 133 ; icell++)
      _hexagons_charge->SetBinContent(icell+1, 0);
  }


  for (unsigned int pix = 0; pix < plane.HitPixels(); pix++)
    {
      const int pixel_x = plane.GetX(pix);
      const int pixel_y = plane.GetY(pix);
      const int ch  = _ski_to_ch_map.find(make_pair(pixel_x,pixel_y))->second;

      
      // ----- Maskig noisy channels ----
      // These are noisy pixels in most hexaboards. Let's just mask them from DQM:
      if ( pixel_x==0 && pixel_y==22 )
      	continue;

      if ( pixel_x==3 && pixel_y==44 )
      	continue;

      /* // Masking for June 2018 beam tests: */

      // Wholle chip 3 is bad on this board
      if ( (_sensor=="HexaBoard-RDB00" && _id==0 ) && 
       	   pixel_x==3)  
       	continue; 


      /* // Masking for October 2018 beam tests: */

      /* // Mask 3, 44 in many modules */
      /* if ( (_sensor=="HexaBoard-RDB1" && _id==1 ) && */
      /* 	   pixel_x==3 && pixel_y==44) */
      /* 	continue; */
      /* if ( (_sensor=="HexaBoard-RDB1" && _id==2 ) && */
      /* 	   pixel_x==3 && pixel_y==44) */
      /* 	continue; */
      /* if ( (_sensor=="HexaBoard-RDB1" && _id==3 ) && */
      /* 	   pixel_x==3 && pixel_y==44) */
      /* 	continue; */
      /* if ( (_sensor=="HexaBoard-RDB2" && _id==0 ) && */
      /* 	   pixel_x==3 && pixel_y==44) */
      /* 	continue; */
      /* if ( (_sensor=="HexaBoard-RDB2" && _id==4 ) && */
      /* 	   pixel_x==3 && pixel_y==44) */
      /* 	continue; */
      /* if ( (_sensor=="HexaBoard-RDB2" && _id==6 ) && */
      /* 	   pixel_x==3 && pixel_y==44) */
      /* 	continue; */
      /* if ( (_sensor=="HexaBoard-RDB2" && _id==7 ) && */
      /* 	   pixel_x==3 && pixel_y==44) */
      /* 	continue; */
      /* if ( (_sensor=="HexaBoard-RDB3" && _id==0 ) && */
      /* 	   pixel_x==3 && pixel_y==44) */
      /* 	continue; */
      /* if ( (_sensor=="HexaBoard-RDB3" && _id==3 ) && */
      /* 	   pixel_x==3 && pixel_y==44) */
      /* 	continue; */
      /* if ( (_sensor=="HexaBoard-RDB3" && _id==6 ) && */
      /* 	   pixel_x==3 && pixel_y==44) */
      /* 	continue; */
      /* if ( (_sensor=="HexaBoard-RDB4" && _id==1 ) && */
      /* 	   pixel_x==3 && pixel_y==44) */
      /* 	continue; */

      /* // others */
      /* if ( (_sensor=="HexaBoard-RDB2" && _id==5 ) && */
      /* 	   pixel_x==3 && pixel_y==12) */
      /* 	continue; */
      /* if ( (_sensor=="HexaBoard-RDB2" && _id==6 ) && */
      /* 	   pixel_x==3 && pixel_y==12) */
      /* 	continue; */
      /* if ( (_sensor=="HexaBoard-RDB3" && _id==3 ) && */
      /* 	   pixel_x==3 && pixel_y==12) */
      /* 	continue; */
      /* if ( (_sensor=="HexaBoard-RDB4" && _id==3 ) && */
      /* 	   ( (pixel_x==3 && pixel_y==12) || (pixel_x==3 && pixel_y==44) ) ) */
      /* 	continue; */
      /* if ( (_sensor=="HexaBoard-RDB4" && _id==6 ) && */
      /* 	   ( (pixel_x==0 && pixel_y==28) || (pixel_x==3 && pixel_y==44) ) ) */
      /* 	continue; */


      /*
      // Below are masking of the modules used in 2017:
      //if ( pixel_x==0 && pixel_y==22 )
      //continue;
      if ( (_sensor=="HexaBoard-RDB1" && _id==0 ) &&
	   pixel_x==3 && (pixel_y==32 || pixel_y==48))
	continue;

      // May module:
      if ( (_sensor=="HexaBoard-RDB1" && _id==0 ) &&
	   ( (pixel_x==0 && pixel_y==0 )  || (pixel_x==0 && pixel_y==2 ) ||
	     (pixel_x==0 && pixel_y==4 )  || (pixel_x==0 && pixel_y==6 ) ||
	     (pixel_x==1 && pixel_y==0 ) ))
	continue;

      // Module #63
      if ( (_sensor=="HexaBoard-RDB1" && _id==3 ) &&
	   ( pixel_x==0 && pixel_y==58 ) || ( pixel_x==0 && pixel_y==22 ) )
	continue;

      // Module #75
      if ( (_sensor=="HexaBoard-RDB1" && _id==4 ) &&
	   ( pixel_x==0 && pixel_y==58 ) )
	continue;

      // Module #38
      if ( _sensor=="HexaBoard-RDB2" && _id==0 &&
	   ( pixel_x==0 && pixel_y==58 ) || ( pixel_x==0 && pixel_y==22 ) )
	continue;

      // Module #46
      if ( _sensor=="HexaBoard-RDB2" && _id==2 &&
	   ( pixel_x==0 && pixel_y==58 ) )
	continue;
      
      // Module #62
      if ( _sensor=="HexaBoard-RDB2" && _id==5 &&
	   ( pixel_x==0 && pixel_y==58 ) )
	continue;

      // Module #53:
      if ( _sensor=="HexaBoard-RDB2" && _id==1 &&
	   ( pixel_x==0 && pixel_y==58 ) )
	continue;

      // Module #55:
      if ( _sensor=="HexaBoard-RDB2" && _id==6 &&
	   ( pixel_x==0 && pixel_y==58 ) )
	continue;

      // All Modules on readout board 3
      if ( _sensor=="HexaBoard-RDB3" &&
	   ( pixel_x==0 && pixel_y==58 ) )
	continue;

      // Module #42
      if ( _sensor=="HexaBoard-RDB3" && _id==3 &&
	   ( pixel_x==1 && pixel_y==4 ) )
	continue;

      // Module #39
      if ( _sensor=="HexaBoard-RDB3" && _id==4 &&
	   ( pixel_x==0 && pixel_y==22 ) )
	continue;

      // Module #64
      if ( _sensor=="HexaBoard-RDB3" && _id==6 &&
	   ( pixel_x==0 && pixel_y==22 ) )
	continue;

      */
      // -------- end masking -------------

      //std::cout<<" We are getting a pixel with pix="<<pix
      //       <<"\t in hexagon channel:"<<ch<<",  ski="<<pixel_x<<"  Ch="<<pixel_y<<std::endl;

      // Arrays to store Time Samples
      std::array<int, nSCA> sig_LG;
      std::array<int, nSCA> sig_HG;

      for (int ts=0; ts<nSCA; ts++){
	sig_LG[ts] = plane.GetPixel(pix, ts);
	sig_HG[ts] = plane.GetPixel(pix, ts+nSCA);
      }

      //std::cout<<"Average TOA of the board ("<<_sensor<<","<<_id<<") is "<< plane.GetPixel(0, 30) << std::endl;

      // Suppress noizy Time Samples:
      sig_LG[9]  /= 10;
      sig_LG[10] /= 10;
      sig_HG[9]  /= 10;
      sig_HG[10] /= 10;
      
      const auto max_LG = std::max_element(std::begin(sig_LG), std::end(sig_LG));
      const int pos_max_LG = std::distance(std::begin(sig_LG), max_LG);
      //std::cout << "Max element in LG is " << *max_LG << " at position " << pos_max_LG << std::endl;

      const auto max_HG = std::max_element(std::begin(sig_HG), std::end(sig_HG));
      const int pos_max_HG = std::distance(std::begin(sig_HG), max_HG);
      //std::cout << "Max element in HG is " << *max_HG << " at position " << pos_max_HG << std::endl;


      // Get pedestal estimates from the first time samples:
      //const int ped_LG = std::accumulate(sig_LG.begin(), sig_LG.begin()+2, 0)/2; // average of the first two TS
      //const int ped_HG = std::accumulate(sig_HG.begin(), sig_HG.begin()+2, 0)/2; // average of the first two TS
      // Pedestals from the first (0th) TS:
      const int ped_LG = sig_LG[0]; 
      const int ped_HG = sig_HG[0]; 
      
      if (_pedLG!=NULL)
	_pedLG->Fill(ped_LG);
      if (_pedHG!=NULL)
	_pedHG->Fill(ped_HG);

      if (_posOfMaxADCinLG!=NULL && (*max_LG) - ped_LG > thresh_LG)
	_posOfMaxADCinLG->Fill(pos_max_LG);
      if (_posOfMaxADCinHG!=NULL && (*max_HG) - ped_HG > thresh_HG)
	_posOfMaxADCinHG->Fill(pos_max_HG);

      // Average of three TS around main frame:
      //const int avg_LG = std::accumulate(sig_LG.begin()+mainFrameTS-1, sig_LG.begin()+mainFrameTS+2, 0)/3; 
      const int avg_HG = std::accumulate(sig_HG.begin()+mainFrameTS-1, sig_HG.begin()+mainFrameTS+2, 0)/3;

      const int peak_LG = sig_LG[mainFrameTS];
      const int peak_HG = sig_HG[mainFrameTS];
      
      if (_sigAdcLG!=NULL)
	_sigAdcLG->Fill(peak_LG - ped_LG);

      if (_sigAdcHG!=NULL)
	_sigAdcHG->Fill(peak_HG - ped_HG);
      
      const int toa_fall = plane.GetPixel(pix, 26);
      const int toa_rise = plane.GetPixel(pix, 27);
      const int tot_fast = plane.GetPixel(pix, 28);
      const int tot_slow = plane.GetPixel(pix, 29);


      if (_waveformHG!=NULL && _waveformLG!=NULL &&_waveformNormHG!=NULL && _waveformNormLG!=NULL){
	
	// Only fill these if maximum is above some threshold (ie, it is signal)
	if ( (*max_LG) - ped_LG > thresh_LG) {
	  for (int ts=0; ts<nSCA; ts++){
	    _waveformLG->Fill(ts, plane.GetPixel(pix, ts));
	    _waveformNormLG->Fill(ts, (float)plane.GetPixel(pix, ts)/(*max_LG));
	  }
	}
	if ( (*max_HG) - ped_HG > thresh_HG) {
	  for (int ts=0; ts<nSCA; ts++){
	    _waveformHG->Fill(ts, plane.GetPixel(pix, nSCA+ts));
	    _waveformNormHG->Fill(ts, (float)plane.GetPixel(pix, nSCA+ts)/(*max_HG));
	  }
	}
      }


      if (_hit2Dmap != NULL)
	_hit2Dmap->Fill(pixel_x, pixel_y);
      if (_hit1Docc != NULL)
	_hit1Docc->Fill(pixel_x*64+pixel_y);


      if ( (*max_LG) > 4000 || (*max_HG) > 4000 ){
	nHot+=1;
	if (_BadPixelMap != NULL)
	  _BadPixelMap->Fill(pixel_x, pixel_y);
      }

      
      if ( (toa_rise==4 || toa_fall==4) && _BadPixelMap != NULL)
	// This is bad because the hits are slected based on HA bit, which should be equivalent to TOA
	_BadPixelMap->Fill(pixel_x, pixel_y);
	            

      if (_LGvsTOTfast != NULL)
	_LGvsTOTfast->Fill(tot_fast, peak_LG);
      if (_LGvsTOTslow != NULL)
	_LGvsTOTslow->Fill(tot_slow, peak_LG);
      if (_HGvsLG != NULL)
	_HGvsLG->Fill(peak_LG, peak_HG);
      if (_TOAvsChId != NULL)
	_TOAvsChId->Fill(pixel_x*64+pixel_y, toa_fall);
	  
      // Loop over the bins and Fill the one matched to our channel
      for (int icell = 0; icell < 133 ; icell++) {

	const int bin = ch_to_bin_map[icell];

	//std::cout<<" pixel_x = "<<pixel_x <<"  pixel_y="<<pixel_y
	//	 <<"  channel = "<<ch<<"  icell="<<icell<<"  bin="<<bin<<std::endl;
	if (bin == ch){
	  //std::cout<<"Our bin matched the channel: "<<bin<<std::endl;

	  if (_hexagons_charge!=NULL)
	    _hexagons_charge->SetBinContent(icell+1, peak_HG);

	  //char buffer_bin[3]; sprintf(buffer_bin,"%d", (char)(icell+1));
	  std::ostringstream oss;  oss << "Sensor_" << icell+1;
          const string bin_name = oss.str();

          if ((avg_HG - ped_HG) > thresh_HG && _hexagons_occ_adc!=NULL)
            _hexagons_occ_adc->Fill(bin_name.c_str(), 1);

          if (tot_slow!=4 &&_hexagons_occ_tot!=NULL)
            _hexagons_occ_tot->Fill(bin_name.c_str(), 1);

          if (toa_fall!=4 && _hexagons_occ_toa!=NULL)
            _hexagons_occ_toa->Fill(bin_name.c_str(), 1);

          if (_hexagons_occ_selection!=NULL)
            _hexagons_occ_selection->Fill(bin_name.c_str(), 1);

	}
      }
      nHit++;

    }
  
  if (_nHits != NULL){
    _nHits->Fill(nHit);
    handleOverflowBins(_nHits);
  }
  
  //if ((_nbadHits != NULL)) {
  //_nbadHits->Fill(2);
  //}
  
  if (_nHotPixels != NULL)
    _nHotPixels->Fill(nHot);


  if (_hexagons_charge!=NULL && evNumber%100==0)
    ev_display_list->Add(_hexagons_charge->Clone(Form("%s_%02d_HG_Display_Event_%05i",
						      _sensor.c_str(), _id,
						      evNumber)));
  // We need to increase the counter once per event.
  // Since this Fill() method is done once per plane, let's just increment it at zeros plane
  //if (plane.Sensor()=="HexaBoard-RDB2" && plane.ID()==0)
  //filling_counter += 1;
 
}

void HexagonHistos::Reset() {
  _hexagons_occ_selection->Reset("");
  _hexagons_occ_adc->Reset("");
  _hexagons_occ_tot->Reset("");
  _hexagons_occ_toa->Reset("");
  _hexagons_charge->Reset("");
  _hit2Dmap->Reset();
  _hit1Docc->Reset();

  _nHits->Reset();
  _nbadHits->Reset();
  _nHotPixels->Reset();
  _BadPixelMap->Reset();

  _waveformLG->Reset();
  _waveformHG->Reset();
  _waveformNormLG->Reset();
  _waveformNormHG->Reset();

  _posOfMaxADCinLG->Reset();
  _posOfMaxADCinHG->Reset();

  _pedLG->Reset();
  _pedHG->Reset();

  _sigAdcLG->Reset();
  _sigAdcHG->Reset();

  _LGvsTOTfast->Reset();
  _LGvsTOTslow->Reset();
  _HGvsLG->Reset();
  _TOAvsChId->Reset();
  
}

void HexagonHistos::Calculate(const int currentEventNum) {
  _wait = true;

  _wait = false;
}

void HexagonHistos::Write() {
  _hexagons_occ_selection->Write();
  _hexagons_occ_adc->Write();
  _hexagons_occ_tot->Write();
  _hexagons_occ_toa->Write();
  _hit2Dmap->Write();
  _hit1Docc->Write();

  handleOverflowBins(_nHits);
  _nHits->Write();
  _nbadHits->Write();
  _BadPixelMap->Write();
  _nHotPixels->Write();

  _waveformLG->Write();
  _waveformHG->Write();

  _waveformNormLG->Write();
  _waveformNormHG->Write();

  _posOfMaxADCinLG->Write();
  _posOfMaxADCinHG->Write();

  handleOverflowBins(_pedLG);
  handleOverflowBins(_pedHG);
  _pedLG->Write();
  _pedHG->Write();

  handleOverflowBins(_sigAdcLG);
  handleOverflowBins(_sigAdcHG);
  _sigAdcLG->Write();
  _sigAdcHG->Write();

  _LGvsTOTfast->Write();
  _LGvsTOTslow->Write();
  _HGvsLG->Write();
  _TOAvsChId->Write();

  
  TIter next(ev_display_list);
  while (TObject *obj = next()){
    obj->Write();
  }
  
  ev_display_list->Clear();

  //std::cout<<"Doing HexagonHistos::Write() before canvas drawing"<<std::endl;

  /*
  gSystem->Sleep(100);

  gROOT->SetBatch(kTRUE);
  TCanvas *tmpcan = new TCanvas("tmpcan","Canvas for PNGs",600,600);
  tmpcan->cd();

  _hexagons_occ_adc->Draw("COLZ TEXT");
  tmpcan->SaveAs("../snapshots/Occupancy_ADC_HG.png");

  _hexagons_occ_tot->Draw("COLZ TEXT");
  tmpcan->SaveAs("../snapshots/Occupancy_TOT.png");

  _hexagons_occ_toa->Draw("COLZ TEXT");
  tmpcan->SaveAs("../snapshots/Occupancy_TOA.png");

  _nHits->Draw("hist");
  tmpcan->SaveAs("../snapshots/nHits.png");

  tmpcan->Close();
  gROOT->SetBatch(kFALSE);

  */

  //std::cout<<"Doing HexagonHistos::Write() after canvas drawing"<<std::endl;

}

int HexagonHistos::SetHistoAxisLabelx(TH1 *histo, string xlabel) {
  if (histo != NULL) {
    histo->GetXaxis()->SetTitle(xlabel.c_str());
  }
  return 0;
}

int HexagonHistos::SetHistoAxisLabels(TH1 *histo, string xlabel, string ylabel) {
  SetHistoAxisLabelx(histo, xlabel);
  SetHistoAxisLabely(histo, ylabel);

  return 0;
}

int HexagonHistos::SetHistoAxisLabely(TH1 *histo, string ylabel) {
  if (histo != NULL) {
    histo->GetYaxis()->SetTitle(ylabel.c_str());
  }
  return 0;
}

void HexagonHistos::Set_SkiToHexaboard_ChannelMap(){

  for(int c=0; c<127; c++){
    int row = c*3;
    _ski_to_ch_map.insert(make_pair(make_pair(sc_to_ch_map[row+1],sc_to_ch_map[row+2]),sc_to_ch_map[row]));
  }
}

TH2Poly* HexagonHistos::get_th2poly(string name, string title) {
   TH2Poly* hitmapoly = new TH2Poly(name.c_str(), title.c_str(), 25, -7.14598, 7.14598, 25, -6.1886, 6.188);
   SetHistoAxisLabels(hitmapoly, "X [cm]", "Y [cm]");
   Double_t Graph_fx1[4] = {
   6.171528,
   6.496345,
   7.14598,
   6.496345};
   Double_t Graph_fy1[4] = {
   0.5626,
   1.166027e-08,
   1.282629e-08,
   1.1252};
   TGraph *graph = new TGraph(4,Graph_fx1,Graph_fy1);
   graph->SetName("Sensor_1");
   graph->SetTitle("Sensor_1");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx2[4] = {
   6.171528,
   6.496345,
   7.14598,
   6.496345};
   Double_t Graph_fy2[4] = {
   -0.5626,
   -1.1252,
   1.282629e-08,
   1.166027e-08};
   graph = new TGraph(4,Graph_fx2,Graph_fy2);
   graph->SetName("Sensor_2");
   graph->SetTitle("Sensor_2");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx3[4] = {
   5.197076,
   5.521893,
   6.171528,
   5.521893};
   Double_t Graph_fy3[4] = {
   2.2504,
   1.6878,
   1.6878,
   2.813};
   graph = new TGraph(4,Graph_fx3,Graph_fy3);
   graph->SetName("Sensor_3");
   graph->SetTitle("Sensor_3");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx4[6] = {
   5.197076,
   5.521893,
   6.171528,
   6.496345,
   6.171528,
   5.521893};
   Double_t Graph_fy4[6] = {
   1.1252,
   0.5626,
   0.5626,
   1.1252,
   1.6878,
   1.6878};
   graph = new TGraph(6,Graph_fx4,Graph_fy4);
   graph->SetName("Sensor_4");
   graph->SetTitle("Sensor_4");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx5[6] = {
   5.197076,
   5.521893,
   6.171528,
   6.496345,
   6.171528,
   5.521893};
   Double_t Graph_fy5[6] = {
   9.328214e-09,
   -0.5626,
   -0.5626,
   1.166027e-08,
   0.5626,
   0.5626};
   graph = new TGraph(6,Graph_fx5,Graph_fy5);
   graph->SetName("Sensor_5");
   graph->SetTitle("Sensor_5");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx6[6] = {
   5.197076,
   5.521893,
   6.171528,
   6.496345,
   6.171528,
   5.521893};
   Double_t Graph_fy6[6] = {
   -1.1252,
   -1.6878,
   -1.6878,
   -1.1252,
   -0.5626,
   -0.5626};
   graph = new TGraph(6,Graph_fx6,Graph_fy6);
   graph->SetName("Sensor_6");
   graph->SetTitle("Sensor_6");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx7[4] = {
   5.197076,
   5.521893,
   6.171528,
   5.521893};
   Double_t Graph_fy7[4] = {
   -2.2504,
   -2.813,
   -1.6878,
   -1.6878};
   graph = new TGraph(4,Graph_fx7,Graph_fy7);
   graph->SetName("Sensor_7");
   graph->SetTitle("Sensor_7");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx8[4] = {
   4.222624,
   4.547442,
   5.197076,
   4.547442};
   Double_t Graph_fy8[4] = {
   3.9382,
   3.3756,
   3.3756,
   4.5008};
   graph = new TGraph(4,Graph_fx8,Graph_fy8);
   graph->SetName("Sensor_8");
   graph->SetTitle("Sensor_8");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx9[6] = {
   4.222624,
   4.547442,
   5.197076,
   5.521893,
   5.197076,
   4.547442};
   Double_t Graph_fy9[6] = {
   2.813,
   2.2504,
   2.2504,
   2.813,
   3.3756,
   3.3756};
   graph = new TGraph(6,Graph_fx9,Graph_fy9);
   graph->SetName("Sensor_9");
   graph->SetTitle("Sensor_9");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx10[6] = {
   4.222624,
   4.547442,
   5.197076,
   5.521893,
   5.197076,
   4.547442};
   Double_t Graph_fy10[6] = {
   1.6878,
   1.1252,
   1.1252,
   1.6878,
   2.2504,
   2.2504};
   graph = new TGraph(6,Graph_fx10,Graph_fy10);
   graph->SetName("Sensor_10");
   graph->SetTitle("Sensor_10");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx11[6] = {
   4.222624,
   4.547442,
   5.197076,
   5.521893,
   5.197076,
   4.547442};
   Double_t Graph_fy11[6] = {
   0.5626,
   8.162187e-09,
   9.328214e-09,
   0.5626,
   1.1252,
   1.1252};
   graph = new TGraph(6,Graph_fx11,Graph_fy11);
   graph->SetName("Sensor_11");
   graph->SetTitle("Sensor_11");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx12[6] = {
   4.222624,
   4.547441,
   5.197076,
   5.521893,
   5.197076,
   4.547442};
   Double_t Graph_fy12[6] = {
   -0.5626,
   -1.1252,
   -1.1252,
   -0.5626,
   9.328214e-09,
   8.162187e-09};
   graph = new TGraph(6,Graph_fx12,Graph_fy12);
   graph->SetName("Sensor_12");
   graph->SetTitle("Sensor_12");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx13[6] = {
   4.222624,
   4.547441,
   5.197076,
   5.521893,
   5.197076,
   4.547441};
   Double_t Graph_fy13[6] = {
   -1.6878,
   -2.2504,
   -2.2504,
   -1.6878,
   -1.1252,
   -1.1252};
   graph = new TGraph(6,Graph_fx13,Graph_fy13);
   graph->SetName("Sensor_13");
   graph->SetTitle("Sensor_13");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx14[6] = {
   4.222624,
   4.547441,
   5.197076,
   5.521893,
   5.197076,
   4.547441};
   Double_t Graph_fy14[6] = {
   -2.813,
   -3.3756,
   -3.3756,
   -2.813,
   -2.2504,
   -2.2504};
   graph = new TGraph(6,Graph_fx14,Graph_fy14);
   graph->SetName("Sensor_14");
   graph->SetTitle("Sensor_14");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx15[4] = {
   4.222624,
   4.547441,
   5.197076,
   4.547441};
   Double_t Graph_fy15[4] = {
   -3.9382,
   -4.5008,
   -3.3756,
   -3.3756};
   graph = new TGraph(4,Graph_fx15,Graph_fy15);
   graph->SetName("Sensor_15");
   graph->SetTitle("Sensor_15");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx16[4] = {
   3.248173,
   3.57299,
   4.222624,
   3.57299};
   Double_t Graph_fy16[4] = {
   5.626,
   5.0634,
   5.0634,
   6.1886};
   graph = new TGraph(4,Graph_fx16,Graph_fy16);
   graph->SetName("Sensor_16");
   graph->SetTitle("Sensor_16");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx17[6] = {
   3.248173,
   3.57299,
   4.222624,
   4.547442,
   4.222624,
   3.57299};
   Double_t Graph_fy17[6] = {
   4.5008,
   3.9382,
   3.9382,
   4.5008,
   5.0634,
   5.0634};
   graph = new TGraph(6,Graph_fx17,Graph_fy17);
   graph->SetName("Sensor_17");
   graph->SetTitle("Sensor_17");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx18[6] = {
   3.248173,
   3.57299,
   4.222624,
   4.547442,
   4.222624,
   3.57299};
   Double_t Graph_fy18[6] = {
   3.3756,
   2.813,
   2.813,
   3.3756,
   3.9382,
   3.9382};
   graph = new TGraph(6,Graph_fx18,Graph_fy18);
   graph->SetName("Sensor_18");
   graph->SetTitle("Sensor_18");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx19[6] = {
   3.248173,
   3.57299,
   4.222624,
   4.547442,
   4.222624,
   3.57299};
   Double_t Graph_fy19[6] = {
   2.2504,
   1.6878,
   1.6878,
   2.2504,
   2.813,
   2.813};
   graph = new TGraph(6,Graph_fx19,Graph_fy19);
   graph->SetName("Sensor_19");
   graph->SetTitle("Sensor_19");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx20[6] = {
   3.248173,
   3.57299,
   4.222624,
   4.547442,
   4.222624,
   3.57299};
   Double_t Graph_fy20[6] = {
   1.1252,
   0.5626,
   0.5626,
   1.1252,
   1.6878,
   1.6878};
   graph = new TGraph(6,Graph_fx20,Graph_fy20);
   graph->SetName("Sensor_20");
   graph->SetTitle("Sensor_20");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx21[6] = {
   3.248173,
   3.57299,
   4.222624,
   4.547442,
   4.222624,
   3.57299};
   Double_t Graph_fy21[6] = {
   5.830134e-09,
   -0.5626,
   -0.5626,
   8.162187e-09,
   0.5626,
   0.5626};
   graph = new TGraph(6,Graph_fx21,Graph_fy21);
   graph->SetName("Sensor_21");
   graph->SetTitle("Sensor_21");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx22[6] = {
   3.248172,
   3.57299,
   4.222624,
   4.547441,
   4.222624,
   3.57299};
   Double_t Graph_fy22[6] = {
   -1.1252,
   -1.6878,
   -1.6878,
   -1.1252,
   -0.5626,
   -0.5626};
   graph = new TGraph(6,Graph_fx22,Graph_fy22);
   graph->SetName("Sensor_22");
   graph->SetTitle("Sensor_22");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx23[6] = {
   3.248172,
   3.57299,
   4.222624,
   4.547441,
   4.222624,
   3.57299};
   Double_t Graph_fy23[6] = {
   -2.2504,
   -2.813,
   -2.813,
   -2.2504,
   -1.6878,
   -1.6878};
   graph = new TGraph(6,Graph_fx23,Graph_fy23);
   graph->SetName("Sensor_23");
   graph->SetTitle("Sensor_23");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx24[6] = {
   3.248172,
   3.57299,
   4.222624,
   4.547441,
   4.222624,
   3.57299};
   Double_t Graph_fy24[6] = {
   -3.3756,
   -3.9382,
   -3.9382,
   -3.3756,
   -2.813,
   -2.813};
   graph = new TGraph(6,Graph_fx24,Graph_fy24);
   graph->SetName("Sensor_24");
   graph->SetTitle("Sensor_24");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx25[6] = {
   3.248172,
   3.57299,
   4.222624,
   4.547441,
   4.222624,
   3.57299};
   Double_t Graph_fy25[6] = {
   -4.5008,
   -5.0634,
   -5.0634,
   -4.5008,
   -3.9382,
   -3.9382};
   graph = new TGraph(6,Graph_fx25,Graph_fy25);
   graph->SetName("Sensor_25");
   graph->SetTitle("Sensor_25");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx26[4] = {
   3.248172,
   3.57299,
   4.222624,
   3.57299};
   Double_t Graph_fy26[4] = {
   -5.626,
   -6.1886,
   -5.0634,
   -5.0634};
   graph = new TGraph(4,Graph_fx26,Graph_fy26);
   graph->SetName("Sensor_26");
   graph->SetTitle("Sensor_26");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx27[4] = {
   2.273721,
   2.598538,
   3.248173,
   3.57299};
   Double_t Graph_fy27[4] = {
   6.1886,
   5.626,
   5.626,
   6.1886};
   graph = new TGraph(4,Graph_fx27,Graph_fy27);
   graph->SetName("Sensor_27");
   graph->SetTitle("Sensor_27");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx28[6] = {
   2.273721,
   2.598538,
   3.248173,
   3.57299,
   3.248173,
   2.598538};
   Double_t Graph_fy28[6] = {
   5.0634,
   4.5008,
   4.5008,
   5.0634,
   5.626,
   5.626};
   graph = new TGraph(6,Graph_fx28,Graph_fy28);
   graph->SetName("Sensor_28");
   graph->SetTitle("Sensor_28");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx29[6] = {
   2.273721,
   2.598538,
   3.248173,
   3.57299,
   3.248173,
   2.598538};
   Double_t Graph_fy29[6] = {
   3.9382,
   3.3756,
   3.3756,
   3.9382,
   4.5008,
   4.5008};
   graph = new TGraph(6,Graph_fx29,Graph_fy29);
   graph->SetName("Sensor_29");
   graph->SetTitle("Sensor_29");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx30[6] = {
   2.273721,
   2.598538,
   3.248173,
   3.57299,
   3.248173,
   2.598538};
   Double_t Graph_fy30[6] = {
   2.813,
   2.2504,
   2.2504,
   2.813,
   3.3756,
   3.3756};
   graph = new TGraph(6,Graph_fx30,Graph_fy30);
   graph->SetName("Sensor_30");
   graph->SetTitle("Sensor_30");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx31[6] = {
   2.273721,
   2.598538,
   3.248173,
   3.57299,
   3.248173,
   2.598538};
   Double_t Graph_fy31[6] = {
   1.6878,
   1.1252,
   1.1252,
   1.6878,
   2.2504,
   2.2504};
   graph = new TGraph(6,Graph_fx31,Graph_fy31);
   graph->SetName("Sensor_31");
   graph->SetTitle("Sensor_31");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx32[6] = {
   2.273721,
   2.598538,
   3.248173,
   3.57299,
   3.248173,
   2.598538};
   Double_t Graph_fy32[6] = {
   0.5626,
   4.664107e-09,
   5.830134e-09,
   0.5626,
   1.1252,
   1.1252};
   graph = new TGraph(6,Graph_fx32,Graph_fy32);
   graph->SetName("Sensor_32");
   graph->SetTitle("Sensor_32");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx33[6] = {
   2.273721,
   2.598538,
   3.248172,
   3.57299,
   3.248173,
   2.598538};
   Double_t Graph_fy33[6] = {
   -0.5626,
   -1.1252,
   -1.1252,
   -0.5626,
   5.830134e-09,
   4.664107e-09};
   graph = new TGraph(6,Graph_fx33,Graph_fy33);
   graph->SetName("Sensor_33");
   graph->SetTitle("Sensor_33");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx34[6] = {
   2.273721,
   2.598538,
   3.248172,
   3.57299,
   3.248172,
   2.598538};
   Double_t Graph_fy34[6] = {
   -1.6878,
   -2.2504,
   -2.2504,
   -1.6878,
   -1.1252,
   -1.1252};
   graph = new TGraph(6,Graph_fx34,Graph_fy34);
   graph->SetName("Sensor_34");
   graph->SetTitle("Sensor_34");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx35[6] = {
   2.273721,
   2.598538,
   3.248172,
   3.57299,
   3.248172,
   2.598538};
   Double_t Graph_fy35[6] = {
   -2.813,
   -3.3756,
   -3.3756,
   -2.813,
   -2.2504,
   -2.2504};
   graph = new TGraph(6,Graph_fx35,Graph_fy35);
   graph->SetName("Sensor_35");
   graph->SetTitle("Sensor_35");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx36[6] = {
   2.273721,
   2.598538,
   3.248172,
   3.57299,
   3.248172,
   2.598538};
   Double_t Graph_fy36[6] = {
   -3.9382,
   -4.5008,
   -4.5008,
   -3.9382,
   -3.3756,
   -3.3756};
   graph = new TGraph(6,Graph_fx36,Graph_fy36);
   graph->SetName("Sensor_36");
   graph->SetTitle("Sensor_36");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx37[6] = {
   2.273721,
   2.598538,
   3.248172,
   3.57299,
   3.248172,
   2.598538};
   Double_t Graph_fy37[6] = {
   -5.0634,
   -5.626,
   -5.626,
   -5.0634,
   -4.5008,
   -4.5008};
   graph = new TGraph(6,Graph_fx37,Graph_fy37);
   graph->SetName("Sensor_37");
   graph->SetTitle("Sensor_37");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx38[4] = {
   2.273721,
   3.57299,
   3.248172,
   2.598538};
   Double_t Graph_fy38[4] = {
   -6.1886,
   -6.1886,
   -5.626,
   -5.626};
   graph = new TGraph(4,Graph_fx38,Graph_fy38);
   graph->SetName("Sensor_38");
   graph->SetTitle("Sensor_38");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx39[6] = {
   1.299269,
   1.624086,
   2.273721,
   2.598538,
   2.273721,
   1.624086};
   Double_t Graph_fy39[6] = {
   5.626,
   5.0634,
   5.0634,
   5.626,
   6.1886,
   6.1886};
   graph = new TGraph(6,Graph_fx39,Graph_fy39);
   graph->SetName("Sensor_39");
   graph->SetTitle("Sensor_39");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx40[6] = {
   1.299269,
   1.624086,
   2.273721,
   2.598538,
   2.273721,
   1.624086};
   Double_t Graph_fy40[6] = {
   4.5008,
   3.9382,
   3.9382,
   4.5008,
   5.0634,
   5.0634};
   graph = new TGraph(6,Graph_fx40,Graph_fy40);
   graph->SetName("Sensor_40");
   graph->SetTitle("Sensor_40");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx41[6] = {
   1.299269,
   1.624086,
   2.273721,
   2.598538,
   2.273721,
   1.624086};
   Double_t Graph_fy41[6] = {
   3.3756,
   2.813,
   2.813,
   3.3756,
   3.9382,
   3.9382};
   graph = new TGraph(6,Graph_fx41,Graph_fy41);
   graph->SetName("Sensor_41");
   graph->SetTitle("Sensor_41");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx42[6] = {
   1.299269,
   1.624086,
   2.273721,
   2.598538,
   2.273721,
   1.624086};
   Double_t Graph_fy42[6] = {
   2.2504,
   1.6878,
   1.6878,
   2.2504,
   2.813,
   2.813};
   graph = new TGraph(6,Graph_fx42,Graph_fy42);
   graph->SetName("Sensor_42");
   graph->SetTitle("Sensor_42");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx43[6] = {
   1.299269,
   1.624086,
   2.273721,
   2.598538,
   2.273721,
   1.624086};
   Double_t Graph_fy43[6] = {
   1.1252,
   0.5626,
   0.5626,
   1.1252,
   1.6878,
   1.6878};
   graph = new TGraph(6,Graph_fx43,Graph_fy43);
   graph->SetName("Sensor_43");
   graph->SetTitle("Sensor_43");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx44[6] = {
   1.299269,
   1.624086,
   2.273721,
   2.598538,
   2.273721,
   1.624086};
   Double_t Graph_fy44[6] = {
   2.332053e-09,
   -0.5626,
   -0.5626,
   4.664107e-09,
   0.5626,
   0.5626};
   graph = new TGraph(6,Graph_fx44,Graph_fy44);
   graph->SetName("Sensor_44");
   graph->SetTitle("Sensor_44");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx45[6] = {
   1.299269,
   1.624086,
   2.273721,
   2.598538,
   2.273721,
   1.624086};
   Double_t Graph_fy45[6] = {
   -1.1252,
   -1.6878,
   -1.6878,
   -1.1252,
   -0.5626,
   -0.5626};
   graph = new TGraph(6,Graph_fx45,Graph_fy45);
   graph->SetName("Sensor_45");
   graph->SetTitle("Sensor_45");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx46[6] = {
   1.299269,
   1.624086,
   2.273721,
   2.598538,
   2.273721,
   1.624086};
   Double_t Graph_fy46[6] = {
   -2.2504,
   -2.813,
   -2.813,
   -2.2504,
   -1.6878,
   -1.6878};
   graph = new TGraph(6,Graph_fx46,Graph_fy46);
   graph->SetName("Sensor_46");
   graph->SetTitle("Sensor_46");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx47[6] = {
   1.299269,
   1.624086,
   2.273721,
   2.598538,
   2.273721,
   1.624086};
   Double_t Graph_fy47[6] = {
   -3.3756,
   -3.9382,
   -3.9382,
   -3.3756,
   -2.813,
   -2.813};
   graph = new TGraph(6,Graph_fx47,Graph_fy47);
   graph->SetName("Sensor_47");
   graph->SetTitle("Sensor_47");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx48[6] = {
   1.299269,
   1.624086,
   2.273721,
   2.598538,
   2.273721,
   1.624086};
   Double_t Graph_fy48[6] = {
   -4.5008,
   -5.0634,
   -5.0634,
   -4.5008,
   -3.9382,
   -3.9382};
   graph = new TGraph(6,Graph_fx48,Graph_fy48);
   graph->SetName("Sensor_48");
   graph->SetTitle("Sensor_48");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx49[6] = {
   1.299269,
   1.624086,
   2.273721,
   2.598538,
   2.273721,
   1.624086};
   Double_t Graph_fy49[6] = {
   -5.626,
   -6.1886,
   -6.1886,
   -5.626,
   -5.0634,
   -5.0634};
   graph = new TGraph(6,Graph_fx49,Graph_fy49);
   graph->SetName("Sensor_49");
   graph->SetTitle("Sensor_49");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx50[4] = {
   0.3248173,
   0.6496345,
   1.299269,
   1.624086};
   Double_t Graph_fy50[4] = {
   6.1886,
   5.626,
   5.626,
   6.1886};
   graph = new TGraph(4,Graph_fx50,Graph_fy50);
   graph->SetName("Sensor_50");
   graph->SetTitle("Sensor_50");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx51[6] = {
   0.3248173,
   0.6496345,
   1.299269,
   1.624086,
   1.299269,
   0.6496345};
   Double_t Graph_fy51[6] = {
   5.0634,
   4.5008,
   4.5008,
   5.0634,
   5.626,
   5.626};
   graph = new TGraph(6,Graph_fx51,Graph_fy51);
   graph->SetName("Sensor_51");
   graph->SetTitle("Sensor_51");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx52[6] = {
   0.3248173,
   0.6496345,
   1.299269,
   1.624086,
   1.299269,
   0.6496345};
   Double_t Graph_fy52[6] = {
   3.9382,
   3.3756,
   3.3756,
   3.9382,
   4.5008,
   4.5008};
   graph = new TGraph(6,Graph_fx52,Graph_fy52);
   graph->SetName("Sensor_52");
   graph->SetTitle("Sensor_52");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx53[6] = {
   0.3248173,
   0.6496345,
   1.299269,
   1.624086,
   1.299269,
   0.6496345};
   Double_t Graph_fy53[6] = {
   2.813,
   2.2504,
   2.2504,
   2.813,
   3.3756,
   3.3756};
   graph = new TGraph(6,Graph_fx53,Graph_fy53);
   graph->SetName("Sensor_53");
   graph->SetTitle("Sensor_53");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx54[6] = {
   0.3248173,
   0.6496345,
   1.299269,
   1.624086,
   1.299269,
   0.6496345};
   Double_t Graph_fy54[6] = {
   1.6878,
   1.1252,
   1.1252,
   1.6878,
   2.2504,
   2.2504};
   graph = new TGraph(6,Graph_fx54,Graph_fy54);
   graph->SetName("Sensor_54");
   graph->SetTitle("Sensor_54");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx55[6] = {
   0.3248173,
   0.6496345,
   1.299269,
   1.624086,
   1.299269,
   0.6496345};
   Double_t Graph_fy55[6] = {
   0.5626,
   1.166027e-09,
   2.332053e-09,
   0.5626,
   1.1252,
   1.1252};
   graph = new TGraph(6,Graph_fx55,Graph_fy55);
   graph->SetName("Sensor_55");
   graph->SetTitle("Sensor_55");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx56[6] = {
   0.3248172,
   0.6496345,
   1.299269,
   1.624086,
   1.299269,
   0.6496345};
   Double_t Graph_fy56[6] = {
   -0.5626,
   -1.1252,
   -1.1252,
   -0.5626,
   2.332053e-09,
   1.166027e-09};
   graph = new TGraph(6,Graph_fx56,Graph_fy56);
   graph->SetName("Sensor_56");
   graph->SetTitle("Sensor_56");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx57[6] = {
   0.3248172,
   0.6496345,
   1.299269,
   1.624086,
   1.299269,
   0.6496345};
   Double_t Graph_fy57[6] = {
   -1.6878,
   -2.2504,
   -2.2504,
   -1.6878,
   -1.1252,
   -1.1252};
   graph = new TGraph(6,Graph_fx57,Graph_fy57);
   graph->SetName("Sensor_57");
   graph->SetTitle("Sensor_57");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx58[6] = {
   0.3248172,
   0.6496345,
   1.299269,
   1.624086,
   1.299269,
   0.6496345};
   Double_t Graph_fy58[6] = {
   -2.813,
   -3.3756,
   -3.3756,
   -2.813,
   -2.2504,
   -2.2504};
   graph = new TGraph(6,Graph_fx58,Graph_fy58);
   graph->SetName("Sensor_58");
   graph->SetTitle("Sensor_58");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx59[6] = {
   0.3248172,
   0.6496345,
   1.299269,
   1.624086,
   1.299269,
   0.6496345};
   Double_t Graph_fy59[6] = {
   -3.9382,
   -4.5008,
   -4.5008,
   -3.9382,
   -3.3756,
   -3.3756};
   graph = new TGraph(6,Graph_fx59,Graph_fy59);
   graph->SetName("Sensor_59");
   graph->SetTitle("Sensor_59");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx60[6] = {
   0.3248172,
   0.6496345,
   1.299269,
   1.624086,
   1.299269,
   0.6496345};
   Double_t Graph_fy60[6] = {
   -5.0634,
   -5.626,
   -5.626,
   -5.0634,
   -4.5008,
   -4.5008};
   graph = new TGraph(6,Graph_fx60,Graph_fy60);
   graph->SetName("Sensor_60");
   graph->SetTitle("Sensor_60");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx61[4] = {
   0.3248172,
   1.624086,
   1.299269,
   0.6496345};
   Double_t Graph_fy61[4] = {
   -6.1886,
   -6.1886,
   -5.626,
   -5.626};
   graph = new TGraph(4,Graph_fx61,Graph_fy61);
   graph->SetName("Sensor_61");
   graph->SetTitle("Sensor_61");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx62[6] = {
   -0.6496345,
   -0.3248172,
   0.3248173,
   0.6496345,
   0.3248173,
   -0.3248172};
   Double_t Graph_fy62[6] = {
   5.626,
   5.0634,
   5.0634,
   5.626,
   6.1886,
   6.1886};
   graph = new TGraph(6,Graph_fx62,Graph_fy62);
   graph->SetName("Sensor_62");
   graph->SetTitle("Sensor_62");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx63[6] = {
   -0.6496345,
   -0.3248172,
   0.3248173,
   0.6496345,
   0.3248173,
   -0.3248172};
   Double_t Graph_fy63[6] = {
   4.5008,
   3.9382,
   3.9382,
   4.5008,
   5.0634,
   5.0634};
   graph = new TGraph(6,Graph_fx63,Graph_fy63);
   graph->SetName("Sensor_63");
   graph->SetTitle("Sensor_63");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx64[6] = {
   -0.6496345,
   -0.3248172,
   0.3248173,
   0.6496345,
   0.3248173,
   -0.3248172};
   Double_t Graph_fy64[6] = {
   3.3756,
   2.813,
   2.813,
   3.3756,
   3.9382,
   3.9382};
   graph = new TGraph(6,Graph_fx64,Graph_fy64);
   graph->SetName("Sensor_64");
   graph->SetTitle("Sensor_64");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx65[6] = {
   -0.6496345,
   -0.3248172,
   0.3248173,
   0.6496345,
   0.3248173,
   -0.3248172};
   Double_t Graph_fy65[6] = {
   2.2504,
   1.6878,
   1.6878,
   2.2504,
   2.813,
   2.813};
   graph = new TGraph(6,Graph_fx65,Graph_fy65);
   graph->SetName("Sensor_65");
   graph->SetTitle("Sensor_65");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx66[6] = {
   -0.6496345,
   -0.3248172,
   0.3248173,
   0.6496345,
   0.3248173,
   -0.3248172};
   Double_t Graph_fy66[6] = {
   1.1252,
   0.5626,
   0.5626,
   1.1252,
   1.6878,
   1.6878};
   graph = new TGraph(6,Graph_fx66,Graph_fy66);
   graph->SetName("Sensor_66");
   graph->SetTitle("Sensor_66");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx67[6] = {
   -0.6496345,
   -0.3248173,
   0.3248172,
   0.6496345,
   0.3248173,
   -0.3248172};
   Double_t Graph_fy67[6] = {
   -1.166027e-09,
   -0.5626,
   -0.5626,
   1.166027e-09,
   0.5626,
   0.5626};
   graph = new TGraph(6,Graph_fx67,Graph_fy67);
   graph->SetName("Sensor_67");
   graph->SetTitle("Sensor_67");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx68[6] = {
   -0.6496345,
   -0.3248173,
   0.3248172,
   0.6496345,
   0.3248172,
   -0.3248173};
   Double_t Graph_fy68[6] = {
   -1.1252,
   -1.6878,
   -1.6878,
   -1.1252,
   -0.5626,
   -0.5626};
   graph = new TGraph(6,Graph_fx68,Graph_fy68);
   graph->SetName("Sensor_68");
   graph->SetTitle("Sensor_68");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx69[6] = {
   -0.6496345,
   -0.3248173,
   0.3248172,
   0.6496345,
   0.3248172,
   -0.3248173};
   Double_t Graph_fy69[6] = {
   -2.2504,
   -2.813,
   -2.813,
   -2.2504,
   -1.6878,
   -1.6878};
   graph = new TGraph(6,Graph_fx69,Graph_fy69);
   graph->SetName("Sensor_69");
   graph->SetTitle("Sensor_69");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx70[6] = {
   -0.6496345,
   -0.3248173,
   0.3248172,
   0.6496345,
   0.3248172,
   -0.3248173};
   Double_t Graph_fy70[6] = {
   -3.3756,
   -3.9382,
   -3.9382,
   -3.3756,
   -2.813,
   -2.813};
   graph = new TGraph(6,Graph_fx70,Graph_fy70);
   graph->SetName("Sensor_70");
   graph->SetTitle("Sensor_70");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx71[6] = {
   -0.6496345,
   -0.3248173,
   0.3248172,
   0.6496345,
   0.3248172,
   -0.3248173};
   Double_t Graph_fy71[6] = {
   -4.5008,
   -5.0634,
   -5.0634,
   -4.5008,
   -3.9382,
   -3.9382};
   graph = new TGraph(6,Graph_fx71,Graph_fy71);
   graph->SetName("Sensor_71");
   graph->SetTitle("Sensor_71");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx72[6] = {
   -0.6496345,
   -0.3248173,
   0.3248172,
   0.6496345,
   0.3248172,
   -0.3248173};
   Double_t Graph_fy72[6] = {
   -5.626,
   -6.1886,
   -6.1886,
   -5.626,
   -5.0634,
   -5.0634};
   graph = new TGraph(6,Graph_fx72,Graph_fy72);
   graph->SetName("Sensor_72");
   graph->SetTitle("Sensor_72");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx73[4] = {
   -1.624086,
   -1.299269,
   -0.6496345,
   -0.3248172};
   Double_t Graph_fy73[4] = {
   6.1886,
   5.626,
   5.626,
   6.1886};
   graph = new TGraph(4,Graph_fx73,Graph_fy73);
   graph->SetName("Sensor_73");
   graph->SetTitle("Sensor_73");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx74[6] = {
   -1.624086,
   -1.299269,
   -0.6496345,
   -0.3248172,
   -0.6496345,
   -1.299269};
   Double_t Graph_fy74[6] = {
   5.0634,
   4.5008,
   4.5008,
   5.0634,
   5.626,
   5.626};
   graph = new TGraph(6,Graph_fx74,Graph_fy74);
   graph->SetName("Sensor_74");
   graph->SetTitle("Sensor_74");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx75[6] = {
   -1.624086,
   -1.299269,
   -0.6496345,
   -0.3248172,
   -0.6496345,
   -1.299269};
   Double_t Graph_fy75[6] = {
   3.9382,
   3.3756,
   3.3756,
   3.9382,
   4.5008,
   4.5008};
   graph = new TGraph(6,Graph_fx75,Graph_fy75);
   graph->SetName("Sensor_75");
   graph->SetTitle("Sensor_75");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx76[6] = {
   -1.624086,
   -1.299269,
   -0.6496345,
   -0.3248172,
   -0.6496345,
   -1.299269};
   Double_t Graph_fy76[6] = {
   2.813,
   2.2504,
   2.2504,
   2.813,
   3.3756,
   3.3756};
   graph = new TGraph(6,Graph_fx76,Graph_fy76);
   graph->SetName("Sensor_76");
   graph->SetTitle("Sensor_76");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx77[6] = {
   -1.624086,
   -1.299269,
   -0.6496345,
   -0.3248172,
   -0.6496345,
   -1.299269};
   Double_t Graph_fy77[6] = {
   1.6878,
   1.1252,
   1.1252,
   1.6878,
   2.2504,
   2.2504};
   graph = new TGraph(6,Graph_fx77,Graph_fy77);
   graph->SetName("Sensor_77");
   graph->SetTitle("Sensor_77");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx78[6] = {
   -1.624086,
   -1.299269,
   -0.6496345,
   -0.3248172,
   -0.6496345,
   -1.299269};
   Double_t Graph_fy78[6] = {
   0.5626,
   -2.332053e-09,
   -1.166027e-09,
   0.5626,
   1.1252,
   1.1252};
   graph = new TGraph(6,Graph_fx78,Graph_fy78);
   graph->SetName("Sensor_78");
   graph->SetTitle("Sensor_78");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx79[6] = {
   -1.624086,
   -1.299269,
   -0.6496345,
   -0.3248173,
   -0.6496345,
   -1.299269};
   Double_t Graph_fy79[6] = {
   -0.5626,
   -1.1252,
   -1.1252,
   -0.5626,
   -1.166027e-09,
   -2.332053e-09};
   graph = new TGraph(6,Graph_fx79,Graph_fy79);
   graph->SetName("Sensor_79");
   graph->SetTitle("Sensor_79");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx80[6] = {
   -1.624086,
   -1.299269,
   -0.6496345,
   -0.3248173,
   -0.6496345,
   -1.299269};
   Double_t Graph_fy80[6] = {
   -1.6878,
   -2.2504,
   -2.2504,
   -1.6878,
   -1.1252,
   -1.1252};
   graph = new TGraph(6,Graph_fx80,Graph_fy80);
   graph->SetName("Sensor_80");
   graph->SetTitle("Sensor_80");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx81[6] = {
   -1.624086,
   -1.299269,
   -0.6496345,
   -0.3248173,
   -0.6496345,
   -1.299269};
   Double_t Graph_fy81[6] = {
   -2.813,
   -3.3756,
   -3.3756,
   -2.813,
   -2.2504,
   -2.2504};
   graph = new TGraph(6,Graph_fx81,Graph_fy81);
   graph->SetName("Sensor_81");
   graph->SetTitle("Sensor_81");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx82[6] = {
   -1.624086,
   -1.299269,
   -0.6496345,
   -0.3248173,
   -0.6496345,
   -1.299269};
   Double_t Graph_fy82[6] = {
   -3.9382,
   -4.5008,
   -4.5008,
   -3.9382,
   -3.3756,
   -3.3756};
   graph = new TGraph(6,Graph_fx82,Graph_fy82);
   graph->SetName("Sensor_82");
   graph->SetTitle("Sensor_82");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx83[6] = {
   -1.624086,
   -1.299269,
   -0.6496345,
   -0.3248173,
   -0.6496345,
   -1.299269};
   Double_t Graph_fy83[6] = {
   -5.0634,
   -5.626,
   -5.626,
   -5.0634,
   -4.5008,
   -4.5008};
   graph = new TGraph(6,Graph_fx83,Graph_fy83);
   graph->SetName("Sensor_83");
   graph->SetTitle("Sensor_83");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx84[4] = {
   -1.624086,
   -0.3248173,
   -0.6496345,
   -1.299269};
   Double_t Graph_fy84[4] = {
   -6.1886,
   -6.1886,
   -5.626,
   -5.626};
   graph = new TGraph(4,Graph_fx84,Graph_fy84);
   graph->SetName("Sensor_84");
   graph->SetTitle("Sensor_84");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx85[6] = {
   -2.598538,
   -2.273721,
   -1.624086,
   -1.299269,
   -1.624086,
   -2.273721};
   Double_t Graph_fy85[6] = {
   5.626,
   5.0634,
   5.0634,
   5.626,
   6.1886,
   6.1886};
   graph = new TGraph(6,Graph_fx85,Graph_fy85);
   graph->SetName("Sensor_85");
   graph->SetTitle("Sensor_85");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx86[6] = {
   -2.598538,
   -2.273721,
   -1.624086,
   -1.299269,
   -1.624086,
   -2.273721};
   Double_t Graph_fy86[6] = {
   4.5008,
   3.9382,
   3.9382,
   4.5008,
   5.0634,
   5.0634};
   graph = new TGraph(6,Graph_fx86,Graph_fy86);
   graph->SetName("Sensor_86");
   graph->SetTitle("Sensor_86");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx87[6] = {
   -2.598538,
   -2.273721,
   -1.624086,
   -1.299269,
   -1.624086,
   -2.273721};
   Double_t Graph_fy87[6] = {
   3.3756,
   2.813,
   2.813,
   3.3756,
   3.9382,
   3.9382};
   graph = new TGraph(6,Graph_fx87,Graph_fy87);
   graph->SetName("Sensor_87");
   graph->SetTitle("Sensor_87");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx88[6] = {
   -2.598538,
   -2.273721,
   -1.624086,
   -1.299269,
   -1.624086,
   -2.273721};
   Double_t Graph_fy88[6] = {
   2.2504,
   1.6878,
   1.6878,
   2.2504,
   2.813,
   2.813};
   graph = new TGraph(6,Graph_fx88,Graph_fy88);
   graph->SetName("Sensor_88");
   graph->SetTitle("Sensor_88");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx89[6] = {
   -2.598538,
   -2.273721,
   -1.624086,
   -1.299269,
   -1.624086,
   -2.273721};
   Double_t Graph_fy89[6] = {
   1.1252,
   0.5626,
   0.5626,
   1.1252,
   1.6878,
   1.6878};
   graph = new TGraph(6,Graph_fx89,Graph_fy89);
   graph->SetName("Sensor_89");
   graph->SetTitle("Sensor_89");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx90[6] = {
   -2.598538,
   -2.273721,
   -1.624086,
   -1.299269,
   -1.624086,
   -2.273721};
   Double_t Graph_fy90[6] = {
   -4.664107e-09,
   -0.5626,
   -0.5626,
   -2.332053e-09,
   0.5626,
   0.5626};
   graph = new TGraph(6,Graph_fx90,Graph_fy90);
   graph->SetName("Sensor_90");
   graph->SetTitle("Sensor_90");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx91[6] = {
   -2.598538,
   -2.273721,
   -1.624086,
   -1.299269,
   -1.624086,
   -2.273721};
   Double_t Graph_fy91[6] = {
   -1.1252,
   -1.6878,
   -1.6878,
   -1.1252,
   -0.5626,
   -0.5626};
   graph = new TGraph(6,Graph_fx91,Graph_fy91);
   graph->SetName("Sensor_91");
   graph->SetTitle("Sensor_91");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx92[6] = {
   -2.598538,
   -2.273721,
   -1.624086,
   -1.299269,
   -1.624086,
   -2.273721};
   Double_t Graph_fy92[6] = {
   -2.2504,
   -2.813,
   -2.813,
   -2.2504,
   -1.6878,
   -1.6878};
   graph = new TGraph(6,Graph_fx92,Graph_fy92);
   graph->SetName("Sensor_92");
   graph->SetTitle("Sensor_92");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx93[6] = {
   -2.598538,
   -2.273721,
   -1.624086,
   -1.299269,
   -1.624086,
   -2.273721};
   Double_t Graph_fy93[6] = {
   -3.3756,
   -3.9382,
   -3.9382,
   -3.3756,
   -2.813,
   -2.813};
   graph = new TGraph(6,Graph_fx93,Graph_fy93);
   graph->SetName("Sensor_93");
   graph->SetTitle("Sensor_93");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx94[6] = {
   -2.598538,
   -2.273721,
   -1.624086,
   -1.299269,
   -1.624086,
   -2.273721};
   Double_t Graph_fy94[6] = {
   -4.5008,
   -5.0634,
   -5.0634,
   -4.5008,
   -3.9382,
   -3.9382};
   graph = new TGraph(6,Graph_fx94,Graph_fy94);
   graph->SetName("Sensor_94");
   graph->SetTitle("Sensor_94");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx95[6] = {
   -2.598538,
   -2.273721,
   -1.624086,
   -1.299269,
   -1.624086,
   -2.273721};
   Double_t Graph_fy95[6] = {
   -5.626,
   -6.1886,
   -6.1886,
   -5.626,
   -5.0634,
   -5.0634};
   graph = new TGraph(6,Graph_fx95,Graph_fy95);
   graph->SetName("Sensor_95");
   graph->SetTitle("Sensor_95");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx96[4] = {
   -3.57299,
   -3.248172,
   -2.598538,
   -2.273721};
   Double_t Graph_fy96[4] = {
   6.1886,
   5.626,
   5.626,
   6.1886};
   graph = new TGraph(4,Graph_fx96,Graph_fy96);
   graph->SetName("Sensor_96");
   graph->SetTitle("Sensor_96");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx97[6] = {
   -3.57299,
   -3.248172,
   -2.598538,
   -2.273721,
   -2.598538,
   -3.248172};
   Double_t Graph_fy97[6] = {
   5.0634,
   4.5008,
   4.5008,
   5.0634,
   5.626,
   5.626};
   graph = new TGraph(6,Graph_fx97,Graph_fy97);
   graph->SetName("Sensor_97");
   graph->SetTitle("Sensor_97");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx98[6] = {
   -3.57299,
   -3.248172,
   -2.598538,
   -2.273721,
   -2.598538,
   -3.248172};
   Double_t Graph_fy98[6] = {
   3.9382,
   3.3756,
   3.3756,
   3.9382,
   4.5008,
   4.5008};
   graph = new TGraph(6,Graph_fx98,Graph_fy98);
   graph->SetName("Sensor_98");
   graph->SetTitle("Sensor_98");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx99[6] = {
   -3.57299,
   -3.248172,
   -2.598538,
   -2.273721,
   -2.598538,
   -3.248172};
   Double_t Graph_fy99[6] = {
   2.813,
   2.2504,
   2.2504,
   2.813,
   3.3756,
   3.3756};
   graph = new TGraph(6,Graph_fx99,Graph_fy99);
   graph->SetName("Sensor_99");
   graph->SetTitle("Sensor_99");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx100[6] = {
   -3.57299,
   -3.248172,
   -2.598538,
   -2.273721,
   -2.598538,
   -3.248172};
   Double_t Graph_fy100[6] = {
   1.6878,
   1.1252,
   1.1252,
   1.6878,
   2.2504,
   2.2504};
   graph = new TGraph(6,Graph_fx100,Graph_fy100);
   graph->SetName("Sensor_100");
   graph->SetTitle("Sensor_100");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx101[6] = {
   -3.57299,
   -3.248173,
   -2.598538,
   -2.273721,
   -2.598538,
   -3.248172};
   Double_t Graph_fy101[6] = {
   0.5626,
   -5.830134e-09,
   -4.664107e-09,
   0.5626,
   1.1252,
   1.1252};
   graph = new TGraph(6,Graph_fx101,Graph_fy101);
   graph->SetName("Sensor_101");
   graph->SetTitle("Sensor_101");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx102[6] = {
   -3.57299,
   -3.248173,
   -2.598538,
   -2.273721,
   -2.598538,
   -3.248173};
   Double_t Graph_fy102[6] = {
   -0.5626,
   -1.1252,
   -1.1252,
   -0.5626,
   -4.664107e-09,
   -5.830134e-09};
   graph = new TGraph(6,Graph_fx102,Graph_fy102);
   graph->SetName("Sensor_102");
   graph->SetTitle("Sensor_102");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx103[6] = {
   -3.57299,
   -3.248173,
   -2.598538,
   -2.273721,
   -2.598538,
   -3.248173};
   Double_t Graph_fy103[6] = {
   -1.6878,
   -2.2504,
   -2.2504,
   -1.6878,
   -1.1252,
   -1.1252};
   graph = new TGraph(6,Graph_fx103,Graph_fy103);
   graph->SetName("Sensor_103");
   graph->SetTitle("Sensor_103");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx104[6] = {
   -3.57299,
   -3.248173,
   -2.598538,
   -2.273721,
   -2.598538,
   -3.248173};
   Double_t Graph_fy104[6] = {
   -2.813,
   -3.3756,
   -3.3756,
   -2.813,
   -2.2504,
   -2.2504};
   graph = new TGraph(6,Graph_fx104,Graph_fy104);
   graph->SetName("Sensor_104");
   graph->SetTitle("Sensor_104");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx105[6] = {
   -3.57299,
   -3.248173,
   -2.598538,
   -2.273721,
   -2.598538,
   -3.248173};
   Double_t Graph_fy105[6] = {
   -3.9382,
   -4.5008,
   -4.5008,
   -3.9382,
   -3.3756,
   -3.3756};
   graph = new TGraph(6,Graph_fx105,Graph_fy105);
   graph->SetName("Sensor_105");
   graph->SetTitle("Sensor_105");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx106[6] = {
   -3.57299,
   -3.248173,
   -2.598538,
   -2.273721,
   -2.598538,
   -3.248173};
   Double_t Graph_fy106[6] = {
   -5.0634,
   -5.626,
   -5.626,
   -5.0634,
   -4.5008,
   -4.5008};
   graph = new TGraph(6,Graph_fx106,Graph_fy106);
   graph->SetName("Sensor_106");
   graph->SetTitle("Sensor_106");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx107[4] = {
   -3.57299,
   -2.273721,
   -2.598538,
   -3.248173};
   Double_t Graph_fy107[4] = {
   -6.1886,
   -6.1886,
   -5.626,
   -5.626};
   graph = new TGraph(4,Graph_fx107,Graph_fy107);
   graph->SetName("Sensor_107");
   graph->SetTitle("Sensor_107");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx108[4] = {
   -4.222624,
   -3.57299,
   -3.248172,
   -3.57299};
   Double_t Graph_fy108[4] = {
   5.0634,
   5.0634,
   5.626,
   6.1886};
   graph = new TGraph(4,Graph_fx108,Graph_fy108);
   graph->SetName("Sensor_108");
   graph->SetTitle("Sensor_108");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx109[6] = {
   -4.547441,
   -4.222624,
   -3.57299,
   -3.248172,
   -3.57299,
   -4.222624};
   Double_t Graph_fy109[6] = {
   4.5008,
   3.9382,
   3.9382,
   4.5008,
   5.0634,
   5.0634};
   graph = new TGraph(6,Graph_fx109,Graph_fy109);
   graph->SetName("Sensor_109");
   graph->SetTitle("Sensor_109");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx110[6] = {
   -4.547441,
   -4.222624,
   -3.57299,
   -3.248172,
   -3.57299,
   -4.222624};
   Double_t Graph_fy110[6] = {
   3.3756,
   2.813,
   2.813,
   3.3756,
   3.9382,
   3.9382};
   graph = new TGraph(6,Graph_fx110,Graph_fy110);
   graph->SetName("Sensor_110");
   graph->SetTitle("Sensor_110");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx111[6] = {
   -4.547441,
   -4.222624,
   -3.57299,
   -3.248172,
   -3.57299,
   -4.222624};
   Double_t Graph_fy111[6] = {
   2.2504,
   1.6878,
   1.6878,
   2.2504,
   2.813,
   2.813};
   graph = new TGraph(6,Graph_fx111,Graph_fy111);
   graph->SetName("Sensor_111");
   graph->SetTitle("Sensor_111");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx112[6] = {
   -4.547441,
   -4.222624,
   -3.57299,
   -3.248172,
   -3.57299,
   -4.222624};
   Double_t Graph_fy112[6] = {
   1.1252,
   0.5626,
   0.5626,
   1.1252,
   1.6878,
   1.6878};
   graph = new TGraph(6,Graph_fx112,Graph_fy112);
   graph->SetName("Sensor_112");
   graph->SetTitle("Sensor_112");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx113[6] = {
   -4.547442,
   -4.222624,
   -3.57299,
   -3.248173,
   -3.57299,
   -4.222624};
   Double_t Graph_fy113[6] = {
   -8.162187e-09,
   -0.5626,
   -0.5626,
   -5.830134e-09,
   0.5626,
   0.5626};
   graph = new TGraph(6,Graph_fx113,Graph_fy113);
   graph->SetName("Sensor_113");
   graph->SetTitle("Sensor_113");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx114[6] = {
   -4.547442,
   -4.222624,
   -3.57299,
   -3.248173,
   -3.57299,
   -4.222624};
   Double_t Graph_fy114[6] = {
   -1.1252,
   -1.6878,
   -1.6878,
   -1.1252,
   -0.5626,
   -0.5626};
   graph = new TGraph(6,Graph_fx114,Graph_fy114);
   graph->SetName("Sensor_114");
   graph->SetTitle("Sensor_114");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx115[6] = {
   -4.547442,
   -4.222624,
   -3.57299,
   -3.248173,
   -3.57299,
   -4.222624};
   Double_t Graph_fy115[6] = {
   -2.2504,
   -2.813,
   -2.813,
   -2.2504,
   -1.6878,
   -1.6878};
   graph = new TGraph(6,Graph_fx115,Graph_fy115);
   graph->SetName("Sensor_115");
   graph->SetTitle("Sensor_115");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx116[6] = {
   -4.547442,
   -4.222624,
   -3.57299,
   -3.248173,
   -3.57299,
   -4.222624};
   Double_t Graph_fy116[6] = {
   -3.3756,
   -3.9382,
   -3.9382,
   -3.3756,
   -2.813,
   -2.813};
   graph = new TGraph(6,Graph_fx116,Graph_fy116);
   graph->SetName("Sensor_116");
   graph->SetTitle("Sensor_116");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx117[6] = {
   -4.547442,
   -4.222624,
   -3.57299,
   -3.248173,
   -3.57299,
   -4.222624};
   Double_t Graph_fy117[6] = {
   -4.5008,
   -5.0634,
   -5.0634,
   -4.5008,
   -3.9382,
   -3.9382};
   graph = new TGraph(6,Graph_fx117,Graph_fy117);
   graph->SetName("Sensor_117");
   graph->SetTitle("Sensor_117");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx118[4] = {
   -3.57299,
   -3.248173,
   -3.57299,
   -4.222624};
   Double_t Graph_fy118[4] = {
   -6.1886,
   -5.626,
   -5.0634,
   -5.0634};
   graph = new TGraph(4,Graph_fx118,Graph_fy118);
   graph->SetName("Sensor_118");
   graph->SetTitle("Sensor_118");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx119[4] = {
   -5.197076,
   -4.547441,
   -4.222624,
   -4.547441};
   Double_t Graph_fy119[4] = {
   3.3756,
   3.3756,
   3.9382,
   4.5008};
   graph = new TGraph(4,Graph_fx119,Graph_fy119);
   graph->SetName("Sensor_119");
   graph->SetTitle("Sensor_119");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx120[6] = {
   -5.521893,
   -5.197076,
   -4.547441,
   -4.222624,
   -4.547441,
   -5.197076};
   Double_t Graph_fy120[6] = {
   2.813,
   2.2504,
   2.2504,
   2.813,
   3.3756,
   3.3756};
   graph = new TGraph(6,Graph_fx120,Graph_fy120);
   graph->SetName("Sensor_120");
   graph->SetTitle("Sensor_120");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx121[6] = {
   -5.521893,
   -5.197076,
   -4.547441,
   -4.222624,
   -4.547441,
   -5.197076};
   Double_t Graph_fy121[6] = {
   1.6878,
   1.1252,
   1.1252,
   1.6878,
   2.2504,
   2.2504};
   graph = new TGraph(6,Graph_fx121,Graph_fy121);
   graph->SetName("Sensor_121");
   graph->SetTitle("Sensor_121");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx122[6] = {
   -5.521893,
   -5.197076,
   -4.547442,
   -4.222624,
   -4.547441,
   -5.197076};
   Double_t Graph_fy122[6] = {
   0.5626,
   -9.328214e-09,
   -8.162187e-09,
   0.5626,
   1.1252,
   1.1252};
   graph = new TGraph(6,Graph_fx122,Graph_fy122);
   graph->SetName("Sensor_122");
   graph->SetTitle("Sensor_122");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx123[6] = {
   -5.521893,
   -5.197076,
   -4.547442,
   -4.222624,
   -4.547442,
   -5.197076};
   Double_t Graph_fy123[6] = {
   -0.5626,
   -1.1252,
   -1.1252,
   -0.5626,
   -8.162187e-09,
   -9.328214e-09};
   graph = new TGraph(6,Graph_fx123,Graph_fy123);
   graph->SetName("Sensor_123");
   graph->SetTitle("Sensor_123");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx124[6] = {
   -5.521893,
   -5.197076,
   -4.547442,
   -4.222624,
   -4.547442,
   -5.197076};
   Double_t Graph_fy124[6] = {
   -1.6878,
   -2.2504,
   -2.2504,
   -1.6878,
   -1.1252,
   -1.1252};
   graph = new TGraph(6,Graph_fx124,Graph_fy124);
   graph->SetName("Sensor_124");
   graph->SetTitle("Sensor_124");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx125[6] = {
   -5.521893,
   -5.197076,
   -4.547442,
   -4.222624,
   -4.547442,
   -5.197076};
   Double_t Graph_fy125[6] = {
   -2.813,
   -3.3756,
   -3.3756,
   -2.813,
   -2.2504,
   -2.2504};
   graph = new TGraph(6,Graph_fx125,Graph_fy125);
   graph->SetName("Sensor_125");
   graph->SetTitle("Sensor_125");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx126[4] = {
   -4.547442,
   -4.222624,
   -4.547442,
   -5.197076};
   Double_t Graph_fy126[4] = {
   -4.5008,
   -3.9382,
   -3.3756,
   -3.3756};
   graph = new TGraph(4,Graph_fx126,Graph_fy126);
   graph->SetName("Sensor_126");
   graph->SetTitle("Sensor_126");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx127[4] = {
   -6.171528,
   -5.521893,
   -5.197076,
   -5.521893};
   Double_t Graph_fy127[4] = {
   1.6878,
   1.6878,
   2.2504,
   2.813};
   graph = new TGraph(4,Graph_fx127,Graph_fy127);
   graph->SetName("Sensor_127");
   graph->SetTitle("Sensor_127");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx128[6] = {
   -6.496345,
   -6.171528,
   -5.521893,
   -5.197076,
   -5.521893,
   -6.171528};
   Double_t Graph_fy128[6] = {
   1.1252,
   0.5626,
   0.5626,
   1.1252,
   1.6878,
   1.6878};
   graph = new TGraph(6,Graph_fx128,Graph_fy128);
   graph->SetName("Sensor_128");
   graph->SetTitle("Sensor_128");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx129[6] = {
   -6.496345,
   -6.171528,
   -5.521893,
   -5.197076,
   -5.521893,
   -6.171528};
   Double_t Graph_fy129[6] = {
   -1.166027e-08,
   -0.5626,
   -0.5626,
   -9.328214e-09,
   0.5626,
   0.5626};
   graph = new TGraph(6,Graph_fx129,Graph_fy129);
   graph->SetName("Sensor_129");
   graph->SetTitle("Sensor_129");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx130[6] = {
   -6.496345,
   -6.171528,
   -5.521893,
   -5.197076,
   -5.521893,
   -6.171528};
   Double_t Graph_fy130[6] = {
   -1.1252,
   -1.6878,
   -1.6878,
   -1.1252,
   -0.5626,
   -0.5626};
   graph = new TGraph(6,Graph_fx130,Graph_fy130);
   graph->SetName("Sensor_130");
   graph->SetTitle("Sensor_130");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx131[4] = {
   -5.521893,
   -5.197076,
   -5.521893,
   -6.171528};
   Double_t Graph_fy131[4] = {
   -2.813,
   -2.2504,
   -1.6878,
   -1.6878};
   graph = new TGraph(4,Graph_fx131,Graph_fy131);
   graph->SetName("Sensor_131");
   graph->SetTitle("Sensor_131");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx132[4] = {
   -7.14598,
   -6.496345,
   -6.171528,
   -6.496345};
   Double_t Graph_fy132[4] = {
   -1.282629e-08,
   -1.166027e-08,
   0.5626,
   1.1252};
   graph = new TGraph(4,Graph_fx132,Graph_fy132);
   graph->SetName("Sensor_132");
   graph->SetTitle("Sensor_132");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);

   Double_t Graph_fx133[4] = {
   -6.496345,
   -6.171528,
   -6.496345,
   -7.14598};
   Double_t Graph_fy133[4] = {
   -1.1252,
   -0.5626,
   -1.166027e-08,
   -1.282629e-08};
   graph = new TGraph(4,Graph_fx133,Graph_fy133);
   graph->SetName("Sensor_133");
   graph->SetTitle("Sensor_133");
   graph->SetFillColor(1);
   hitmapoly->AddBin(graph);
   return hitmapoly;
}
