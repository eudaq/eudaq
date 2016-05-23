#include "Processor_inspector.hh"
#include <utility>
#include <memory>
#include "Processor_batch.hh"




namespace eudaq{
  using ReturnParam = ProcessorBase::ReturnParam;




  ReturnParam Processor_Inspector::ProcessEvent(event_sp ev, ConnectionName_ref con) {
    if (!ev) {
      return stop;
    }
    auto ret = inspectEvent(*ev,con);

    if (ret != sucess) {
      return ret;
    }

    return processNext(std::move(ev),con);
  }

  

  Processor_Inspector::Processor_Inspector()  {

  }


  inspector_view::inspector_view(std::unique_ptr<ProcessorBase> proc) {
    
    auto dummy = proc.release();
    m_proc_up = std::unique_ptr<Processor_Inspector>( dynamic_cast<Processor_Inspector*>( dummy));

    if (!m_proc_up)
    {
      EUDAQ_THROW("unable to convert pointer");
    }
  }

}

eudaq::inspector_view::inspector_view(ProcessorBase* proc) {
  m_proc_rp = dynamic_cast<Processor_Inspector*>(proc);
  if (!m_proc_rp) {
    EUDAQ_THROW("unable to convert pointer");
  }
}

eudaq::inspector_view::inspector_view(std::unique_ptr<Processor_Inspector> proc) :m_proc_up(std::move(proc)) {

}

eudaq::inspector_view::inspector_view(Processor_Inspector* proc) : m_proc_rp(proc) {
  
}


