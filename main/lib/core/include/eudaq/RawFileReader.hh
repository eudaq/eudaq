#ifndef EUDAQ_INCLUDED_FileReader
#define EUDAQ_INCLUDED_FileReader

#include "FileSerializer.hh"
#include "DetectorEvent.hh"
#include "StandardEvent.hh"
#include "FileReader.hh"

#include <string>
#include <memory>

namespace eudaq {

  class DLLEXPORT RawFileReader : public FileReader {
  public:
    RawFileReader(const std::string & filename, const std::string & filepattern);
    RawFileReader(const std::string& filename);
    // RawFileReader(Parameter_ref param);
    ~RawFileReader();
    
    virtual unsigned RunNumber() const;
    virtual bool NextEvent(size_t skip = 0);
    virtual EventSPC GetNextEvent();
    virtual void Interrupt() { m_des.Interrupt(); }

  private:
    FileDeserializer m_des;
    EventSP m_ev;
    unsigned m_ver;
  };
}

#endif // EUDAQ_INCLUDED_FileReader
