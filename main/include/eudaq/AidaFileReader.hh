#ifndef EUDAQ_INCLUDED_AidaFileReader
#define EUDAQ_INCLUDED_AidaFileReader

#include <inttypes.h> /* uint64_t */
#include <string>
#include <memory>
#include <vector>
#include "eudaq/Platform.hh"

namespace eudaq {

class FileDeserializer;
class AidaPacket;

  class DLLEXPORT AidaFileReader {
    public:
      AidaFileReader(const std::string & filename );

      virtual ~AidaFileReader();
      bool readNext();
      std::string Filename() const { return m_filename; }
      unsigned RunNumber() const { return m_runNumber; }
      std::string getJsonConfig() { return m_json_config; }
      std::string getJsonPacketInfo();
      std::vector<uint64_t> getData();
      std::shared_ptr<eudaq::AidaPacket> GetPacket() const { return m_packet; };
      
    private:
      std::string m_filename;
      unsigned long long m_runNumber;
      FileDeserializer * m_des;
      std::string m_json_config;
      std::shared_ptr<eudaq::AidaPacket> m_packet;
  };
 
}


#endif // EUDAQ_INCLUDED_AidaFileReader
