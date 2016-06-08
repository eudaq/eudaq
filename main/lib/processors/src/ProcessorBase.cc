#include "ProcessorBase.hh"
#include "eudaq/OptionParser.hh"
#include "Processor_inspector.hh"
#include "Processor_batch.hh"

namespace eudaq{
  


// ProcessorBase::ProcessorBase(ProcessorBase::Parameter_ref name):m_conf(name) {
// 
// }

ProcessorBase::ProcessorBase()  {

}

void ProcessorBase::AddNextProcessor(Processor_rp next) {
    m_next = next;
  }


 




ProcessorBase::ReturnParam ProcessorBase::processNext(event_sp ev, ConnectionName_ref con) {
  if (m_next)
  {
    return m_next->ProcessEvent(std::move(ev), con);
  }
  return sucess;
}

ProcessorBase::ConnectionName_t default_connection() {
  return 0;
}

ProcessorBase::ConnectionName_t random_connection() {
  static ProcessorBase::ConnectionName_t  i = 100;
  return ++i;
}



processor_view::processor_view(ProcessorBase* base_r) :m_proc_rp(base_r) {

}

processor_view::processor_view(std::unique_ptr<ProcessorBase> base_u) : m_proc_up(std::move(base_u)) {

}


processor_view::processor_view(std::unique_ptr<Processor_batch_splitter> batch_split) {
  m_proc_up = Processor_up(dynamic_cast<Processor_rp>(batch_split.release()));
  if (!m_proc_up) {
    EUDAQ_THROW("unable to convert");
  }
}



processor_view::processor_view(std::unique_ptr<Processor_Inspector> proc) {
  m_proc_up = Processor_up(dynamic_cast<Processor_rp>(proc.release()));
  if (!m_proc_up) {
    EUDAQ_THROW("unable to convert");
  }
}

}
