/*
 * EUDAQMonitorHistos.cc
 *
 *  Created on: Sep 27, 2011
 *      Author: stanitz
 */

#include "../include/EUDAQMonitorHistos.hh"

EUDAQMonitorHistos::EUDAQMonitorHistos(const SimpleStandardEvent &ev) {
  nplanes = ev.getNPlanes();
  Hits_vs_Events = new TProfile *[nplanes];
  Planes_perEventHisto = new TH1F("Planes in Event", "Planes in Event",
                                  nplanes * 2, 0, nplanes * 2);
  Hits_vs_PlaneHisto =
      new TProfile("Hits vs Plane", "Hits vs Plane", nplanes, 0, nplanes);
  Hits_vs_EventsTotal =
      new TProfile("Hits vs Event", "Hits vs Event", 1000, 0, 20000);
  //    TracksPerEvent = new TH2I("Tracks per Event", "Tracks per Event", 1000,
  //    0, 20000, 5, 0, 5);
  TracksPerEvent =
      new TProfile("Tracks per Event", "Tracks per Event", 1000, 0, 20000);


  m_EventN_vs_TimeStamp = new TGraph();
  m_EventN_vs_TimeStamp->SetTitle("event number vs timestamp");
  m_EventN_vs_TimeStamp->GetXaxis()->SetTitle("timestamp");
  m_EventN_vs_TimeStamp->GetYaxis()->SetTitle("event number");

  
  Hits_vs_EventsTotal->SetCanExtend(TH1::kAllAxes);
  TracksPerEvent->SetCanExtend(TH1::kAllAxes);
  
  for (unsigned int i = 0; i < nplanes; i++) {

    stringstream number;
    stringstream histolabel;
    number << i;
    string name = ev.getPlane(i).getName() + " " + number.str();
    Hits_vs_PlaneHisto->GetXaxis()->SetBinLabel(i + 1, name.c_str());

    histolabel << "Hits vs Event Nr " << name;
    Hits_vs_Events[i] = new TProfile(histolabel.str().c_str(),
                                     histolabel.str().c_str(), 1000, 0, 20000);
    Hits_vs_Events[i]->SetLineColor(i + 1);
    Hits_vs_Events[i]->SetMarkerColor(i + 1);

    // fix for root being stupid
    if (i == 9) // root features //FIXME
    {
      Hits_vs_Events[i]->SetLineColor(i + 2);
      Hits_vs_Events[i]->SetMarkerColor(i + 2);
    }

    Hits_vs_Events[i]->SetCanExtend(TH1::kAllAxes);
  }
}

EUDAQMonitorHistos::~EUDAQMonitorHistos() {
  // TODO Auto-generated destructor stub
}

void EUDAQMonitorHistos::Fill(const SimpleStandardEvent &ev) {

  unsigned int event_nr = ev.getEvent_number();
  unsigned int nhits_total = 0;

  std::lock_guard<std::mutex> lck(m_mu);
  Planes_perEventHisto->Fill(ev.getNPlanes());
  for (unsigned int i = 0; i < nplanes; i++) {
    Hits_vs_PlaneHisto->Fill(i, ev.getPlane(i).getNHits());
    Hits_vs_Events[i]->Fill(event_nr, ev.getPlane(i).getNHits());
    nhits_total += ev.getPlane(i).getNHits();
  }
  Hits_vs_EventsTotal->Fill(event_nr, nhits_total);

  m_EventN_vs_TimeStamp->SetPoint(m_EventN_vs_TimeStamp->GetN(),ev.getEvent_timestamp(), ev.getEvent_number());
}

void EUDAQMonitorHistos::Fill(const unsigned int evt_number,
                              const unsigned int tracks) {
  TracksPerEvent->Fill(evt_number, tracks);
}

void EUDAQMonitorHistos::Write() {
  Planes_perEventHisto->Write();
  Hits_vs_PlaneHisto->Write();
  for (unsigned int i = 0; i < nplanes; i++) {
    Hits_vs_Events[i]->Write();
  }
  Hits_vs_EventsTotal->Write();
  TracksPerEvent->Write();
}

TNamed *EUDAQMonitorHistos::getEventN_vs_TimeStamp() const{
  return m_EventN_vs_TimeStamp;
}

TProfile *EUDAQMonitorHistos::getHits_vs_Events(unsigned int i) const {
  return Hits_vs_Events[i];
}

TProfile *EUDAQMonitorHistos::getHits_vs_EventsTotal() const {
  return Hits_vs_EventsTotal;
}

TProfile *EUDAQMonitorHistos::getHits_vs_PlaneHisto() const {
  return Hits_vs_PlaneHisto;
}

TH1F *EUDAQMonitorHistos::getPlanes_perEventHisto() const {
  return Planes_perEventHisto;
}

TProfile *EUDAQMonitorHistos::getTracksPerEventHisto() const {
  return TracksPerEvent;
}

unsigned int EUDAQMonitorHistos::getNplanes() const { return nplanes; }

void EUDAQMonitorHistos::setPlanes_perEventHisto(TH1F *Planes_perEventHisto) {
  this->Planes_perEventHisto = Planes_perEventHisto;
}

void EUDAQMonitorHistos::Reset() {
  Planes_perEventHisto->Reset();
  Hits_vs_PlaneHisto->Reset();
  for (unsigned int i = 0; i < nplanes; i++) {
    Hits_vs_Events[i]->Reset();
  }
  Hits_vs_EventsTotal->Reset();
  TracksPerEvent->Reset();
}
