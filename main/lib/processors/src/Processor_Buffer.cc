#include "Processor_Buffer.hh"




namespace eudaq{
  using ReturnParam = ProcessorBase::ReturnParam;



  Proccessor_Buffer::Proccessor_Buffer() {

  }

 

  ReturnParam Proccessor_Buffer::ProcessEvent(event_sp ev, ConnectionName_ref con) {
    if (m_queue.size() > m_bufferSize) {
      return ProcessorBase::busy_skip;
    }

    if (m_queue.empty()) {
      auto ret = processNext(ev,con);
      if (ret == ProcessorBase::busy_retry) {
        m_queue.push_back(ev);
        return ProcessorBase::sucess;
      }
      return ret;

    }

    m_queue.push_back(ev);

    ReturnParam ret = ProcessorBase::sucess;
    while (!m_queue.empty()) {
      auto ev1 = m_queue.front();
      ret = processNext(ev1,con);
      if (ret != busy_retry) {
        m_queue.pop_front();
      } else if (ret == ProcessorBase::busy_retry) {
        return ProcessorBase::sucess;
      } else if (ret != ProcessorBase::sucess) {
        return ret;
      }

    }
    return ProcessorBase::sucess;
  }

  void Proccessor_Buffer::init() {
    m_queue.clear();
  }


  
}
