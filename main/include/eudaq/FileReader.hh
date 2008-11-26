#ifndef EUDAQ_INCLUDED_FileReader
#define EUDAQ_INCLUDED_FileReader

#include "eudaq/FileSerializer.hh"
#include "eudaq/DetectorEvent.hh"
#include "eudaq/counted_ptr.hh"

namespace eudaq {

  class FileReader {
  public:
    FileReader(const std::string & filename, const std::string & filepattern = "");
    bool NextEvent();
    unsigned RunNumber() const;
    const DetectorEvent & Event() const;
  private:
    FileDeserializer m_des;
    counted_ptr<eudaq::Event> m_ev;
  };

}

#endif // EUDAQ_INCLUDED_FileReader
