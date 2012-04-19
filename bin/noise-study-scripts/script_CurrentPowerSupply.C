#include "TLine.h"
#include "TText.h"
#include "TH1D.h"
#include "TFile.h"
#include "TPad.h"
#include "TMath.h"
#include "TGraph.h"
#include "TCanvas.h"
#include "TGaxis.h"

#include <map>
#include <vector>
#include <iostream>

using namespace std;

vector<double> nocc;
vector<double> ncont;
vector<double> nerr;

vector<double> mvthr4;
vector<double> mvthr5;
vector<double> mvthr6;
vector<double> mvthr7;
vector<double> mvthr8;
vector<double> mvthr9;
vector<double> mvthr10;
vector<double> mvthr11;

std::map<int,TH1D*> h_occ;
std::map<int,TH1D*> h_occ_scaled;

std::map<int,double> sensor07mvthrA;
std::map<int,double> sensor07mvthrB;
std::map<int,double> sensor07mvthrC;
std::map<int,double> sensor07mvthrD;

std::map<int,double> sensor09mvthrA;
std::map<int,double> sensor09mvthrB;
std::map<int,double> sensor09mvthrC;
std::map<int,double> sensor09mvthrD;

std::map<int,double> sensor53mvthrA;
std::map<int,double> sensor53mvthrB;
std::map<int,double> sensor53mvthrC;
std::map<int,double> sensor53mvthrD;

std::map<int,double> sensor65mvthrA;
std::map<int,double> sensor65mvthrB;
std::map<int,double> sensor65mvthrC;
std::map<int,double> sensor65mvthrD;

std::map<int,double> sensor58mvthrA;
std::map<int,double> sensor58mvthrB;
std::map<int,double> sensor58mvthrC;
std::map<int,double> sensor58mvthrD;

std::map<int,double> sensor60mvthrA;
std::map<int,double> sensor60mvthrB;
std::map<int,double> sensor60mvthrC;
std::map<int,double> sensor60mvthrD;

void init_maps()
{

 sensor07mvthrA[4]=4.11;
 sensor07mvthrA[5]=5.03;
 sensor07mvthrA[6]=5.96;
 sensor07mvthrA[7]=6.88;
 sensor07mvthrA[8]=7.81;
 sensor07mvthrA[9]=8.73;
 sensor07mvthrA[10]=9.66;
 sensor07mvthrA[11]=10.58;

 sensor07mvthrB[4]=4.19;
 sensor07mvthrB[5]=5.09;
 sensor07mvthrB[6]=5.98;
 sensor07mvthrB[7]=6.87;
 sensor07mvthrB[8]=7.77;
 sensor07mvthrB[9]=8.66;
 sensor07mvthrB[10]=9.56;
 sensor07mvthrB[11]=10.45;

 sensor07mvthrC[4]=3.88;
 sensor07mvthrC[5]=4.84;
 sensor07mvthrC[6]=5.79;
 sensor07mvthrC[7]=6.75;
 sensor07mvthrC[8]=7.70;
 sensor07mvthrC[9]=8.65;
 sensor07mvthrC[10]=9.61;
 sensor07mvthrC[11]=10.56;

 sensor07mvthrD[4]=3.65;
 sensor07mvthrD[5]=4.54;
 sensor07mvthrD[6]=5.43;
 sensor07mvthrD[7]=6.31;
 sensor07mvthrD[8]=7.20;
 sensor07mvthrD[9]=8.08;
 sensor07mvthrD[10]=8.97;
 sensor07mvthrD[11]=9.85;

 sensor09mvthrA[4]=4.12;
 sensor09mvthrA[5]=5.02;
 sensor09mvthrA[6]=5.93;
 sensor09mvthrA[7]=6.84;
 sensor09mvthrA[8]=7.74;
 sensor09mvthrA[9]=8.65;
 sensor09mvthrA[10]=9.56;
 sensor09mvthrA[11]=10.46;

 sensor09mvthrB[4]=4.01;
 sensor09mvthrB[5]=4.88;
 sensor09mvthrB[6]=5.76;
 sensor09mvthrB[7]=6.63;
 sensor09mvthrB[8]=7.51;
 sensor09mvthrB[9]=8.38;
 sensor09mvthrB[10]=9.26;
 sensor09mvthrB[11]=10.13;

 sensor09mvthrC[4]=3.87;
 sensor09mvthrC[5]=4.73;
 sensor09mvthrC[6]=5.59;
 sensor09mvthrC[7]=6.45;
 sensor09mvthrC[8]=7.32;
 sensor09mvthrC[9]=8.18;
 sensor09mvthrC[10]=9.04;
 sensor09mvthrC[11]=9.90;

 sensor09mvthrD[4]=3.46;
 sensor09mvthrD[5]=4.27;
 sensor09mvthrD[6]=5.07;
 sensor09mvthrD[7]=5.87;
 sensor09mvthrD[8]=6.67;
 sensor09mvthrD[9]=7.48;
 sensor09mvthrD[10]=8.28;
 sensor09mvthrD[11]=9.08;

 sensor53mvthrA[4]=4.51;
 sensor53mvthrA[5]=5.53;
 sensor53mvthrA[6]=6.56;
 sensor53mvthrA[7]=7.58;
 sensor53mvthrA[8]=8.61;
 sensor53mvthrA[9]=9.63;
 sensor53mvthrA[10]=10.66;
 sensor53mvthrA[11]=11.68;

 sensor53mvthrB[4]=4.26;
 sensor53mvthrB[5]=5.24;
 sensor53mvthrB[6]=6.23;
 sensor53mvthrB[7]=7.21;
 sensor53mvthrB[8]=8.20;
 sensor53mvthrB[9]=9.18;
 sensor53mvthrB[10]=10.16;
 sensor53mvthrB[11]=11.15;

 sensor53mvthrC[4]=4.77;
 sensor53mvthrC[5]=5.78;
 sensor53mvthrC[6]=6.79;
 sensor53mvthrC[7]=7.80;
 sensor53mvthrC[8]=8.80;
 sensor53mvthrC[9]=9.81;
 sensor53mvthrC[10]=10.82;
 sensor53mvthrC[11]=11.83;

 sensor53mvthrD[4]=4.02;
 sensor53mvthrD[5]=4.97;
 sensor53mvthrD[6]=5.91;
 sensor53mvthrD[7]=6.86;
 sensor53mvthrD[8]=7.80;
 sensor53mvthrD[9]=8.74;
 sensor53mvthrD[10]=9.69;
 sensor53mvthrD[11]=10.63;


 sensor65mvthrA[4]=4.19;
 sensor65mvthrA[5]=5.12;
 sensor65mvthrA[6]=6.05;
 sensor65mvthrA[7]=6.98;
 sensor65mvthrA[8]=7.91;
 sensor65mvthrA[9]=8.84;
 sensor65mvthrA[10]=9.78;
 sensor65mvthrA[11]=10.71;

 sensor65mvthrB[4]=3.88;
 sensor65mvthrB[5]=4.78;
 sensor65mvthrB[6]=5.68;
 sensor65mvthrB[7]=6.59;
 sensor65mvthrB[8]=7.49;
 sensor65mvthrB[9]=8.39;
 sensor65mvthrB[10]=9.29;
 sensor65mvthrB[11]=10.19;

 sensor65mvthrC[4]=4.29;
 sensor65mvthrC[5]=5.22;
 sensor65mvthrC[6]=6.15;
 sensor65mvthrC[7]=7.08;
 sensor65mvthrC[8]=8.01;
 sensor65mvthrC[9]=8.94;
 sensor65mvthrC[10]=9.87;
 sensor65mvthrC[11]=10.80;

 sensor65mvthrD[4]=3.98;
 sensor65mvthrD[5]=4.85;
 sensor65mvthrD[6]=5.73;
 sensor65mvthrD[7]=6.61;
 sensor65mvthrD[8]=7.49;
 sensor65mvthrD[9]=8.36;
 sensor65mvthrD[10]=9.24;
 sensor65mvthrD[11]=10.12;


 sensor58mvthrA[4]=4.51;
 sensor58mvthrA[5]=5.48;
 sensor58mvthrA[6]=6.45;
 sensor58mvthrA[7]=7.42;
 sensor58mvthrA[8]=8.38;
 sensor58mvthrA[9]=9.35;
 sensor58mvthrA[10]=10.32;
 sensor58mvthrA[11]=11.29;

 sensor58mvthrB[4]=4.36;
 sensor58mvthrB[5]=5.28;
 sensor58mvthrB[6]=6.21;
 sensor58mvthrB[7]=7.13;
 sensor58mvthrB[8]=8.06;
 sensor58mvthrB[9]=8.99;
 sensor58mvthrB[10]=9.91;
 sensor58mvthrB[11]=10.84;

 sensor58mvthrC[4]=4.31;
 sensor58mvthrC[5]=5.27;
 sensor58mvthrC[6]=6.22;
 sensor58mvthrC[7]=7.18;
 sensor58mvthrC[8]=8.14;
 sensor58mvthrC[9]=9.09;
 sensor58mvthrC[10]=10.05;
 sensor58mvthrC[11]=11.01;

 sensor58mvthrD[4]=4.50;
 sensor58mvthrD[5]=5.43;
 sensor58mvthrD[6]=6.36;
 sensor58mvthrD[7]=7.29;
 sensor58mvthrD[8]=8.22;
 sensor58mvthrD[9]=9.15;
 sensor58mvthrD[10]=10.08;
 sensor58mvthrD[11]=11.01;

 sensor60mvthrA[4]=4.86;
 sensor60mvthrA[5]=5.84;
 sensor60mvthrA[6]=6.83;
 sensor60mvthrA[7]=7.81;
 sensor60mvthrA[8]=8.79;
 sensor60mvthrA[9]=9.77;
 sensor60mvthrA[10]=10.75;
 sensor60mvthrA[11]=11.73;

 sensor60mvthrB[4]=4.34;
 sensor60mvthrB[5]=5.28;
 sensor60mvthrB[6]=6.22;
 sensor60mvthrB[7]=7.17;
 sensor60mvthrB[8]=8.11;
 sensor60mvthrB[9]=9.05;
 sensor60mvthrB[10]=10.00;
 sensor60mvthrB[11]=10.94;

 sensor60mvthrC[4]=4.37;
 sensor60mvthrC[5]=5.36;
 sensor60mvthrC[6]=6.35;
 sensor60mvthrC[7]=7.34;
 sensor60mvthrC[8]=8.33;
 sensor60mvthrC[9]=9.32;
 sensor60mvthrC[10]=10.30;
 sensor60mvthrC[11]=11.29;

 sensor60mvthrD[4]=4.50;
 sensor60mvthrD[5]=5.45;
 sensor60mvthrD[6]=6.39;
 sensor60mvthrD[7]=7.33;
 sensor60mvthrD[8]=8.27;
 sensor60mvthrD[9]=9.21;
 sensor60mvthrD[10]=10.16;
 sensor60mvthrD[11]=11.10;

}

const double npixel_per_submatrix=576*1152/4.;

void load_histos(TString filename, Int_t ithr)
{
TFile::Open(filename);

TH1D* hPlanesInEvent=(TH1D*)(gDirectory->Get("/EUDAQMonitor/Planes in Event"));
int nevents = hPlanesInEvent->GetEntries();
std::cout<<filename << " " << nevents <<std::endl;

TH1D* h0=(TH1D*)(gDirectory->Get("/Hitmaps/MIMOSA26_0/h_hitmapSections_MIMOSA26_0"));

ncont.clear();

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

printf("ncont size: %5d \n", ncont.size());

for(unsigned int ibin=ncont.size()-1; ibin>ncont.size()-24; ibin--)
{
  printf("ibin[%2d] %3.2f ", ibin, ncont[ibin]);
}
printf("\n\n");

TString snamescaled= "nocc"+ithr;
TString sname= "nocc"+ithr;

h_occ_scaled[ithr]=new TH1D(snamescaled,snamescaled,6*4+0, 0.5,6*4+0.5);
h_occ[ithr]=new TH1D(sname,sname,6*4+0, 0.5,6*4+0.5);
for(int ibin=1; ibin <h_occ[ithr]->GetNbinsX()+1;ibin++)
{
 double norm_factor = npixel_per_submatrix*nevents;
 double icont=ncont[ibin-1]; h_occ[ithr]->SetBinContent(ibin,icont/norm_factor); h_occ[ithr]->SetBinError(ibin,TMath::Sqrt(icont)/norm_factor);
 printf(" %12.5e ", icont/norm_factor);
}
 printf("\n");
}

void script_CurrentPowerSupply()
{
init_maps();
load_histos("run000281.raw.root", 4);
load_histos("run000282.raw.root", 5);
load_histos("run000283.raw.root", 6);
load_histos("run000284.raw.root", 7);
load_histos("run000285.raw.root", 8);
load_histos("run000286.raw.root", 9);
load_histos("run000287.raw.root",10);
load_histos("run000289.raw.root",11);

TCanvas *canvas=new TCanvas("canvas","canvas",1000.,1000.);

//h_occ->SetStats(0);

//h_occ->Draw("E1");
//canvas->Update();

//h_occ->Print("ALL");
//h_occ->SetMinimum(h_occ->GetMaximum(1)*0.01);
//h_occ->SetMaximum(h_occ->GetMaximum(1)*10.);

//gPad->SetLogy(1);

//////////// threshold 4 in mv

mvthr4.push_back( sensor07mvthrA[4] );
mvthr4.push_back( sensor07mvthrB[4] );
mvthr4.push_back( sensor07mvthrC[4] );
mvthr4.push_back( sensor07mvthrD[4] );
mvthr4.push_back( sensor09mvthrA[4] );
mvthr4.push_back( sensor09mvthrB[4] );
mvthr4.push_back( sensor09mvthrC[4] );
mvthr4.push_back( sensor09mvthrD[4] );
mvthr4.push_back( sensor53mvthrA[4] );
mvthr4.push_back( sensor53mvthrB[4] );
mvthr4.push_back( sensor53mvthrC[4] );
mvthr4.push_back( sensor53mvthrD[4] );
mvthr4.push_back( sensor65mvthrA[4] );
mvthr4.push_back( sensor65mvthrB[4] );
mvthr4.push_back( sensor65mvthrC[4] );
mvthr4.push_back( sensor65mvthrD[4] );
mvthr4.push_back( sensor58mvthrA[4] );
mvthr4.push_back( sensor58mvthrB[4] );
mvthr4.push_back( sensor58mvthrC[4] );
mvthr4.push_back( sensor58mvthrD[4] );
mvthr4.push_back( sensor60mvthrA[4] );
mvthr4.push_back( sensor60mvthrB[4] );
mvthr4.push_back( sensor60mvthrC[4] );
mvthr4.push_back( sensor60mvthrD[4] );

//////////// threshold 5 in mv

mvthr5.push_back( sensor07mvthrA[5] );
mvthr5.push_back( sensor07mvthrB[5] );
mvthr5.push_back( sensor07mvthrC[5] );
mvthr5.push_back( sensor07mvthrD[5] );
mvthr5.push_back( sensor09mvthrA[5] );
mvthr5.push_back( sensor09mvthrB[5] );
mvthr5.push_back( sensor09mvthrC[5] );
mvthr5.push_back( sensor09mvthrD[5] );
mvthr5.push_back( sensor53mvthrA[5] );
mvthr5.push_back( sensor53mvthrB[5] );
mvthr5.push_back( sensor53mvthrC[5] );
mvthr5.push_back( sensor53mvthrD[5] );
mvthr5.push_back( sensor65mvthrA[5] );
mvthr5.push_back( sensor65mvthrB[5] );
mvthr5.push_back( sensor65mvthrC[5] );
mvthr5.push_back( sensor65mvthrD[5] );
mvthr5.push_back( sensor58mvthrA[5] );
mvthr5.push_back( sensor58mvthrB[5] );
mvthr5.push_back( sensor58mvthrC[5] );
mvthr5.push_back( sensor58mvthrD[5] );
mvthr5.push_back( sensor60mvthrA[5] );
mvthr5.push_back( sensor60mvthrB[5] );
mvthr5.push_back( sensor60mvthrC[5] );
mvthr5.push_back( sensor60mvthrD[5] );

//////////// threshold 6 in mv

mvthr6.push_back( sensor07mvthrA[6] );
mvthr6.push_back( sensor07mvthrB[6] );
mvthr6.push_back( sensor07mvthrC[6] );
mvthr6.push_back( sensor07mvthrD[6] );
mvthr6.push_back( sensor09mvthrA[6] );
mvthr6.push_back( sensor09mvthrB[6] );
mvthr6.push_back( sensor09mvthrC[6] );
mvthr6.push_back( sensor09mvthrD[6] );
mvthr6.push_back( sensor53mvthrA[6] );
mvthr6.push_back( sensor53mvthrB[6] );
mvthr6.push_back( sensor53mvthrC[6] );
mvthr6.push_back( sensor53mvthrD[6] );
mvthr6.push_back( sensor65mvthrA[6] );
mvthr6.push_back( sensor65mvthrB[6] );
mvthr6.push_back( sensor65mvthrC[6] );
mvthr6.push_back( sensor65mvthrD[6] );
mvthr6.push_back( sensor58mvthrA[6] );
mvthr6.push_back( sensor58mvthrB[6] );
mvthr6.push_back( sensor58mvthrC[6] );
mvthr6.push_back( sensor58mvthrD[6] );
mvthr6.push_back( sensor60mvthrA[6] );
mvthr6.push_back( sensor60mvthrB[6] );
mvthr6.push_back( sensor60mvthrC[6] );
mvthr6.push_back( sensor60mvthrD[6] );

//////////// threshold 7 in mv

mvthr7.push_back( sensor07mvthrA[7] );
mvthr7.push_back( sensor07mvthrB[7] );
mvthr7.push_back( sensor07mvthrC[7] );
mvthr7.push_back( sensor07mvthrD[7] );
mvthr7.push_back( sensor09mvthrA[7] );
mvthr7.push_back( sensor09mvthrB[7] );
mvthr7.push_back( sensor09mvthrC[7] );
mvthr7.push_back( sensor09mvthrD[7] );
mvthr7.push_back( sensor53mvthrA[7] );
mvthr7.push_back( sensor53mvthrB[7] );
mvthr7.push_back( sensor53mvthrC[7] );
mvthr7.push_back( sensor53mvthrD[7] );
mvthr7.push_back( sensor65mvthrA[7] );
mvthr7.push_back( sensor65mvthrB[7] );
mvthr7.push_back( sensor65mvthrC[7] );
mvthr7.push_back( sensor65mvthrD[7] );
mvthr7.push_back( sensor58mvthrA[7] );
mvthr7.push_back( sensor58mvthrB[7] );
mvthr7.push_back( sensor58mvthrC[7] );
mvthr7.push_back( sensor58mvthrD[7] );
mvthr7.push_back( sensor60mvthrA[7] );
mvthr7.push_back( sensor60mvthrB[7] );
mvthr7.push_back( sensor60mvthrC[7] );
mvthr7.push_back( sensor60mvthrD[7] );

//////////// threshold 8 in mv

mvthr8.push_back( sensor07mvthrA[8] );
mvthr8.push_back( sensor07mvthrB[8] );
mvthr8.push_back( sensor07mvthrC[8] );
mvthr8.push_back( sensor07mvthrD[8] );
mvthr8.push_back( sensor09mvthrA[8] );
mvthr8.push_back( sensor09mvthrB[8] );
mvthr8.push_back( sensor09mvthrC[8] );
mvthr8.push_back( sensor09mvthrD[8] );
mvthr8.push_back( sensor53mvthrA[8] );
mvthr8.push_back( sensor53mvthrB[8] );
mvthr8.push_back( sensor53mvthrC[8] );
mvthr8.push_back( sensor53mvthrD[8] );
mvthr8.push_back( sensor65mvthrA[8] );
mvthr8.push_back( sensor65mvthrB[8] );
mvthr8.push_back( sensor65mvthrC[8] );
mvthr8.push_back( sensor65mvthrD[8] );
mvthr8.push_back( sensor58mvthrA[8] );
mvthr8.push_back( sensor58mvthrB[8] );
mvthr8.push_back( sensor58mvthrC[8] );
mvthr8.push_back( sensor58mvthrD[8] );
mvthr8.push_back( sensor60mvthrA[8] );
mvthr8.push_back( sensor60mvthrB[8] );
mvthr8.push_back( sensor60mvthrC[8] );
mvthr8.push_back( sensor60mvthrD[8] );

//////////// threshold 9 in mv

mvthr9.push_back( sensor07mvthrA[9] );
mvthr9.push_back( sensor07mvthrB[9] );
mvthr9.push_back( sensor07mvthrC[9] );
mvthr9.push_back( sensor07mvthrD[9] );
mvthr9.push_back( sensor09mvthrA[9] );
mvthr9.push_back( sensor09mvthrB[9] );
mvthr9.push_back( sensor09mvthrC[9] );
mvthr9.push_back( sensor09mvthrD[9] );
mvthr9.push_back( sensor53mvthrA[9] );
mvthr9.push_back( sensor53mvthrB[9] );
mvthr9.push_back( sensor53mvthrC[9] );
mvthr9.push_back( sensor53mvthrD[9] );
mvthr9.push_back( sensor65mvthrA[9] );
mvthr9.push_back( sensor65mvthrB[9] );
mvthr9.push_back( sensor65mvthrC[9] );
mvthr9.push_back( sensor65mvthrD[9] );
mvthr9.push_back( sensor58mvthrA[9] );
mvthr9.push_back( sensor58mvthrB[9] );
mvthr9.push_back( sensor58mvthrC[9] );
mvthr9.push_back( sensor58mvthrD[9] );
mvthr9.push_back( sensor60mvthrA[9] );
mvthr9.push_back( sensor60mvthrB[9] );
mvthr9.push_back( sensor60mvthrC[9] );
mvthr9.push_back( sensor60mvthrD[9] );

//////////// threshold 10 in mv

mvthr10.push_back( sensor07mvthrA[10] );
mvthr10.push_back( sensor07mvthrB[10] );
mvthr10.push_back( sensor07mvthrC[10] );
mvthr10.push_back( sensor07mvthrD[10] );
mvthr10.push_back( sensor09mvthrA[10] );
mvthr10.push_back( sensor09mvthrB[10] );
mvthr10.push_back( sensor09mvthrC[10] );
mvthr10.push_back( sensor09mvthrD[10] );
mvthr10.push_back( sensor53mvthrA[10] );
mvthr10.push_back( sensor53mvthrB[10] );
mvthr10.push_back( sensor53mvthrC[10] );
mvthr10.push_back( sensor53mvthrD[10] );
mvthr10.push_back( sensor65mvthrA[10] );
mvthr10.push_back( sensor65mvthrB[10] );
mvthr10.push_back( sensor65mvthrC[10] );
mvthr10.push_back( sensor65mvthrD[10] );
mvthr10.push_back( sensor58mvthrA[10] );
mvthr10.push_back( sensor58mvthrB[10] );
mvthr10.push_back( sensor58mvthrC[10] );
mvthr10.push_back( sensor58mvthrD[10] );
mvthr10.push_back( sensor60mvthrA[10] );
mvthr10.push_back( sensor60mvthrB[10] );
mvthr10.push_back( sensor60mvthrC[10] );
mvthr10.push_back( sensor60mvthrD[10] );

//////////// threshold 11 in mv

mvthr11.push_back( sensor07mvthrA[11] );
mvthr11.push_back( sensor07mvthrB[11] );
mvthr11.push_back( sensor07mvthrC[11] );
mvthr11.push_back( sensor07mvthrD[11] );
mvthr11.push_back( sensor09mvthrA[11] );
mvthr11.push_back( sensor09mvthrB[11] );
mvthr11.push_back( sensor09mvthrC[11] );
mvthr11.push_back( sensor09mvthrD[11] );
mvthr11.push_back( sensor53mvthrA[11] );
mvthr11.push_back( sensor53mvthrB[11] );
mvthr11.push_back( sensor53mvthrC[11] );
mvthr11.push_back( sensor53mvthrD[11] );
mvthr11.push_back( sensor65mvthrA[11] );
mvthr11.push_back( sensor65mvthrB[11] );
mvthr11.push_back( sensor65mvthrC[11] );
mvthr11.push_back( sensor65mvthrD[11] );
mvthr11.push_back( sensor58mvthrA[11] );
mvthr11.push_back( sensor58mvthrB[11] );
mvthr11.push_back( sensor58mvthrC[11] );
mvthr11.push_back( sensor58mvthrD[11] );
mvthr11.push_back( sensor60mvthrA[11] );
mvthr11.push_back( sensor60mvthrB[11] );
mvthr11.push_back( sensor60mvthrC[11] );
mvthr11.push_back( sensor60mvthrD[11] );

///////////////////////////////////////////////////////////////////
TLine* line1=new TLine(  4.5, 2.5,  4.5, 24. ); line1->Draw("Same");
TLine* line2=new TLine(  8.5, 2.5,  8.5, 24. ); line2->Draw("Same");
TLine* line3=new TLine( 12.5, 2.5, 12.5, 24. ); line3->Draw("Same");
TLine* line4=new TLine( 16.5, 2.5, 16.5, 24. ); line4->Draw("Same");
TLine* line5=new TLine( 20.5, 2.5, 20.5, 24. ); line5->Draw("Same");

TText* text0=new TText( 1.2, 24.0,"sensor 7" ); text0->SetTextSize(0.02);text0->Draw("Same");
TText* text1=new TText( 5.2, 24.0,"sensor 9" ); text1->SetTextSize(0.02);text1->Draw("Same");
TText* text2=new TText( 9.2, 24.0,"sensor 53"); text2->SetTextSize(0.02);text2->Draw("Same");
TText* text3=new TText(13.2, 24.0,"sensor 65"); text3->SetTextSize(0.02);text3->Draw("Same");
TText* text4=new TText(17.2, 24.0,"sensor 58"); text4->SetTextSize(0.02);text4->Draw("Same");
TText* text5=new TText(21.2, 24.0,"sensor 60"); text5->SetTextSize(0.02);text5->Draw("Same");


////////////////////////////////////////////////////////////////////
TH1D* h_thr8_unit=new TH1D("thr8","thr8",6*4+0, 0.5,6*4+0.5);

for(int ibin=1; ibin <h_thr8_unit->GetNbinsX()+1;ibin++)
{
 h_thr8_unit->SetBinContent(ibin,1.); 
}

TH1D* h_thr8_scaled=new TH1D("thr8","thr8",6*4+0, 0.5,6*4+0.5);

double icont=6.;

//////////////////////////////////////////////////////////////////////
TH1D* h_thr4=new TH1D("igor","current power supplies",6*4+0, 0.5,6*4+0.5);

for(int ibin=1; ibin <h_thr4->GetNbinsX()+1;ibin++)
{
  icont=mvthr4[ibin-1]; 
  h_thr4->SetBinContent(ibin,icont);
}


//////////////////////////////////////////////////////////////////////
TH1D* h_thr5=new TH1D("thr5","thr5",6*4+0, 0.5,6*4+0.5);

for(int ibin=1; ibin <h_thr5->GetNbinsX()+1;ibin++)
{
  icont=mvthr5[ibin-1]; 
  h_thr5->SetBinContent(ibin,icont);
}


//////////////////////////////////////////////////////////////////////
TH1D* h_thr6=new TH1D("thr6","thr6",6*4+0, 0.5,6*4+0.5);

for(int ibin=1; ibin <h_thr6->GetNbinsX()+1;ibin++)
{
  icont=mvthr6[ibin-1]; 
  h_thr6->SetBinContent(ibin,icont);
}


//////////////////////////////////////////////////////////////////////
TH1D* h_thr7=new TH1D("thr7","thr7",6*4+0, 0.5,6*4+0.5);

for(int ibin=1; ibin <h_thr7->GetNbinsX()+1;ibin++)
{
  icont=mvthr7[ibin-1]; 
  h_thr7->SetBinContent(ibin,icont);
}


//////////////////////////////////////////////////////////////////////
TH1D* h_thr8=new TH1D("thr8","thr8",6*4+0, 0.5,6*4+0.5);

for(int ibin=1; ibin <h_thr8->GetNbinsX()+1;ibin++)
{
  icont=mvthr8[ibin-1]; 
  h_thr8->SetBinContent(ibin,icont);
}


//////////////////////////////////////////////////////////////////////
TH1D* h_thr9=new TH1D("thr9","thr9",6*4+0, 0.5,6*4+0.5);

for(int ibin=1; ibin <h_thr9->GetNbinsX()+1;ibin++)
{
  icont=mvthr9[ibin-1]; 
  h_thr9->SetBinContent(ibin,icont);
}


//////////////////////////////////////////////////////////////////////
TH1D* h_thr10=new TH1D("thr10","thr10",6*4+0, 0.5,6*4+0.5);

for(int ibin=1; ibin <h_thr10->GetNbinsX()+1;ibin++)
{
  icont=mvthr10[ibin-1]; 
  h_thr10->SetBinContent(ibin,icont);
}


//////////////////////////////////////////////////////////////////////
TH1D* h_thr11=new TH1D("thr11","thr11",6*4+0, 0.5,6*4+0.5);

for(int ibin=1; ibin <h_thr11->GetNbinsX()+1;ibin++)
{
  icont=mvthr11[ibin-1]; 
  h_thr11->SetBinContent(ibin,icont);
}

///////////////////////////////////////
h_thr4->SetMarkerStyle(20);
h_thr5->SetMarkerStyle(21);
h_thr6->SetMarkerStyle(22);
h_thr7->SetMarkerStyle(23);
h_thr8->SetMarkerStyle(24);
h_thr9->SetMarkerStyle(25);
h_thr10->SetMarkerStyle(26);
h_thr11->SetMarkerStyle(29);

h_thr4->SetMarkerColor(1);
h_thr5->SetMarkerColor(2);
h_thr6->SetMarkerColor(3);
h_thr7->SetMarkerColor(4);
h_thr8->SetMarkerColor(6);
h_thr9->SetMarkerColor(7);
h_thr10->SetMarkerColor(8);
h_thr11->SetMarkerColor(9);

h_thr4->SetLineColor(1);
h_thr5->SetLineColor(2);
h_thr6->SetLineColor(3);
h_thr7->SetLineColor(4);
h_thr8->SetLineColor(6);
h_thr9->SetLineColor(7);
h_thr10->SetLineColor(8);
h_thr11->SetLineColor(9);

h_thr4->SetLineStyle(2);
h_thr5->SetLineStyle(2);
h_thr6->SetLineStyle(2);
h_thr7->SetLineStyle(2);
h_thr8->SetLineStyle(2);
h_thr9->SetLineStyle(2);
h_thr10->SetLineStyle(2);
h_thr11->SetLineStyle(2);

///////////////////////////////////////////////////////////////////////
h_thr4->SetMarkerSize(1.5);
h_thr5->SetMarkerSize(1.5);
h_thr6->SetMarkerSize(1.5);
h_thr7->SetMarkerSize(1.5);
h_thr8->SetMarkerSize(1.5);
h_thr9->SetMarkerSize(1.5);
h_thr10->SetMarkerSize(1.5);
h_thr11->SetMarkerSize(1.5);

///////////////////////////////////////////////////////////////////////
h_thr4->Draw("PL");
h_thr4->SetMaximum(25.);
h_thr4->SetMinimum(2.5);
h_thr4->SetStats(0);

h_thr5->Draw("PLSAME");
h_thr6->Draw("PLSAME");
h_thr7->Draw("PLSAME");
h_thr8->Draw("PLSAME");
h_thr9->Draw("PLSAME");
h_thr10->Draw("PLSAME");
h_thr11->Draw("PLSAME");

line1->Draw("Same");
line2->Draw("Same");
line3->Draw("Same");
line4->Draw("Same");
line5->Draw("Same");

text0->Draw("SAME");
text1->Draw("SAME");
text2->Draw("SAME");
text3->Draw("SAME");
text4->Draw("SAME");
text5->Draw("SAME");

///////////////////////////////////////////////////////////////////////

double x1=0.5;
double x2=24.5;
double y1=h_thr4->GetMinimum();
double y2=h_thr4->GetMaximum();
// gPad->GetRange(x1,x2,y1,y2);
cout<<x1 << " " << x2 << " " << y1 << " " << y2 << endl;
   //scale hint1 to the pad coordinates
      Float_t rightmax = 5e-4                 ;
      Float_t rightmin = 2e-12                 ;
      Float_t scale = 1./(rightmax-rightmin);
//cout<< gPad->GetUymax()<< endl;
//cout<< gPad->GetUymin()<< endl;
 cout << rightmin << " " << rightmax << endl;
//      h_thr8->SetLineColor(kRed);
//    h_thr8->Add(h_thr8_unit,-rightmin);
//    h_thr8->Scale(scale);
//    h_thr8->Print("ALL");

//      h_thr8->Add(h_thr8_unit,y1);
//      h_thr8->Draw("same");

/////////////////////////////////
     double fnorm = 1.;


for(int ithr=4; ithr<12;ithr++)
{
for(int i=1; i<=h_occ[ithr]->GetNbinsX(); i++)
{
// cout<<x1 << " " << x2 << " " << y1 << " " << y2 << endl;
    double yscale=(y2-y1)*fnorm;
     double ymin=      y1*fnorm;
//    long double f0= 0.;
     Double_t g0 = (TMath::Log10(h_occ[ithr]->GetBinContent(i) ) - TMath::Log10(rightmin) )/( TMath::Log10(rightmax)-TMath::Log10(rightmin))*yscale+ymin;
     Double_t f0 = g0;
//     Double_t f1 = TMath::Log10(2.718281828);
//     Double_t f0 = g0;
//    f0 = ((h_thr8->GetBinContent(i)-rightmin )/(rightmax-rightmin)*yscale+y1 );
//   double f0=g0;

//cout<< i << " " << h_occ[ithr]->GetBinContent(i) << " " << yscale << " " << ymin << " " << g0 << " " << f0 <<   endl;
//printf("%12.5e \n", f0);
     h_occ_scaled[ithr]->SetBinContent(i, f0  );
}
     h_occ_scaled[ithr]->Scale(1.);
//     h_occ_scaled[ithr]->Print("ALL"); 
     h_occ_scaled[ithr]->SetMarkerSize(1);
     h_occ_scaled[ithr]->SetLineStyle(1);
     h_occ_scaled[ithr]->Draw("pLSAME");
}
     h_occ_scaled[4]->SetMarkerColor(1);
     h_occ_scaled[5]->SetMarkerColor(2);
     h_occ_scaled[6]->SetMarkerColor(3);
     h_occ_scaled[7]->SetMarkerColor(4);
     h_occ_scaled[8]->SetMarkerColor(6);
     h_occ_scaled[9]->SetMarkerColor(7);
     h_occ_scaled[10]->SetMarkerColor(8);
     h_occ_scaled[11]->SetMarkerColor(9);

     h_occ_scaled[4]->SetLineColor(1);
     h_occ_scaled[5]->SetLineColor(2);
     h_occ_scaled[6]->SetLineColor(3);
     h_occ_scaled[7]->SetLineColor(4);
     h_occ_scaled[8]->SetLineColor(6);
     h_occ_scaled[9]->SetLineColor(7);
     h_occ_scaled[10]->SetLineColor(8);
     h_occ_scaled[11]->SetLineColor(9);

     h_occ_scaled[4]->SetMarkerStyle(20);
     h_occ_scaled[5]->SetMarkerStyle(21);
     h_occ_scaled[6]->SetMarkerStyle(22);
     h_occ_scaled[7]->SetMarkerStyle(23);
     h_occ_scaled[8]->SetMarkerStyle(24);
     h_occ_scaled[9]->SetMarkerStyle(25);
     h_occ_scaled[10]->SetMarkerStyle(26);
     h_occ_scaled[11]->SetMarkerStyle(29);

//
// gPad->SetLogy(1);
// TPad* newpad=new TPad();
// newpad->Draw("same");
// h_thr8->SetStats(0);
// TGaxis *axis = new TGaxis(gPad->GetUxmax(),gPad->GetUymin(), gPad->GetUxmax(), gPad->GetUymax(), rightmin,rightmax,510,"+L");
//
  
   TGaxis *axis = new TGaxis( x2, y1, x2, y2, rightmin,rightmax,510,"G+");
   axis->SetLineColor(kRed);
   axis->SetLabelColor(kRed);
   axis->Draw("");

//  TH1F *hr = canvas->DrawFrame(0.,0.0000001,25.,0.0001);
//  hr->SetTitle(";Q^{2}, [GeV^{2}];#sigma(#gamma * p#rightarrow V p), [nb]");
//  hr->GetYaxis()->SetTitleOffset(1.5);
//  hr->GetXaxis()->SetTitleOffset(1.5);
//    gPad->SetLogy(1);
//    gPad->SetLogx(1);
//    gPad->SetTickx();
//    gPad->SetTicky();
//    graph->Draw("PE");
//
//    h_thr8->Draw("E1SAME");
/*
canvas->cd();
    TGraph *graph = new TGraph ();
       graph->SetPoint(0, 0.00100, 30.00);
       graph->SetPoint(1, 0.00150, 20.00);
       graph->SetPoint(2, 0.01150, 30.00);
       graph->Draw("p");
       graph->GetXaxis()->SetTitle("foo") ;

    TGraph *graph2 = new TGraph ();
       graph2->SetPoint(0, 0.100, 30.00);
       graph2->SetPoint(1, 0.150, 20.00);
       graph2->SetPoint(2, 0.1150, 30.00);
       graph2->Draw("pSAME");
canvas->Update();
*/

 canvas->SaveAs("threshold-scan-current-power-supply.png");
}


