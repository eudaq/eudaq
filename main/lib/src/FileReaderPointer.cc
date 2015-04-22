#include <list>
#include "eudaq/FileNamer.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/Event.hh"
#include "eudaq/Logger.hh"
#include "eudaq/FileSerializer.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/factory.hh"
#include "eudaq/baseFileReader.hh"
#include "eudaq/PointerEvent.hh"
#include "eudaq/EventSynchronisationBase.hh"

namespace eudaq {


  class FileReaderPointer : public baseFileReader{
  public:
    using counter_t = PointerEvent::counter_t;
    using reference_t = PointerEvent::reference_t;

    FileReaderPointer(Parameter_ref param);

    virtual  unsigned RunNumber() const;
    virtual bool NextEvent(size_t skip = 0);
    virtual const eudaq::Event & GetEvent() const;
    virtual std::shared_ptr<eudaq::Event> getEventPtr() { return m_ev; }
    virtual std::shared_ptr<eudaq::Event> GetNextEvent();
    virtual void Interrupt() { m_read->Interrupt(); }

 


  private:
    std::unique_ptr<baseFileReader> m_read;
    std::unique_ptr<SyncBase> m_sync;
    event_sp m_ev;
  };




  FileReaderPointer::FileReaderPointer(Parameter_ref param) : baseFileReader(param)
  {
    m_read = FileReaderFactory::create("raw", param);
    m_sync = EventSyncFactory::create("Pointer2Events", param);

    auto ev = m_read->getEventPtr();
    m_sync->pushEvent(ev);
    
    NextEvent();


  }


  bool FileReaderPointer::NextEvent(size_t skip) {
    
    while (!m_sync->getNextEvent(m_ev))
    {
      auto ev = m_read->GetNextEvent();
      if (!m_sync->pushEvent(ev)){
        return false;
      }
    }
    return true;
  }

  unsigned FileReaderPointer::RunNumber() const {
    return m_ev->GetRunNumber();
  }
  std::shared_ptr<eudaq::Event> FileReaderPointer::GetNextEvent(){
    if (!NextEvent()) {
      return nullptr;
    }
    return m_ev;
  }

  const eudaq::Event & FileReaderPointer::GetEvent() const
  {
    return *m_ev;
  }






  RegisterFileReader(FileReaderPointer, "raw3");

}
