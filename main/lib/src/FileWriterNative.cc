#include "eudaq/FileNamer.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/FileSerializer.hh"
//#include "eudaq/Logger.hh"

namespace eudaq {

  class FileWriterNative : public FileWriter {
    public:
      FileWriterNative(const std::string &);
      virtual void StartRun(unsigned);
      virtual void StartRun( const std::string & name, unsigned int runnumber );
      virtual void WriteEvent(const DetectorEvent &);
      virtual uint64_t FileBytes() const;
      virtual ~FileWriterNative();
    private:
      void OpenNextRaw();
      FileSerializer * m_ser;
  };

  namespace {
    static RegisterFileWriter<FileWriterNative> reg("native");
  }

  FileWriterNative::FileWriterNative(const std::string & /*param*/) : m_ser(0) {
    //EUDAQ_DEBUG("Constructing FileWriterNative(" + to_string(param) + ")");
  }

  void FileWriterNative::StartRun(unsigned runnumber) {
    delete m_ser;
    m_ser = new FileSerializer(FileNamer(m_filepattern).Set('X', ".raw").Set('R', runnumber));
  }

  void FileWriterNative::StartRun( const std::string & name, unsigned int runnumber )
  {
       m_name = name;
       m_runNumber = runnumber;
       m_fileNumber = 0;

       OpenNextRaw();
  }

  void FileWriterNative::OpenNextRaw() {
       m_fileNumber += 1;
       if ( m_ser ) {
               m_ser->Flush();
               delete m_ser;
       }
    m_ser = new FileSerializer(FileNamer(m_filepattern).Set('X', ".raw").Set( 's', "_" ).Set('i', m_fileNumber ).Set('R', m_runNumber).Set('N', m_name));

    m_ser->Flush();
  }


  void FileWriterNative::WriteEvent(const DetectorEvent & ev) {
    if (!m_ser) EUDAQ_THROW("FileWriterNative: Attempt to write unopened file");
    m_ser->write(ev);
    m_ser->Flush();
  }

  FileWriterNative::~FileWriterNative() {
    delete m_ser;
  }

  uint64_t FileWriterNative::FileBytes() const { return m_ser ? m_ser->FileBytes() : 0; }

}
