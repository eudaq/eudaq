#include "eudaq/AidaFileWriter.hh"
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

  AidaFileWriter::AidaFileWriter() : m_filepattern(FileNamer::default_pattern) {}


  class AidaFileWriterNative : public AidaFileWriter {
    public:
      AidaFileWriterNative(const std::string &);
      virtual void StartRun(unsigned);
      virtual void WritePacket(const AidaPacket &);
      virtual unsigned long long FileBytes() const;
      virtual ~AidaFileWriterNative();
    private:
      FileSerializer * m_ser;
  };

  namespace {
    static RegisterAidaFileWriter<AidaFileWriterNative> reg("native");
  }

  AidaFileWriterNative::AidaFileWriterNative(const std::string & /*param*/) : m_ser(0) {
    //EUDAQ_DEBUG("Constructing AidaFileWriterNative(" + to_string(param) + ")");
  }

  void AidaFileWriterNative::StartRun(unsigned runnumber) {
    delete m_ser;
    m_ser = new FileSerializer(FileNamer(m_filepattern).Set('X', ".raw2").Set('R', runnumber));

    json header;
    header["runnumber"] = runnumber;
    header["package_name"] = PACKAGE_NAME;
    header["package_version"] = PACKAGE_VERSION;
    header["date"] = Time::Current().Formatted();
    // std::cout << "JSON: " << header << std::endl;
    m_ser->write( header.to_string() );
    m_ser->Flush();
  }

  void AidaFileWriterNative::WritePacket(const AidaPacket & packet ) {
    if (!m_ser)
    	EUDAQ_THROW("AidaFileWriterNative: Attempt to write unopened file");
    m_ser->write( packet );
    m_ser->Flush();
  }

  AidaFileWriterNative::~AidaFileWriterNative() {
    delete m_ser;
  }


  unsigned long long AidaFileWriterNative::FileBytes() const {
	  return m_ser ? m_ser->FileBytes() : 0;
  }


  class AidaFileWriterNative2 : public AidaFileWriter {
    public:
      AidaFileWriterNative2(const std::string &);
      virtual void StartRun(unsigned);
      virtual void WritePacket(const AidaPacket &);
      virtual unsigned long long FileBytes() const;
      virtual ~AidaFileWriterNative2();
    private:
      BufferSerializer m_buf;
      FileSerializer * m_ser;
  };

  namespace {
    static RegisterAidaFileWriter<AidaFileWriterNative2> reg2("native2");
  }

  AidaFileWriterNative2::AidaFileWriterNative2(const std::string & /*param*/) : m_ser(0) {
    //EUDAQ_DEBUG("Constructing AidaFileWriterNative(" + to_string(param) + ")");
  }

  void AidaFileWriterNative2::StartRun(unsigned runnumber) {
    delete m_ser;
    m_ser = new FileSerializer(FileNamer(m_filepattern).Set('X', ".raw2").Set('R', runnumber));
    json header;
    header["runnumber"] = runnumber;
    header["package_name"] = PACKAGE_NAME;
    header["package_version"] = PACKAGE_VERSION;
    std::cout << "JSON: " << header << std::endl;

    unsigned versiontag = Event::str2id("VER2");
    m_ser->write(versiontag);
  }

  void AidaFileWriterNative2::WritePacket(const AidaPacket & packet) {
    if (!m_ser)
    	EUDAQ_THROW("AidaFileWriterNative: Attempt to write unopened file");
    m_buf.clear();
    m_buf.write( packet );
    m_ser->write(m_buf);
    m_ser->Flush();
  }

  AidaFileWriterNative2::~AidaFileWriterNative2() {
    delete m_ser;
  }

  unsigned long long AidaFileWriterNative2::FileBytes() const {
	  return m_ser ? m_ser->FileBytes() : 0;
  }


  namespace {
    typedef std::map<std::string, AidaFileWriterFactory::factoryfunc> map_t;

    static map_t & AidaFileWriterMap() {
      static map_t m;
      return m;
    }
  }

  void AidaFileWriterFactory::do_register(const std::string & name, AidaFileWriterFactory::factoryfunc func) {
    //std::cout << "DEBUG: Registering AidaFileWriter: " << name << std::endl;
    AidaFileWriterMap()[name] = func;
  }

  AidaFileWriter * AidaFileWriterFactory::Create(const std::string & name, const std::string & params) {
    map_t::const_iterator it = AidaFileWriterMap().find(name == "" ? "native" : name);
    if (it == AidaFileWriterMap().end())
    	EUDAQ_THROW("Unknown file writer: " + name);
    return (it->second)(params);
  }

  std::vector<std::string> AidaFileWriterFactory::GetTypes() {
    std::vector<std::string> result;
    for (map_t::const_iterator it = AidaFileWriterMap().begin(); it != AidaFileWriterMap().end(); ++it) {
      result.push_back(it->first);
    }
    return result;
  }



}
