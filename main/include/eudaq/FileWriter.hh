#ifndef EUDAQ_INCLUDED_FileWriter
#define EUDAQ_INCLUDED_FileWriter

#include "eudaq/DetectorEvent.hh"
#include <vector>
#include <string>

namespace eudaq {

  class DLLEXPORT FileWriter {
    public:
      FileWriter();
      virtual void StartRun(unsigned runnumber) = 0;
      virtual void StartRun( const std::string & name, unsigned int runnumber) {};
      virtual void WriteEvent(const DetectorEvent &) = 0;
      virtual uint64_t FileBytes() const = 0;
      void SetFilePattern(const std::string & p) { m_filepattern = p; }
      virtual ~FileWriter() {}
    protected:
      std::string m_filepattern;
      std::string m_name;
      unsigned int m_runNumber;
      unsigned int m_fileNumber;
  };


  class DLLEXPORT FileWriterFactory {
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
