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

  AidaFileWriter::AidaFileWriter()
      : m_filepattern(FileNamer::default_pattern) {}

  class AidaFileWriterNative : public AidaFileWriter {
  public:
    AidaFileWriterNative(const std::string &);
    virtual void StartRun(unsigned);
    virtual void WritePacket(std::shared_ptr<AidaPacket>);
    virtual unsigned long long FileBytes() const;
    virtual ~AidaFileWriterNative();

  private:
    FileSerializer *m_ser;
    FileSerializer *m_idx;
  };

  namespace {
    static RegisterAidaFileWriter<AidaFileWriterNative> reg("native");
  }

  AidaFileWriterNative::AidaFileWriterNative(const std::string & /*param*/)
      : m_ser(0), m_idx(0) {
    // EUDAQ_DEBUG("Constructing AidaFileWriterNative(" + to_string(param) +
    // ")");
  }

  void AidaFileWriterNative::StartRun(unsigned runnumber) {
    delete m_ser;
    delete m_idx;
    m_ser = new FileSerializer(FileNamer(m_filepattern)
                                   .Set('X', ".raw2")
                                   .Set('S', "_")
                                   .Set('N', 0)
                                   .Set('R', runnumber));
    m_idx = new FileSerializer(
        FileNamer(m_filepattern).Set('X', ".idx").Set('R', runnumber));

    json header;
    header["runnumber"] = runnumber;
    header["package_name"] = PACKAGE_NAME;
    header["package_version"] = PACKAGE_VERSION;
    header["date"] = Time::Current().Formatted();
    // std::cout << "JSON: " << header << std::endl;
    m_ser->write(header.to_string());
    m_ser->Flush();
    m_idx->write(header.to_string());
    m_idx->Flush();
  }

  void AidaFileWriterNative::WritePacket(std::shared_ptr<AidaPacket> packet) {
    if (!m_idx)
      EUDAQ_THROW("AidaFileWriterNative: Attempt to write unopened index file");
    m_idx->write(AidaIndexData(*packet, 42 /* fileNo */, m_ser->FileBytes()));
    m_idx->Flush();

    if (!m_ser)
      EUDAQ_THROW("AidaFileWriterNative: Attempt to write unopened file");
    m_ser->write(*packet);
    m_ser->Flush();
  }

  AidaFileWriterNative::~AidaFileWriterNative() {
    delete m_ser;
    delete m_idx;
  }

  unsigned long long AidaFileWriterNative::FileBytes() const {
    return m_ser ? m_ser->FileBytes() : 0;
  }

  namespace {
    typedef std::map<std::string, AidaFileWriterFactory::factoryfunc> map_t;

    static map_t &AidaFileWriterMap() {
      static map_t m;
      return m;
    }
  }

  void
  AidaFileWriterFactory::do_register(const std::string &name,
                                     AidaFileWriterFactory::factoryfunc func) {
    // std::cout << "DEBUG: Registering AidaFileWriter: " << name << std::endl;
    AidaFileWriterMap()[name] = func;
  }

  AidaFileWriter *AidaFileWriterFactory::Create(const std::string &name,
                                                const std::string &params) {
    const auto it = AidaFileWriterMap().find(name == "" ? "native" : name);
    if (it == AidaFileWriterMap().end())
      EUDAQ_THROW("Unknown file writer: " + name);
    return (it->second)(params);
  }

  std::vector<std::string> AidaFileWriterFactory::GetTypes() {
    std::vector<std::string> result;
    for (const auto& it:AidaFileWriterMap()) { result.emplace_back(it.first); }
    return result;
  }
}
