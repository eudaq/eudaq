#include "eudaq/TTreeEventConverter.hh"

namespace eudaq{
  
  template DLLEXPORT
  std::map<uint32_t, typename Factory<TTreeEventConverter>::UP(*)()>&
  Factory<TTreeEventConverter>::Instance<>();
  
  
  bool TTreeEventConverter::Convert(EventSPC d1, TTreeEventSP d2, ConfigurationSPC conf){
    if(d1->IsFlagFake()){
      return true;
    }
    
    if(d1->IsFlagPacket()){
      EUDAQ_INFO("Converting the inputfile into a TTree ");
      TTree* m_ttree = new TTree("tree", "a simple Tree with simple variables");
      
      uint32_t run_n = (d1->GetRunN());
      uint32_t event_n = (d1->GetEventN());
      //      i_tlu = 0;
      
      m_ttree->Branch("run_n", &run_n, "run_n/I");
      m_ttree->Branch("event_n", &event_n, "event_n/I");
      
      size_t nsub = d1->GetNumSubEvent();
      for(size_t i=0; i<nsub; i++){
	auto subev = d1->GetSubEvent(i);
	if(!TTreeEventConverter::Convert(subev, d2, conf))
	  return false;
      }

      m_ttree->Fill();

    }

    uint32_t id = d1->GetType();
    auto cvt = Factory<TTreeEventConverter>::MakeUnique(id);
    if(cvt){
      return cvt->Converting(d1, d2, conf);
    }
    else{
      std::cerr<<"TTreeEventConverter: WARNING, no converter for EventID = "<<d1<<"\n";
      return false;
    }
  }
}
