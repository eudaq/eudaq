#ifndef EUDAQ_INCLUDED_FileWriter
#define EUDAQ_INCLUDED_FileWriter

#include <vector>
#include <string>
#include <memory>

#include "DetectorEvent.hh"
#include "ClassFactory.hh"

#define registerFileWriter(DerivedFileWriter,ID)  REGISTER_DERIVED_CLASS(FileWriter,DerivedFileWriter,ID)

namespace eudaq {
  
  class OptionParser;
  class DLLEXPORT FileWriter {
  public:
    using MainType = std::string;
    using Parameter_t = std::string;
    using Parameter_ref = const Parameter_t&;
    FileWriter();
    virtual void StartRun(unsigned runnumber) = 0;
    virtual void WriteBaseEvent(const Event&);
    virtual void WriteEvent(const DetectorEvent &) = 0;
    virtual uint64_t FileBytes() const = 0;
    void SetFilePattern(const std::string &p) { m_filepattern = p; }
    virtual ~FileWriter() {}

  protected:
    std::string m_filepattern;
  };

  using FileWriter_up = std::unique_ptr<FileWriter, std::function<void(FileWriter*)> >;

  
  class DLLEXPORT FileWriterFactory {
  public:
    
    static FileWriter_up Create(const std::string & name, const std::string & params = "");
    static FileWriter_up Create();
    static std::vector<std::string> GetTypes();

    static void addComandLineOptions(OptionParser & op);
    static std::string  Help_text();
    static std::string getDefaultType();
    static std::string getDefaultOutputPattern();
  private:
    class Impl;
    static Impl& getImpl();


  };
}

#endif // EUDAQ_INCLUDED_FileWriter
