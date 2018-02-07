#include "eudaq/TTreeEventConverter.hh"

namespace eudaq{
  
  template DLLEXPORT
  std::map<uint32_t, typename Factory<TTreeEventConverter>::UP(*)()>&
  Factory<TTreeEventConverter>::Instance<>();
  
  bool TTreeEventConverter::Convert(EventSPC d1, TTreeEventSP d2, ConfigurationSPC conf){

    if(d1->IsFlagFake()){
      return true;
    }
    uint32_t d2_flag;
    if(d1->IsFlagPacket()){
      EUDAQ_INFO("Converting the inputfile to a root file ");
      uint32_t run_n = (d1->GetRunN());
      uint32_t event_n = (d1->GetEventN());
      d2->Branch("run_n", &run_n, "run_n/I");
      d2->Branch("event_n", &event_n, "event_n/I");
      if(d1->IsFlagTimestamp()){
	uint64_t tsb = (int)d1->GetTimestampBegin();
	d2->SetBranchAddress("timestampbegin", &tsb);
	uint64_t tse = (int)d1->GetTimestampEnd();
	d2->SetBranchAddress("timestampend", &tse);
      }
      uint32_t eflag = d1->GetFlag();
      d2->SetBranchAddress("event_flag", &eflag);
      uint32_t devnum = d1->GetDeviceN();
      d2->SetBranchAddress("device_n", & devnum);
      if(d1->IsFlagTrigger()){
	uint32_t trign = d1->GetTriggerN();
	d2->SetBranchAddress("trigger_n", &trign);
      }
      size_t nsub = d1->GetNumSubEvent();
      for(size_t i=0; i<nsub; i++){
	auto subev = d1->GetSubEvent(i);
	if(!TTreeEventConverter::Convert(subev, d2, conf))
	  return false;
      }
      d2_flag	=  (int)(d1->GetFlag() & ~Event::Flags::FLAG_PACK);
      return true;
    }
    if((d2_flag & Event::Flags::FLAG_PACK) != Event::Flags::FLAG_PACK){
      // EUDAQ_INFO("Converting the input file to a root file");
      uint32_t run_n = (d1->GetRunN());
      d2->SetBranchAddress("run_n",&run_n);
      uint32_t event_n = (d1->GetEventN());
           d2->SetBranchAddress("event_n", &event_n);
      if(d1->IsFlagTimestamp()){
	uint64_t tsb = (int)d1->GetTimestampBegin();
	d2->SetBranchAddress("timestampbegin", &tsb);
	uint64_t tse = (int)d1->GetTimestampEnd();
	d2->SetBranchAddress("timestampend", &tse);
      }
	uint32_t eflag = d1->GetFlag();
	d2->SetBranchAddress("event_flag", &eflag);
	uint32_t devnum = d1->GetDeviceN();
	d2->SetBranchAddress("device_n", & devnum);
	uint32_t trign = d1->GetTriggerN();
	d2->SetBranchAddress("trigger_n", &trign);
	if(d1->IsFlagTrigger()){
	  uint32_t trign = d1->GetTriggerN();
	  d2->SetBranchAddress("trigger_n", &trign);
	}
    }
    d2->Fill();      

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
