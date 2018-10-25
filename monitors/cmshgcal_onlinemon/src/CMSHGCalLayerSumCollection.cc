#include "CMSHGCalLayerSumCollection.hh"
#include "OnlineMon.hh"


void CMSHGCalLayerSumCollection::Reset() {
  if (mymonhistos != NULL)
    mymonhistos->Reset();
}

void CMSHGCalLayerSumCollection::Write(TFile *file) {
  if (file == NULL) {
    cout << "CMSHGCalLayerSumCollection::Write File pointer is NULL" << endl;
    exit(-1);
  }

 if (mymonhistos == NULL) return; //histogram class does not exist, pointer is free, nothing to do -->return
  
 if (gDirectory != NULL) // check if this pointer exists
  {
    gDirectory->mkdir("CMSHGCalLayerSum");
    gDirectory->cd("CMSHGCalLayerSum");
    std::cout<<"Trying to write cms hgcal layer sum histos"<<std::endl;
    mymonhistos->Write();
    std::cout<<"Done"<<std::endl;
    gDirectory->cd("..");
  }
}

//void CMSHGCalLayerSumCollection::bookHistograms(const SimpleStandardEvent & /*simpev*/) {
//void CMSHGCalLayerSumCollection::bookHistograms(const eudaq::StandardEvent &ev) {
void CMSHGCalLayerSumCollection::bookHistograms() {
  if (_mon != NULL) {
    cout << "CMSHGCalLayerSumCollection:: Monitor running in online-mode. Booking Histograms" << endl;
    
    string performance_folder_name = "HGCal-EnergySum";
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
          mymonhistos->getEnergyVSNhitHisto(),"COL2");

      _mon->getOnlineMon()->registerTreeItem(
          (performance_folder_name + "/Energy VS CoGZ"));
      _mon->getOnlineMon()->registerHisto(
          (performance_folder_name + "/Energy VS CoGZ"),
          mymonhistos->getEnergyVSCoGZHisto(),"COL2");


      _mon->getOnlineMon()->registerTreeItem(
          (performance_folder_name + "/Longitudinal shower 2D"));
      _mon->getOnlineMon()->registerHisto(
          (performance_folder_name + "/Longitudinal shower 2D"),
          mymonhistos->getLongitudinal2D(),"COL2");


      _mon->getOnlineMon()->registerTreeItem(
          (performance_folder_name + "/Longitudinal shower profile"));
      _mon->getOnlineMon()->registerHisto(
          (performance_folder_name + "/Longitudinal shower profile"),
          mymonhistos->getLongitudinalProfile());

      _mon->getOnlineMon()->registerTreeItem(
          (performance_folder_name + "/CoGZ VS Nhits"));
      _mon->getOnlineMon()->registerHisto(
          (performance_folder_name + "/CoGZ VS Nhits"),
          mymonhistos->getCoGZVSNhitsHisto(),"COL2");


      _mon->getOnlineMon()->makeTreeItemSummary(
        performance_folder_name.c_str()); // make summary page

      /*
      stringstream namestring;
      string name_root = performance_folder_name + "/Planes";
      for (unsigned int i = 0; i < mymonhistos->getNplanes(); i++) {
	stringstream namestring_hits;
	namestring_hits << name_root << "/Energy in MIP LG " << i;
	_mon->getOnlineMon()->registerTreeItem(namestring_hits.str());
	_mon->getOnlineMon()->registerHisto(namestring_hits.str(),
					    mymonhistos->getEnergyMIPHisto_pp(i));


	namestring_hits << name_root << "/Energy in LG in ADC " << i;
	_mon->getOnlineMon()->registerTreeItem(namestring_hits.str());
	_mon->getOnlineMon()->registerHisto(namestring_hits.str(),
					    mymonhistos->getEnergyLGHisto_pp(i));

	namestring_hits << name_root << "/Energy in HG in ADC " << i;
	_mon->getOnlineMon()->registerTreeItem(namestring_hits.str());
	_mon->getOnlineMon()->registerHisto(namestring_hits.str(),
					    mymonhistos->getEnergyHGHisto_pp(i));

	namestring_hits << name_root << "/Energy from ToT in ADC " << i;
	_mon->getOnlineMon()->registerTreeItem(namestring_hits.str());
	_mon->getOnlineMon()->registerHisto(namestring_hits.str(),
					    mymonhistos->getEnergyTOTHisto_pp(i));
	
	namestring_hits << name_root << "/Number of Hits in EEFH" << i;
	_mon->getOnlineMon()->registerTreeItem(namestring_hits.str());
	_mon->getOnlineMon()->registerHisto(namestring_hits.str(),
					    mymonhistos->getNhitHisto_pp(i));


	namestring_hits << name_root << "/Number of Hits in EE " << i;
	_mon->getOnlineMon()->registerTreeItem(namestring_hits.str());
	_mon->getOnlineMon()->registerHisto(namestring_hits.str(),
					    mymonhistos->getNhitEEHisto_pp(i));

	namestring_hits << name_root << "/Number of Hits in FH0 " << i;
	_mon->getOnlineMon()->registerTreeItem(namestring_hits.str());
	_mon->getOnlineMon()->registerHisto(namestring_hits.str(),
					    mymonhistos->getNhitFH0Histo_pp(i));

	namestring_hits << name_root << "/Number of Hits in FH1 " << i;
	_mon->getOnlineMon()->registerTreeItem(namestring_hits.str());
	_mon->getOnlineMon()->registerHisto(namestring_hits.str(),
					    mymonhistos->getNhitFH1Histo_pp(i));


	namestring_hits << name_root << "/Energy VS number of hits" << i;
	_mon->getOnlineMon()->registerTreeItem(namestring_hits.str());
	_mon->getOnlineMon()->registerHisto(namestring_hits.str(),
					    mymonhistos->getEnergyVSNhitHisto_pp(i));



      }
      _mon->getOnlineMon()->makeTreeItemSummary(
						name_root.c_str()); // make summary page
      */

  }//if (_mon != NULL)
}

CMSHGCalLayerSumHistos *CMSHGCalLayerSumCollection::getCMSHGCalLayerSumHistos() {
  return mymonhistos;
}

void CMSHGCalLayerSumCollection::Calculate(
    const unsigned int /*currentEventNumber*/) {}


void CMSHGCalLayerSumCollection::Fill(const eudaq::StandardEvent &stdev, int evNumber) {
  //cout<<"In CMSHGCalLayerSumCollection::Fill"<<endl;
  if (histos_init == false) {
    mymonhistos = new CMSHGCalLayerSumHistos(_mon,stdev);
    if (mymonhistos == NULL) {
      cout << "CMSHGCalLayerSumCollection:: Can't book histograms " << endl;
      exit(-1);
    }
    //bookHistograms(stdev);
    bookHistograms();
    histos_init = true;
  }
  mymonhistos->Fill(stdev);

  // cout<<"End of CMSHGCalLayerSumCollection::Fill"<<endl;

}
