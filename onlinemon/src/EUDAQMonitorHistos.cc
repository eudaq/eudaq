/*
 * EUDAQMonitorHistos.cc
 *
 *  Created on: Sep 27, 2011
 *      Author: stanitz
 */

#include "../include/EUDAQMonitorHistos.hh"

EUDAQMonitorHistos::EUDAQMonitorHistos(const SimpleStandardEvent &ev)
{
    nplanes=ev.getNPlanes();
    Hits_vs_Events=new TProfile*[nplanes];
    TLUdelta_perEventHisto=new TProfile*[nplanes];

    Planes_perEventHisto= new TH1F("Planes in Event","Planes in Event",nplanes*2,0,nplanes*2);
    Hits_vs_PlaneHisto =new TProfile("Hits vs Plane", "Hits vs Plane",nplanes,0, nplanes);
    Hits_vs_EventsTotal=new TProfile("Hits vs Event", "Hits vs Event",1000, 0, 20000);
//    TracksPerEvent = new TH2I("Tracks per Event", "Tracks per Event", 1000, 0, 20000, 5, 0, 5);
    TracksPerEvent = new TProfile("Tracks per Event", "Tracks per Event", 1000, 0, 20000);

    Hits_vs_EventsTotal->SetBit(TH1::kCanRebin);
    TracksPerEvent->SetBit(TH1::kCanRebin);

    for (unsigned int i=0; i<nplanes; i++)
    {

        stringstream number;
        stringstream histolabel;
        stringstream histolabel_tlu;
        number<<i;
        string name=ev.getPlane(i).getName()+" "+number.str();
        Hits_vs_PlaneHisto->GetXaxis()->SetBinLabel(i+1,name.c_str());

        histolabel<< "Hits vs Event Nr "<< name;
        histolabel_tlu<<"TLU Delta vs Event Nr " << name;

        Hits_vs_Events[i]=new TProfile(histolabel.str().c_str(), histolabel.str().c_str(),1000,0, 20000);
        TLUdelta_perEventHisto[i]=new TProfile(histolabel_tlu.str().c_str(), histolabel_tlu.str().c_str(),1000,0, 20000);

        Hits_vs_Events[i]->SetLineColor(i+1);
        Hits_vs_Events[i]->SetMarkerColor(i+1);
        TLUdelta_perEventHisto[i]->SetLineColor(i+1);
        TLUdelta_perEventHisto[i]->SetMarkerColor(i+1);

        //fix for root being stupid
        if (i==9) //root features //FIXME
        {
            Hits_vs_Events[i]->SetLineColor(i+2);
            Hits_vs_Events[i]->SetMarkerColor(i+2);
            TLUdelta_perEventHisto[i]->SetLineColor(i+2);
            TLUdelta_perEventHisto[i]->SetMarkerColor(i+2);
        }
        Hits_vs_Events[i]->SetBit(TH1::kCanRebin);
        TLUdelta_perEventHisto[i]->SetBit(TH1::kCanRebin);
    }



}

EUDAQMonitorHistos::~EUDAQMonitorHistos()
{
    // TODO Auto-generated destructor stub
}

void EUDAQMonitorHistos::Fill(const SimpleStandardEvent &ev)
{

    unsigned int event_nr=ev.getEvent_number();
    Planes_perEventHisto->Fill(ev.getNPlanes());
    unsigned int nhits_total=0;

    for (unsigned int i=0; i<nplanes; i++)
    {
        Hits_vs_PlaneHisto->Fill(i,ev.getPlane(i).getNHits());
        Hits_vs_Events[i]->Fill(event_nr,ev.getPlane(i).getNHits());
        TLUdelta_perEventHisto[i]->Fill(event_nr,ev.getPlane(i).getTLUEvent()-(event_nr%32768));// TLU counter can only hnadel 32768 counts
        nhits_total+=ev.getPlane(i).getNHits();
    }
    Hits_vs_EventsTotal->Fill(event_nr,nhits_total);

}

void EUDAQMonitorHistos::Fill(const unsigned int evt_number, const unsigned int tracks)
{
    TracksPerEvent->Fill(evt_number, tracks);
}



void EUDAQMonitorHistos::Write()
{
    Planes_perEventHisto->Write();
    Hits_vs_PlaneHisto->Write();
    for (unsigned int i=0; i<nplanes; i++)
    {
        Hits_vs_Events[i]->Write();
        TLUdelta_perEventHisto[i]->Write();
    }
    Hits_vs_EventsTotal->Write();
    TracksPerEvent->Write();
}



TProfile *EUDAQMonitorHistos::getHits_vs_Events(unsigned int i) const
{
    return Hits_vs_Events[i];
}

TProfile *EUDAQMonitorHistos::getHits_vs_EventsTotal() const
{
    return Hits_vs_EventsTotal;
}

TProfile *EUDAQMonitorHistos::getHits_vs_PlaneHisto() const
{
    return Hits_vs_PlaneHisto;
}

TH1F *EUDAQMonitorHistos::getPlanes_perEventHisto() const
{
    return Planes_perEventHisto;
}

TProfile *EUDAQMonitorHistos::getTLUdelta_perEventHisto(unsigned int i) const
{
    return TLUdelta_perEventHisto[i];
}

//TH2I *EUDAQMonitorHistos::getTracksPerEventHisto() const
TProfile *EUDAQMonitorHistos::getTracksPerEventHisto() const
{
    return TracksPerEvent;
}

unsigned int EUDAQMonitorHistos::getNplanes() const
{
    return nplanes;
}

void EUDAQMonitorHistos::setPlanes_perEventHisto(TH1F *Planes_perEventHisto)
{
    this->Planes_perEventHisto = Planes_perEventHisto;
}

void EUDAQMonitorHistos::Reset()
{
    Planes_perEventHisto->Reset();
    Hits_vs_PlaneHisto->Reset();
    for (unsigned int i=0; i<nplanes; i++)
    {
        Hits_vs_Events[i]->Reset();
        TLUdelta_perEventHisto[i]->Reset();
    }
    Hits_vs_EventsTotal->Reset();
    TracksPerEvent->Reset();
}


