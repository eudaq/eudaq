#include "Processor.hh"
#include "Event.hh"

#include <fstream>

namespace eudaq{

  class OldEventFilterPS: public Processor{
  public:
    OldEventFilterPS();
    void ProcessEvent(EventSPC ev) final override;
  private:
    std::ofstream m_file;    
  };


  namespace{
  auto dummy0 = Factory<Processor>::Register<OldEventFilterPS>(eudaq::cstr2hash("OldEventFilterPS"));
  }

  OldEventFilterPS::OldEventFilterPS()
    :Processor("OldEventFilterPS"){
    m_file.open ("filter.txt", std::ofstream::out | std::ofstream::trunc);
  }

  void OldEventFilterPS::ProcessEvent(EventSPC ev){
    if(ev->GetTag("Status") == "Old")
      return;

    // ev->Print(std::cout);
    // ev->Print(m_file);
    ForwardEvent(ev);
    
  }


}
