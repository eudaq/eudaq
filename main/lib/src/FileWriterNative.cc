#include "eudaq/FileNamer.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/FileSerializer.hh"
#include "eudaq/PluginManager.hh"
//#include "eudaq/Logger.hh"

namespace eudaq {

  class FileWriterNative : public FileWriter {
    public:
      FileWriterNative(const std::string &);
      virtual void StartRun(unsigned);
      virtual void WriteEvent(const DetectorEvent &);
      virtual void WriteBaseEvent(const Event&);
      virtual uint64_t FileBytes() const;
      virtual ~FileWriterNative();
    private:
      FileSerializer * m_ser;
      std::string m_ending;
  };

  
  registerFileWriter(FileWriterNative, "native");
  
  FileWriterNative::FileWriterNative(const std::string & param) : m_ser(0){
    //EUDAQ_DEBUG("Constructing FileWriterNative(" + to_string(param) + ")");
    
    if (param.empty())
    {
      m_ending = ".raw";
    }
    else
    {
      m_ending = param;
    }
  }

  void FileWriterNative::StartRun(unsigned runnumber) {
    delete m_ser;
    m_ser = new FileSerializer(FileNamer(m_filepattern).Set('X', m_ending).Set('R', runnumber), true);
  }

  void FileWriterNative::WriteEvent(const DetectorEvent & ev) {
    if (ev.IsBORE()) {
      eudaq::PluginManager::Initialize(ev);
    }
    if (!m_ser) EUDAQ_THROW("FileWriterNative: Attempt to write unopened file");
    m_ser->write(ev);
    m_ser->Flush();
  }
  void FileWriterNative::WriteBaseEvent(const Event& ev){
    
    if (ev.IsBORE()) {
      auto det = dynamic_cast<const DetectorEvent*>(&ev);
      if (det)
      {
        eudaq::PluginManager::Initialize(*det);

      }
    }
    if (!m_ser) EUDAQ_THROW("FileWriterNative: Attempt to write unopened file");
    m_ser->write(ev);
    m_ser->Flush();
  }

  FileWriterNative::~FileWriterNative() {
    delete m_ser;
  }

  uint64_t FileWriterNative::FileBytes() const { return m_ser ? m_ser->FileBytes() : 0; }

}
