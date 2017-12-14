#ifndef EUDAQ_INCLUDED_AidaFileWriter
#define EUDAQ_INCLUDED_AidaFileWriter

#include "eudaq/AidaPacket.hh"
#include <vector>
#include <string>

namespace eudaq {

  class DLLEXPORT AidaFileWriter {
  public:
    virtual void StartRun(unsigned runnumber) = 0;
    virtual void WritePacket(std::shared_ptr<AidaPacket>) = 0;
    virtual unsigned long long FileBytes() const = 0;
    void SetFilePattern(const std::string &p) { m_filepattern = p; }
    virtual ~AidaFileWriter() {}

  protected:
    friend class AidaFileWriterFactory;
    AidaFileWriter();

    std::string m_filepattern;
  };

  class DLLEXPORT AidaFileWriterFactory {
  public:
    static AidaFileWriter *Create(const std::string &name,
                                  const std::string &params = "");
    template <typename T> static void Register(const std::string &name) {
      do_register(name, filewriterfactory<T>);
    }
    typedef AidaFileWriter *(*factoryfunc)(const std::string &);
    static std::vector<std::string> GetTypes();

  private:
    template <typename T>
    static AidaFileWriter *filewriterfactory(const std::string &params) {
      return new T(params);
    }
    static void do_register(const std::string &name, factoryfunc);
  };

  template <typename T> class RegisterAidaFileWriter {
  public:
    RegisterAidaFileWriter(const std::string &name) {
      AidaFileWriterFactory::Register<T>(name);
    }
  };
}

#endif // EUDAQ_INCLUDED_AidaFileWriter
