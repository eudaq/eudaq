#include "eudaq/Producer.hh"

class FrankenserverProducer : public eudaq::Producer {
  public:
  FrankenserverProducer(const std::string & name, const std::string & runcontrol);
  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReset() override;
  void RunLoop() override;
  
  static const uint32_t m_id_factory = eudaq::cstr2hash("FrankenserverProducer");
private:
  // Setting up the stage relies on he fact, that the PI stage copmmand set GCS holds for the device, tested with C-863 and c-884
  void setupStage(std::string axisname, double refPos, double rangeNegative, double rangePositive, double Speed = 1);
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<FrankenserverProducer, const std::string&, const std::string&>(FrankenserverProducer::m_id_factory);
}

FrankenserverProducer::FrankenserverProducer(const std::string & name, const std::string & runcontrol)
    : eudaq::Producer(name, runcontrol){
}



void FrankenserverProducer::DoInitialise(){



 }

//----------DOC-MARK-----BEG*CONF-----DOC-MARK----------
void FrankenserverProducer::DoConfigure(){
  m_evt_c =0x0;
 }

void FrankenserverProducer::DoStartRun(){
  m_evt_c=0x0;
}

void FrankenserverProducer::DoStopRun(){
}

void FrankenserverProducer::DoReset(){
}

void FrankenserverProducer::DoTerminate(){
}
void FrankenserverProducer::RunLoop(){

// if(SIMON)
   m_evt_c=0x1;
}

