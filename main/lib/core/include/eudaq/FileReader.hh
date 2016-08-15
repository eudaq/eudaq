#ifndef EUDAQ_INCLUDED_FileReader
#define EUDAQ_INCLUDED_FileReader

#include "FileSerializer.hh"
#include "DetectorEvent.hh"
#include "StandardEvent.hh"
#include "baseFileReader.hh"

#include <string>
#include <memory>

namespace eudaq {

  class DLLEXPORT FileReader : public baseFileReader {
  public:
    FileReader(const std::string & filename, const std::string & filepattern = "");
    FileReader(Parameter_ref param);
    ~FileReader();
    
    virtual unsigned RunNumber() const;
    virtual bool NextEvent(size_t skip = 0);
    virtual EventSP getEventPtr() { return m_ev; }
    virtual EventSP GetNextEvent();
    virtual void Interrupt() { m_des.Interrupt(); }

    const eudaq::Event & GetEvent() const;
    const DetectorEvent & Event() const { return GetDetectorEvent(); } // for backward compatibility
    const DetectorEvent & GetDetectorEvent() const;
    const StandardEvent & GetStandardEvent() const;
    std::shared_ptr<eudaq::DetectorEvent> GetDetectorEvent_ptr(){ return std::dynamic_pointer_cast<eudaq::DetectorEvent>(m_ev); };

  private:
    FileDeserializer m_des;
    EventSP m_ev;
    unsigned m_ver;
    size_t m_subevent_counter = 0;
  };
}

#endif // EUDAQ_INCLUDED_FileReader
