#include "eudaq/EventSynchronisationBase.hh"
#include "eudaq/Event.hh"
#include "eudaq/PointerEvent.hh"
#include <string>
#include <queue>
#include "eudaq/DetectorEvent.hh"


namespace eudaq{

  class data2Pointer_test_processor : public SyncBase {
  public:

   // public interface

    data2Pointer_test_processor(Parameter_ref param);
    virtual bool pushEvent(event_sp ev, size_t Index = 0) ;
    virtual bool getNextEvent(event_sp& ev);
    virtual bool OutputIsEmpty() const{ return m_outProcess->OutputIsEmpty(); }
    virtual bool InputIsEmpty() const { return m_process->InputIsEmpty(); };
    virtual bool InputIsEmpty(size_t FileID) const { return m_process->InputIsEmpty(FileID); };


  private:

    std::unique_ptr<SyncBase> m_process, m_outProcess;
  };

  data2Pointer_test_processor::data2Pointer_test_processor(Parameter_ref param) :SyncBase(param)
  {
    m_outProcess = EventSyncFactory::create("Pointer2Events", param);
    m_process = EventSyncFactory::create("Events2Pointer", param);
  }

  bool data2Pointer_test_processor::pushEvent(event_sp ev, size_t Index /*= 0*/)
  {
   return m_process->pushEvent(ev);
  }

  bool data2Pointer_test_processor::getNextEvent(event_sp& ev)
  {

    event_sp dummy;

    while(m_process->getNextEvent(dummy))
    {
      m_outProcess->pushEvent(dummy);
    }

   

    if (!m_outProcess->getNextEvent(ev))
    {
      return false;
    }

    return true;
  }



  registerSyncClass(data2Pointer_test_processor, "pointerTest");
}