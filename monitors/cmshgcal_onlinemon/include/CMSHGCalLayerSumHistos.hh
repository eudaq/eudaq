#ifndef CMSHGCALLAYERSUMHISTOS_HH_
#define CMSHGCALLAYERSUMHISTOS_HH_

#include <TH2F.h>
#include <TH1I.h>
#include "TProfile.h"
#include <TFile.h>

#include "eudaq/StandardEvent.hh"

#include "overFlowBins.hh"
#include "NoisyChannel.hh"

class RootMonitor;

class CMSHGCalLayerSumHistos {
public:
  //CMSHGCalLayerSumHistos(RootMonitor *mon, NoisyChannelList noisyCells);
  CMSHGCalLayerSumHistos(RootMonitor *mon);
  void Fill(const eudaq::StandardEvent &event, int evNumber=-1);
  void Reset();
  void Write();
  
  TH1F *getEnergyMIPHisto(){return h_energyMIP;}
  TH1I *getEnergyLGHisto(){return h_energyLG;}
  TH1I *getEnergyHGHisto(){return h_energyHG;}
  TH1I *getEnergyTOTHisto(){return h_energyTOT;}
  TH1I *getNhitHisto(){return h_nhit;}
  TH1I *getNhitEEHisto(){return h_nhitEE;}
  TH1I *getNhitFH0Histo(){return h_nhitFH0;}
  TH1I *getNhitFH1Histo(){return h_nhitFH1;}
  TH2F *getEnergyVSNhitHisto(){return h_energy_nhit;}
  TH2F *getEnergyVSCoGZHisto(){return h_energy_cogz;}
  TProfile *getLongitudinalProfile(){return h_energy_layer->ProfileX();}
  void setRootMonitor(RootMonitor *mon) { m_mon = mon; }
  
private:
  RootMonitor *m_mon;
  NoisyChannelList m_noisyCells;
  int m_mainFrameTS;
  float m_lgtomip;
  float m_maxEnergy;
  int m_maxEnergyHG;
  int m_maxEnergyLG;
  int m_maxEnergyTOT;

  TH1F *h_energyMIP;
  TH1I *h_energyLG;
  TH1I *h_energyHG;
  TH1I *h_energyTOT;
  TH1I *h_nhit;
  TH1I *h_nhitEE;
  TH1I *h_nhitFH0;
  TH1I *h_nhitFH1;
  TH2F *h_energy_nhit;
  TH2F *h_energy_cogz;
  TH2F *h_energy_layer;
};

#ifdef __CINT__
#pragma link C++ class CMSHGCALLayerSumHistos - ;
#endif

#endif /* CMSHGCALLAYERSUMHISTOS_HH_ */
