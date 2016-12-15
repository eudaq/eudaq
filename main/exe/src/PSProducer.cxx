
#include "eudaq/Producer.hh"
#include "eudaq/Processor.hh"

using namespace eudaq;
class PSProducer : public Producer {
public:
  PSProducer(const std::string& name, const std::string& runcontrol);
  ~PSProducer() override;
  void OnConfigure(const eudaq::Configuration &param) override;
  void OnStartRun(uint32_t run_n) override;
  void OnStopRun() override;
  void OnTerminate() override;
private:
  ProcessorSP m_ps;
  uint32_t m_run_n;
  
  std::string m_name_ps;
  std::string m_addr_rc;
};




PSProducer::PSProducer(const std::string& name, const std::string& runcontrol){

}


void PSProducer::OnConfigure(const eudaq::Configuration &param){
  m_ps = Processor::MakeShared(m_name_ps);

  m_ps<<

  

}


void PSProducer::OnStartRun(uint32_t run_n){
  m_run_n = run_n;
  m_ps<<"SYS:PD:RUN";

}


void PSProducer::OnStopRun(){
  m_ps<<"SYS:PD_STOP";
  
}

void PSProducer::OnTerminate(){
  m_ps.reset();
}



  
