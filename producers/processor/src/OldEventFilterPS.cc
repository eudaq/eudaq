#include "Processor.hh"
#include "Event.hh"

#include <fstream>

namespace eudaq{

  class OldEventFilterPS: public Processor{
  public:
    OldEventFilterPS(std::string cmd);
    void ProcessUserEvent(EVUP ev);
  private:
    std::ofstream m_file;
    
  };


  namespace{
  auto dummy0 = Factory<Processor>::Register<OldEventFilterPS, std::string&>(eudaq::cstr2hash("OldEventFilterPS"));
  auto dummy1 = Factory<Processor>::Register<OldEventFilterPS, std::string&&>(eudaq::cstr2hash("OldEventFilterPS"));
  }

  OldEventFilterPS::OldEventFilterPS(std::string cmd)
    :Processor("OldEventFilterPS", ""){
    ProcessCmd(cmd);
    m_file.open ("filter.txt", std::ofstream::out | std::ofstream::trunc);
  }

  void OldEventFilterPS::ProcessUserEvent(EVUP ev){
    if(ev->GetTag("Status") == "Old")
      return;

    // ev->Print(std::cout);
    // ev->Print(m_file);
    ForwardEvent(std::move(ev));
    
  }


}
