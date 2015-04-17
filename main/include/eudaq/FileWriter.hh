#ifndef EUDAQ_INCLUDED_FileWriter
#define EUDAQ_INCLUDED_FileWriter

#include <vector>
#include <string>
#include <memory>

#include "eudaq/DetectorEvent.hh"
#include "eudaq/factory.hh"



#define registerFileWriter(DerivedFileWriter,ID)  registerClass(FileWriter,DerivedFileWriter,ID)

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
    void SetFilePattern(const std::string & p) { m_filepattern = p; }
    virtual ~FileWriter() {}
  protected:
    std::string m_filepattern;
  };



  inline void helper_ProcessEvent(FileWriter& writer, const Event &ev){ writer.WriteBaseEvent(ev); }
  inline void helper_StartRun(FileWriter& writer, unsigned runnumber){ writer.StartRun(runnumber); }
  inline void helper_EndRun(FileWriter& writer){};





  class DLLEXPORT FileWriterFactory {
  public:
    static std::unique_ptr<FileWriter> Create(const std::string & name, const std::string & params = "");
    static std::unique_ptr<FileWriter> Create();
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
