#ifndef CMSHGCALLAYERSUMHISTOS_HH_
#define CMSHGCALLAYERSUMHISTOS_HH_

#include <TH2F.h>
#include <TH1I.h>
#include "TProfile.h"
#include <TFile.h>

#include "eudaq/StandardEvent.hh"

#include "SimpleStandardEvent.hh"
#include "overFlowBins.hh"
#include "NoisyChannel.hh"

class RootMonitor;

class CMSHGCalLayerSumHistos {
public:
  //CMSHGCalLayerSumHistos(RootMonitor *mon, NoisyChannelList noisyCells);
  CMSHGCalLayerSumHistos(RootMonitor *mon,const  eudaq::StandardEvent &ev);
  void Fill(const eudaq::StandardEvent &event, int evNumber=-1);
  //void Fill(const SimpleStandardEvent &ev, int evNumber=-1);
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
  TH2I *getEnergyVSNhitHisto(){return h_energy_nhit;}
  TH2I *getEnergyVSCoGZHisto(){return h_energy_cogz;}
  TH2I *getLongitudinalProfile(){return h_energy_layer;}
  //TProfile *getLongitudinalProfile(){return h_energy_layer->ProfileX();}
  ///return per-plane histo
  /*
  TH1F *getEnergyMIPHisto_pp(unsigned int iplane){return h_energyMIP_pp[iplane];}
  TH1I *getEnergyLGHisto_pp(unsigned int iplane){return h_energyLG_pp[iplane];}
  TH1I *getEnergyHGHisto_pp(unsigned int iplane){return h_energyHG_pp[iplane];}
  TH1I *getEnergyTOTHisto_pp(unsigned int iplane){return h_energyTOT_pp[iplane];}
  TH1I *getNhitHisto_pp(unsigned int iplane){return h_nhit_pp[iplane];}
  TH1I *getNhitEEHisto_pp(unsigned int iplane){return h_nhitEE_pp[iplane];}
  TH1I *getNhitFH0Histo_pp(unsigned int iplane){return h_nhitFH0_pp[iplane];}
  TH1I *getNhitFH1Histo_pp(unsigned int iplane){return h_nhitFH1_pp[iplane];}
  TH2F *getEnergyVSNhitHisto_pp(unsigned int iplane){return h_energy_nhit_pp[iplane];}
  */
  unsigned int getNplanes() const;


  void setRootMonitor(RootMonitor *mon) { m_mon = mon; }



  
private:
  unsigned int nplanes;
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
  TH2I *h_energy_nhit;
  TH2I *h_energy_cogz;
  TH2I *h_energy_layer;

  //per-plane histos
  /*
  TH1F **h_energyMIP_pp;
  TH1I **h_energyLG_pp;
  TH1I **h_energyHG_pp;
  TH1I **h_energyTOT_pp;
  TH1I **h_nhit_pp;
  TH1I **h_nhitEE_pp;
  TH1I **h_nhitFH0_pp;
  TH1I **h_nhitFH1_pp;
  TH2F **h_energy_nhit_pp;

  */

};

#ifdef __CINT__
#pragma link C++ class CMSHGCALLayerSumHistos - ;
#endif

#endif /* CMSHGCALLAYERSUMHISTOS_HH_ */
