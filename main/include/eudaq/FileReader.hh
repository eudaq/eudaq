#ifndef EUDAQ_INCLUDED_FileReader
#define EUDAQ_INCLUDED_FileReader

#include "eudaq/FileSerializer.hh"
#include "eudaq/DetectorEvent.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/counted_ptr.hh"
#include <string>

namespace eudaq {

  class FileReader {
  public:
    FileReader(const std::string & filename, const std::string & filepattern = "");
    bool NextEvent(size_t skip = 0);
    std::string Filename() const { return m_filename; }
    unsigned RunNumber() const;
    const DetectorEvent & Event() const;
    //const StandardEvent & GetStandardEvent() const;
    void Interrupt() { m_des.Interrupt(); }
  private:
    std::string m_filename;
    FileDeserializer m_des;
    counted_ptr<eudaq::Event> m_ev;
    unsigned m_ver;
    //mutable counted_ptr<eudaq::StandardEvent> m_sev;
  };

}

#endif // EUDAQ_INCLUDED_FileReader
