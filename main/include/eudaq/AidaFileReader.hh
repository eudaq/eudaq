#ifndef EUDAQ_INCLUDED_AidaFileReader
#define EUDAQ_INCLUDED_AidaFileReader

#include <inttypes.h> /* uint64_t */
#include <string>
#include <memory>
#include <vector>
#include "eudaq/Platform.hh"
#include "eudaq/baseFileReader.hh"


namespace eudaq {

class FileDeserializer;
class AidaPacket;
class EventPacket;

  class DLLEXPORT AidaFileReader :public baseFileReader{
    public:
      AidaFileReader(const std::string & filename );

      virtual ~AidaFileReader();
      bool readNext();
	  virtual std::shared_ptr<eudaq::Event> GetNextEvent();
	 // std::string Filename() const { return m_filename; }
      virtual unsigned RunNumber() const { return m_runNumber; }
      std::string getJsonConfig() { return m_json_config; }
      std::string getJsonPacketInfo();
      std::vector<uint64_t> getData();
      std::shared_ptr<eudaq::AidaPacket> GetPacket() const { return m_packet; };
	 
    private:
      //std::string m_filename;
		std::shared_ptr<eudaq::Event> GetNextEventFromPacket();
		std::shared_ptr<eudaq::Event> GetNextEventFromEventPacket(std::shared_ptr<EventPacket>& eventPack);
      unsigned long long m_runNumber;
      FileDeserializer * m_des;
      std::string m_json_config;
      std::shared_ptr<eudaq::AidaPacket> m_packet;
	  size_t itter = 0;
  };
 
  bool FileIsAIDA(const std::string& in);
}


#endif // EUDAQ_INCLUDED_AidaFileReader
