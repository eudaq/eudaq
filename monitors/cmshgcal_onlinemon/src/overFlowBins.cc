
#include "overFlowBins.hh"

void handleOverflowBins(TH1* hist, bool doOver, bool doUnder, bool doErr){

  const int nBins   = hist->GetNbinsX();

  if (doOver) {
    const int lastBin = hist->GetBinContent(nBins);
    const int ovflBin = hist->GetBinContent(nBins+1);
    hist->SetBinContent(nBins, lastBin+ovflBin);
   
    if (doErr){
      const int lastBinErr = hist->GetBinError(nBins);
      const int ovflBinErr = hist->GetBinError(nBins+1);
      hist->SetBinError(nBins, sqrt(pow(lastBinErr,2) + pow(ovflBinErr,2)) );
    }
    hist->SetBinContent(nBins+1,0);
  }
  
  if (doUnder) {
    const int firstBin    = hist->GetBinContent(1);
    const int undflBin    = hist->GetBinContent(0);
    hist->SetBinContent(1, firstBin+undflBin);
    
    if (doErr){
      const int firstBinErr = hist->GetBinError(1);
      const int undflBinErr = hist->GetBinError(0);
      hist->SetBinError(1, sqrt(pow(firstBinErr,2) + pow(undflBinErr,2)) );
    }
    hist->SetBinContent(0,0);
  }
}
