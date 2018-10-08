#include "CMSHGCalLayerSumCollection.hh"
#include "OnlineMon.hh"




void CMSHGCalLayerSumCollection::fillHistograms(const SimpleStandardEvent &ev) {
  Fill(ev);
}

void CMSHGCalLayerSumCollection::Reset() {
  if (mymonhistos != NULL)
    mymonhistos->Reset();
}

void CMSHGCalLayerSumCollection::Write(TFile *file) {
  if (file == NULL) {
    cout << "CMSHGCalLayerSumCollection::Write File pointer is NULL" << endl;
    exit(-1);
  }
  if (gDirectory != NULL) // check if this pointer exists
  {
    gDirectory->mkdir("CMSHGCalLayerSum");
    gDirectory->cd("CMSHGCalLayerSum");
    mymonhistos->Write();
    gDirectory->cd("..");
  }
}

//void CMSHGCalLayerSumCollection::bookHistograms(const SimpleStandardEvent & /*simpev*/) {
void CMSHGCalLayerSumCollection::bookHistograms(const eudaq::StandardEvent &ev) {
  if (_mon != NULL) {
    cout << "CMSHGCalLayerSumCollection:: Monitor running in online-mode" << endl;
    string performance_folder_name = "CMSHGCalLayerSum Monitor";
    _mon->getOnlineMon()->registerTreeItem(
        (performance_folder_name + "/Energy in MIP LG"));
    _mon->getOnlineMon()->registerHisto(
        (performance_folder_name + "/Energy in MIP LG"),
        mymonhistos->getEnergyMIPHisto());

    _mon->getOnlineMon()->registerTreeItem(
        (performance_folder_name + "/Energy in LG in ADC"));
    _mon->getOnlineMon()->registerHisto(
        (performance_folder_name + "/Energy in LG in ADC"),
        mymonhistos->getEnergyLGHisto());

    _mon->getOnlineMon()->registerTreeItem(
        (performance_folder_name + "/Energy in HG in ADC"));
    _mon->getOnlineMon()->registerHisto(
        (performance_folder_name + "/Energy in HG in ADC"),
        mymonhistos->getEnergyHGHisto());

      _mon->getOnlineMon()->registerTreeItem(
          (performance_folder_name + "/Energy from ToT in ADC"));
      _mon->getOnlineMon()->registerHisto(
          (performance_folder_name + "/Energy from ToT in ADC"),
          mymonhistos->getEnergyTOTHisto());


      _mon->getOnlineMon()->registerTreeItem(
          (performance_folder_name + "/Number of Hits in EEFH"));
      _mon->getOnlineMon()->registerHisto(
          (performance_folder_name + "/Number of Hits in EEFH"),
          mymonhistos->getNhitHisto());


      _mon->getOnlineMon()->registerTreeItem(
          (performance_folder_name + "/Number of Hits in EE"));
      _mon->getOnlineMon()->registerHisto(
          (performance_folder_name + "/Number of Hits in EE"),
          mymonhistos->getNhitEEHisto());

      _mon->getOnlineMon()->registerTreeItem(
          (performance_folder_name + "/Number of Hits in FH0"));
      _mon->getOnlineMon()->registerHisto(
          (performance_folder_name + "/Number of Hits in FH0"),
          mymonhistos->getNhitFH0Histo());

      _mon->getOnlineMon()->registerTreeItem(
          (performance_folder_name + "/Number of Hits in FH1"));
      _mon->getOnlineMon()->registerHisto(
          (performance_folder_name + "/Number of Hits in FH1"),
          mymonhistos->getNhitFH1Histo());

      _mon->getOnlineMon()->registerTreeItem(
          (performance_folder_name + "/Energy VS number of hits"));
      _mon->getOnlineMon()->registerHisto(
          (performance_folder_name + "/Energy VS number of hits"),
          mymonhistos->getEnergyVSNhitHisto());

      _mon->getOnlineMon()->registerTreeItem(
          (performance_folder_name + "/Energy VS CoGZ"));
      _mon->getOnlineMon()->registerHisto(
          (performance_folder_name + "/Energy VS CoGZ"),
          mymonhistos->getEnergyVSCoGZHisto());


      _mon->getOnlineMon()->registerTreeItem(
          (performance_folder_name + "/Longitudinal shower profile"));
      _mon->getOnlineMon()->registerHisto(
          (performance_folder_name + "/Longitudinal shower profile"),
          mymonhistos->getLongitudinalProfile());

      
      _mon->getOnlineMon()->makeTreeItemSummary(
        performance_folder_name.c_str()); // make summary page

  }//if (_mon != NULL)
}

CMSHGCalLayerSumHistos *CMSHGCalLayerSumCollection::getCMSHGCalLayerSumHistos() {
  return mymonhistos;
}

void CMSHGCalLayerSumCollection::Calculate(
    const unsigned int /*currentEventNumber*/) {}

//void CMSHGCalLayerSumCollection::Fill(const SimpleStandardEvent &simpev) {
void CMSHGCalLayerSumCollection::Fill(const eudaq::StandardEvent &simpev, int evNumber) {
  if (histos_init == false) {
    //mymonhistos = new CMSHGCalLayerSumHistos(_mon,noisyCells);
    mymonhistos = new CMSHGCalLayerSumHistos(_mon);
    if (mymonhistos == NULL) {
      cout << "CMSHGCalLayerSumCollection:: Can't book histograms " << endl;
      exit(-1);
    }
    bookHistograms(simpev);
    histos_init = true;
  }
  mymonhistos->Fill(simpev);
}
