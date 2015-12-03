#include "eudaq/ProcessorBase.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/StandardEvent.hh"

namespace eudaq{


  class Processor_convert_2_StandardEvent :public ProcessorBase{
    Processor_convert_2_StandardEvent() {}

    virtual ReturnParam ProcessEvent(event_sp ev, ConnectionName_ref con) override;
  };



  using ReturnParam = ProcessorBase::ReturnParam;

  ReturnParam Processor_convert_2_StandardEvent::ProcessEvent(event_sp ev, ConnectionName_ref con)
  {
    auto devent = dynamic_cast<const DetectorEvent*>(ev.get());
    if (ev->IsBORE())
    {
      eudaq::PluginManager::Initialize(*devent);
    }
    auto sev = event_sp(new StandardEvent(std::move(eudaq::PluginManager::ConvertToStandard(*devent))));
    return processNext(sev,con);
  }
  
}