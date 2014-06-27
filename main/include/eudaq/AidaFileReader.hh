#ifndef EUDAQ_INCLUDED_AidaFileReader
#define EUDAQ_INCLUDED_AidaFileReader

#include "eudaq/FileSerializer.hh"
#include "eudaq/AidaPacket.hh"

#include <string>

#include <memory>

namespace eudaq {


  class DLLEXPORT AidaFileReader {
    public:
      AidaFileReader(const std::string & filename, const std::string & filepattern = "");

      ~AidaFileReader();
      bool NextPacket(size_t skip = 0);
      std::string Filename() const { return m_filename; }
      unsigned RunNumber() const;
      const eudaq::AidaPacket & GetPacket() const;
      void Interrupt() { m_des.Interrupt(); }
      
    private:
      std::string m_filename;
      FileDeserializer m_des;
     std::shared_ptr<eudaq::AidaPacket> m_ev;
      unsigned m_ver;

  };
 


}

#endif // EUDAQ_INCLUDED_AidaFileReader
