#include "Processor_add2queue.hh"
namespace eudaq{
  using ReturnParam = ProcessorBase::ReturnParam;
  eudaq::Processor_add2queue::Processor_add2queue(ConnectionName_ref con_) : m_con(con_)
  {
    m_status = running;
  }

  void Processor_add2queue::init() {
    m_status = running;
    initialize();
  }



  ReturnParam Processor_add2queue::ProcessEvent(event_sp ev, ConnectionName_ref con) {
    if (m_status == running) {
      event_sp ev1;
      auto ret = add2queue(ev1);
      if (ret == stop) {
        m_status = stopping;
      }
      if (ret != sucess) {
        return ret;
      }
      handelReturn(processNext(std::move(ev1),m_con));




      if (ev) {
        return processNext(std::move(ev),con);
      }

    } else if (m_status == stopping) {
      return processNext(std::move(ev),con);
    }

    return sucess;
  }

  void Processor_add2queue::handelReturn(ReturnParam ret)
  {

    switch (ret)
    {
    case eudaq::ProcessorBase::stop:
      m_status = stopping;
      break;
    default:
      break;
    }

    
  }

}
