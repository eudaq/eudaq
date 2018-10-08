//ARNAUD : need to add energy and nhit max parameters in config file, onlinemonconfiguration.hh(.cc) and here
//ARNAUD : 1 module is still corresponding to 1 layer -> be careful with geometry when filling number of hits per FH sections or longitudinal profile and cogz 

#include <CMSHGCalLayerSumHistos.hh>
#include "OnlineMon.hh"

#define N_TIMESAMPLE 13
#define LG_TO_MIP 0.198 //mean value of lgcoeff, extracted from injection data

//CMSHGCalLayerSumHistos::CMSHGCalLayerSumHistos(RootMonitor *mon, NoisyChannelList noisyCells)
CMSHGCalLayerSumHistos::CMSHGCalLayerSumHistos(RootMonitor *mon)
{
  m_mon=mon;
  //m_noisyCells=noisyCells;
  m_mainFrameTS = m_mon->mon_configdata.getMainFrameTS();
  
  h_energyMIP = new TH1F("energyMIP","",4000,0,40000);
  h_energyMIP->GetXaxis()->SetTitle("Energy [MIP] (from LG)");
  
  h_energyLG = new TH1I("energyLG","",1000,0,100000);
  h_energyLG->GetXaxis()->SetTitle("LG [ADC counts]");
  
  h_energyHG = new TH1I("energyHG","",1000,0,800000);
  h_energyHG->GetXaxis()->SetTitle("HG [ADC counts]");

  h_energyTOT = new TH1I("energyTOT","",1000,0,10000);
  h_energyTOT->GetXaxis()->SetTitle("TOT [ADC counts]");

  h_nhit = new TH1I("nhit","",1000,0,3000);
  h_nhit->GetXaxis()->SetTitle("Nhit");
  
  h_nhitEE = new TH1I("nhitEE","",1000,0,2000);
  h_nhitEE->GetXaxis()->SetTitle("Nhit EE");
  
  h_nhitFH0 = new TH1I("nhitFH0","",1000,0,2000);
  h_nhitFH0->GetXaxis()->SetTitle("Nhit FH0");

  h_nhitFH1 = new TH1I("mhitFH1","",1000,0,2000);
  h_nhitFH1->GetXaxis()->SetTitle("Nhit FH1");
    
  h_energy_nhit = new TH2F("energy_VS_nhit","",4000,0,40000,1000,0,3000);
  h_energy_nhit->GetXaxis()->SetTitle("Nhit");
  h_energy_nhit->GetYaxis()->SetTitle("Energy [MIP]");

  h_energy_cogz = new TH2F("energy_VS_cogz","",4000,0,40000,400,0,40);
  h_energy_cogz->GetXaxis()->SetTitle("CoG Z");
  h_energy_cogz->GetYaxis()->SetTitle("Energy [MIP]");

  h_energy_layer = new TH2F("energy_VS_layer","",4000,0,40000,40,0,40);
  h_energy_layer->GetXaxis()->SetTitle("Layer");
  h_energy_layer->GetYaxis()->SetTitle("Energy [MIP]");
}

void CMSHGCalLayerSumHistos::Fill(const eudaq::StandardEvent &event, int evNumber)
{
  float energyMIP(0.),cogz(0.);
  int energyLG(0),energyHG(0),energyTOT(0),nhit(0),nhitEE(0),nhitFH0(0),nhitFH1(0);
  std::vector<float> energyLayer;
  for( size_t ip=0; ip<event.NumPlanes(); ip++ ){
    const eudaq::StandardPlane plane=event.GetPlane(ip);
    //energyLayer.push_back(ip,0.0);
    energyLayer.push_back(0.0);
    for (unsigned int ipix = 0; ipix < plane.HitPixels(); ipix++){
      int pedLG = plane.GetPixel(ipix,0);//pedestal approx: first time sample 
      int sigLG = plane.GetPixel(ipix,m_mainFrameTS);
      int pedHG = plane.GetPixel(ipix,N_TIMESAMPLE);//pedestal approx: first time sample (shift of 13 indeces needed)
      int sigHG = plane.GetPixel(ipix,m_mainFrameTS+N_TIMESAMPLE);
      energyLG += (sigLG-pedLG);
      energyHG += (sigHG-pedHG);
      energyMIP += (sigLG-pedLG)*LG_TO_MIP;
      energyTOT += plane.GetPixel(ipix,2*N_TIMESAMPLE+3);//hardcoded: tot_slow is last(29) index
      nhit++;
      if(ip<28) nhitEE++;
      else if(ip<34) nhitFH0++;
      else nhitFH1++;
      energyLayer.at(ip)+=(sigLG-pedLG)*LG_TO_MIP;
      cogz+=(sigLG-pedLG)*LG_TO_MIP*ip;
    }
  }
  cogz/=energyMIP;

  h_energyMIP->Fill(energyMIP);
  h_energyLG->Fill(energyLG);
  h_energyHG->Fill(energyHG);
  h_energyTOT->Fill(energyTOT);
  h_nhit->Fill(nhit);
  h_nhitEE->Fill(nhitEE);
  h_nhitFH0->Fill(nhitFH0);
  h_nhitFH1->Fill(nhitFH1);
  h_energy_nhit->Fill(nhit,energyMIP);
  h_energy_cogz->Fill(cogz,energyMIP);
  for( size_t ip=0; ip<event.NumPlanes(); ip++ )
    h_energy_layer->Fill(ip,energyLayer.at(ip));
}

void CMSHGCalLayerSumHistos::Reset()
{
  h_energyMIP->Reset();
  h_energyLG->Reset();
  h_energyHG->Reset();
  h_energyTOT->Reset();
  h_nhit->Reset();
  h_nhitEE->Reset();
  h_nhitFH0->Reset();
  h_nhitFH1->Reset();
  h_energy_nhit->Reset();
  h_energy_cogz->Reset();
  h_energy_layer->Reset();
}

void CMSHGCalLayerSumHistos::Write()
{
  h_energyMIP->Write();
  h_energyLG->Write();
  h_energyHG->Write();
  h_energyTOT->Write();
  h_nhit->Write();
  h_nhitEE->Write();
  h_nhitFH0->Write();
  h_nhitFH1->Write();
  h_energy_nhit->Write();
  h_energy_cogz->Write();
  h_energy_layer->Write();
}


