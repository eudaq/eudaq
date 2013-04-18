#include "../include/TreeHistoRelation.hh"

void HistoItem::setHisto(const TH1* histo, const std::string opt, const unsigned int logOpt) {
  _histo = histo;
  if (_treeItem->GetFirstChild() != 0) _overviewHisto = _treeItem->GetFirstChild()->getHisto();
  _histoOptions = opt;
  _histoLogOptions = logOpt;
}

  void HistoItem::Draw() {
    if (_fCanvas != NULL)
    {
      _fCanvas->Clear();
      if (histo != NULL && _treeItem->GetFirstChild == 0)
      { // This is a leave
        _fCanvas->SetLogx(bool(logOpt & kLogX)); 
        _fCanvas->SetLogy(bool(logOpt& kLogY)); 
        _fCanvas->SetLogz(bool(logOpt & kLogZ)); 
        _fCanvas->cd();
        _histo->Draw(_histoOptions.c_str());
      }
      else
      {
        //Count the number of histograms to show
        unsigned int no = 0;
        if (histo !=NULL) ++no;
        TGListTreeItem* child = treeItem->GetFirstChild();
        while (child != NULL) {
          ++no;
          child = child->GetNextSibling();
        }

        calculateNumSubpads(no);
        no = 1;
        // Now draw them one by one

        if (histo !=NULL)
        {
          _fCanvas->GetPad(no)->SetLogx(bool(logOpt & kLogX)); 
          _fCanvas->GetPad(no)->SetLogy(bool(logOpt & kLogY)); 
          _fCanvas->GetPad(no)->SetLogz(bool(logOpt & kLogZ)); 
          _fCanvas->cd(no);
          _histo->Draw(_histoOptions.c_str());
          ++no;
        }
        TGListTreeItem* child = treeItem->GetFirstChild();
        //Recursive Draw to All Children
        while (child != NULL)
        {
          child->setCanvas((TCanvas*)_fCanvas->GetPad(no));
          child->Draw();
          ++no;
          child = child->GetNextSibling();
        }


      }

    }
    else
    {
      cout << "Where should I draw it to?" << endl;
    }
  }

void HistoItem::calculateNumSubpads(const unsigned int no) {
  int d1=1;
  int d2=1;
  while (no > d1*d2) {
    d2=d1 +1;
    if(no > d1*d2) {d1 = d2;}
  }
  _fCanvas->Divide(d2,d1);
}
