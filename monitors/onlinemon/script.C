

#include <stdio.h>
#include <vector.h>

using namespace std;

vector<double> nocc;
vector<double> ncont;
vector<double> nerr;

const double npixel_per_submatrix=576*1152/4.;
const int nevents; 

void script()
{
TFile::Open("run215.root");

TH1D* hPlanesInEvent=(TH1D*)(gDirectory->Get("/EUDAQMonitor/Planes in Event"));
events = hPlanesInEvent->GetEntries();
cout<<events<<endl;

TH1D* h0=(TH1D*)(gDirectory->Get("/Hitmaps/MIMOSA26_0/h_hitmapSections_MIMOSA26_0"));

ncont.push_back(h0->GetBinContent(1));
ncont.push_back(h0->GetBinContent(2));
ncont.push_back(h0->GetBinContent(3));
ncont.push_back(h0->GetBinContent(4));

TH1D* h1=(TH1D*)(gDirectory->Get("/Hitmaps/MIMOSA26_1/h_hitmapSections_MIMOSA26_1"));

ncont.push_back(h1->GetBinContent(1));
ncont.push_back(h1->GetBinContent(2));
ncont.push_back(h1->GetBinContent(3));
ncont.push_back(h1->GetBinContent(4));

TH1D* h2=(TH1D*)(gDirectory->Get("/Hitmaps/MIMOSA26_2/h_hitmapSections_MIMOSA26_2"));

ncont.push_back(h2->GetBinContent(1));
ncont.push_back(h2->GetBinContent(2));
ncont.push_back(h2->GetBinContent(3));
ncont.push_back(h2->GetBinContent(4));

TH1D* h3=(TH1D*)(gDirectory->Get("/Hitmaps/MIMOSA26_3/h_hitmapSections_MIMOSA26_3"));

ncont.push_back(h3->GetBinContent(1));
ncont.push_back(h3->GetBinContent(2));
ncont.push_back(h3->GetBinContent(3));
ncont.push_back(h3->GetBinContent(4));

TH1D* h4=(TH1D*)(gDirectory->Get("/Hitmaps/MIMOSA26_4/h_hitmapSections_MIMOSA26_4"));

ncont.push_back(h4->GetBinContent(1));
ncont.push_back(h4->GetBinContent(2));
ncont.push_back(h4->GetBinContent(3));
ncont.push_back(h4->GetBinContent(4));

TH1D* h5=(TH1D*)(gDirectory->Get("/Hitmaps/MIMOSA26_5/h_hitmapSections_MIMOSA26_5"));

ncont.push_back(h5->GetBinContent(1));
ncont.push_back(h5->GetBinContent(2));
ncont.push_back(h5->GetBinContent(3));
ncont.push_back(h5->GetBinContent(4));

int ibin=0;
double icont=0;


TH1D* h_occ=new TH1D("nocc","nocc",6*4+0, 0.5,6*4+0.5);
for(int ibin=1; ibin <h_occ->GetNbinsX()+1;ibin++)
{
 double norm_factor = npixel_per_submatrix*events;
 icont=ncont[ibin-1]; h_occ->SetBinContent(ibin,icont/norm_factor); h_occ->SetBinError(ibin,TMath::Sqrt(icont)/norm_factor);
}
h_occ->SetStats(0);

h_occ->Draw("E1");
h_occ->Print("ALL");
h_occ->SetMinimum(h_occ->GetMinimum(1));
h_occ->SetMaximum(h_occ->GetMaximum(1)*10.);

TLine* line1=new TLine( 4.5,0., 4.5, h_occ->GetMaximum(1) ); line1->Draw("Same");
TLine* line2=new TLine( 8.5,0., 8.5, h_occ->GetMaximum(1) ); line2->Draw("Same");
TLine* line3=new TLine(12.5,0.,12.5, h_occ->GetMaximum(1) ); line3->Draw("Same");
TLine* line4=new TLine(16.5,0.,16.5, h_occ->GetMaximum(1) ); line4->Draw("Same");
TLine* line5=new TLine(20.5,0.,20.5, h_occ->GetMaximum(1) ); line5->Draw("Same");

gPad->SetLogy(1);
}


