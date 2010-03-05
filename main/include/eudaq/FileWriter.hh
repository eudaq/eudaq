#ifndef EUDAQ_INCLUDED_FileWriter
#define EUDAQ_INCLUDED_FileWriter

#include "eudaq/DetectorEvent.hh"
#include <vector>
#include <string>

namespace eudaq {

  class FileWriter {
  public:
    FileWriter();
    virtual void StartRun(unsigned runnumber) = 0;
    virtual void WriteEvent(const DetectorEvent &) = 0;
    virtual unsigned long long FileBytes() const = 0;
    void SetFilePattern(const std::string & p) { m_filepattern = p; }
    virtual ~FileWriter() {}
  protected:
    std::string m_filepattern;
  };


  class FileWriterFactory {
  public:
    static FileWriter * Create(const std::string & name, const std::string & params = "");
    template <typename T>
    static void Register(const std::string & name) {
      do_register(name, filewriterfactory<T>);
    }
    typedef FileWriter * (*factoryfunc)(const std::string &);
    static std::vector<std::string> GetTypes();
  private:
    template <typename T>
    static FileWriter * filewriterfactory(const std::string & params) {
      return new T(params);
    }
    static void do_register(const std::string & name, factoryfunc);
  };

  template <typename T>
  class RegisterFileWriter {
  public:
    RegisterFileWriter(const std::string & name) {
      FileWriterFactory::Register<T>(name);
    }
  };

}

#endif // EUDAQ_INCLUDED_FileWriter
