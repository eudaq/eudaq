#ifndef FILEREADER_HH__
#define FILEREADER_HH__
#include <string>
#include <memory>
#include "eudaq/Platform.hh"
#include "eudaq/ClassFactory.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/Factory.hh"

#define RegisterFileReader(derivedClass,ID) REGISTER_DERIVED_CLASS(eudaq::FileReader,derivedClass,ID)

namespace eudaq{
  class FileReader;  
#ifndef EUDAQ_CORE_EXPORTS
  extern template class DLLEXPORT
  Factory<FileReader>;
  extern template DLLEXPORT std::map<uint32_t, typename Factory<FileReader>::UP(*)(std::string&)>&
  Factory<FileReader>::Instance<std::string&>();
  extern template DLLEXPORT std::map<uint32_t, typename Factory<FileReader>::UP(*)(std::string&&)>&
  Factory<FileReader>::Instance<std::string&&>();
#endif
  using FileReaderUP = Factory<FileReader>::UP;
  using FileReaderSP = Factory<FileReader>::SP;

  class Event;
  class OptionParser;
  class fileConfig;
  using FileReader_up = FileReaderUP;

  class DLLEXPORT FileReader{
  public:
    static const char* getKeyFileName();
    static const char* getKeyInputPattern();
    static const char* getKeySectionName();
    using MainType = std::string;
    using Parameter_t = eudaq::Configuration;
    using Parameter_ref = const Parameter_t&;
    static Parameter_t getConfiguration(const std::string& fileName, const std::string& filePattern);
    FileReader(Parameter_ref config);
    FileReader(const std::string&  fileName);
    std::string Filename()const;
    std::string InputPattern() const;
    Parameter_ref getConfiguration()const;
    Parameter_t&  getConfiguration();
    virtual unsigned RunNumber() const = 0;
    virtual bool NextEvent(size_t skip = 0) = 0;
    virtual std::shared_ptr<eudaq::Event> GetNextEvent() = 0;
    virtual const eudaq::Event & GetEvent() const = 0;
    virtual std::shared_ptr<eudaq::Event> getEventPtr()  = 0 ;
    virtual void Interrupt();

  private:
    Parameter_t m_config;
  };



  class DLLEXPORT FileReaderFactory{
  public:
    
    static FileReader_up create(const std::string & filename, const std::string & filepattern);
    static FileReader_up create(const std::string & filename);
    static FileReader_up create(FileReader::MainType type, FileReader::Parameter_ref  param);
    static FileReader_up create(eudaq::OptionParser & op);
    static FileReader_up create(const fileConfig& file_conf_);

    static void addComandLineOptions(eudaq::OptionParser & op);
    static std::string Help_text();
    static std::string getDefaultInputpattern();

  private:

    class Impl;
    static Impl& getImpl();

  };

  class DLLEXPORT fileConfig {
  public:
    fileConfig(eudaq::OptionParser & op);
    explicit fileConfig(const std::string& fileName);
    std::string get() const;
  private:
    std::string m_type;
  };
}

#endif // FileReader_h__
