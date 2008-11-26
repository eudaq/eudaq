#include "eudaq/FileWriter.hh"
#include "eudaq/FileNamer.hh"
#include "eudaq/Exception.hh"

namespace eudaq {

  namespace {
    typedef std::map<std::string, FileWriterFactory::factoryfunc> map_t;

    static map_t & FileWriterMap() {
      static map_t m;
      return m;
    }
  }


  void FileWriterFactory::do_register(const std::string & name, FileWriterFactory::factoryfunc func) {
    FileWriterMap()[name] = func;
  }

  FileWriter * FileWriterFactory::Create(const std::string & name, const std::string & params) {
    map_t::const_iterator it = FileWriterMap().find(name == "" ? "native" : name);
    if (it == FileWriterMap().end()) EUDAQ_THROW("Unknown file writer: " + name);
    return (it->second)(params);
  }

  FileWriter::FileWriter() : m_filepattern(FileNamer::default_pattern) {}

}
