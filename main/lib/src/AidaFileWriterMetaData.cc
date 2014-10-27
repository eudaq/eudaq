#include "eudaq/AidaFileWriter.hh"
#include "eudaq/AidaIndexData.hh"
#include "eudaq/FileNamer.hh"
#include "eudaq/FileSerializer.hh"
#include "eudaq/BufferSerializer.hh"
#include "eudaq/Exception.hh"

#include "config.h"
#include "jsoncons/json.hpp"

using std::string;
using jsoncons::null_type;
using jsoncons::json;


namespace eudaq {

  


  class AidaFileWriterMetaData : public AidaFileWriter {
    public:
      AidaFileWriterMetaData(const std::string &);
      virtual void StartRun(unsigned);
      virtual void WritePacket( std::shared_ptr<AidaPacket> );
      virtual unsigned long long FileBytes() const;
      virtual ~AidaFileWriterMetaData();
    private:
      FileSerializer * m_ser;
      
  };

  namespace {
    static RegisterAidaFileWriter<AidaFileWriterMetaData> reg("metaData");
  }

  AidaFileWriterMetaData::AidaFileWriterMetaData(const std::string & /*param*/) : m_ser(nullptr) {
    //EUDAQ_DEBUG("Constructing AidaFileWriterNative(" + to_string(param) + ")");
  }

  void AidaFileWriterMetaData::StartRun(unsigned runnumber) {
	delete m_ser;

    m_ser   = new FileSerializer(FileNamer(m_filepattern).Set('X', ".raw2").Set( 'S', "_" ).Set('N', 0 ).Set('R', runnumber));
 

    json header;
    header["runnumber"] = runnumber;
    header["package_name"] = PACKAGE_NAME;
    header["package_version"] = PACKAGE_VERSION;
    header["date"] = Time::Current().Formatted();
    // std::cout << "JSON: " << header << std::endl;
    m_ser->write( header.to_string() );
    m_ser->Flush();

  }

  void AidaFileWriterMetaData::WritePacket(std::shared_ptr<AidaPacket> packet) {

    if (!m_ser)
    	EUDAQ_THROW("AidaFileWriterNative: Attempt to write unopened file");
    std::string line = AidaPacket::type2str(packet->GetPacketType()) + " ;  " + AidaPacket::type2str(packet->GetPacketSubType()) + " ; "+ to_string(packet->GetPacketNumber()) + " ; " + to_string(packet->GetMetaData().getTriggerID(0)) + " \n";
      std::cout << line;
      m_ser->write(line );
	    m_ser->Flush();

  }

  AidaFileWriterMetaData::~AidaFileWriterMetaData() {
    delete m_ser;

  }


  unsigned long long AidaFileWriterMetaData::FileBytes() const {
	  return m_ser ? m_ser->FileBytes() : 0;
  }


  

}
