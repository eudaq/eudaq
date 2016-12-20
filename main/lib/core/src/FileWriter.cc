#include "FileWriter.hh"
#include "FileNamer.hh"
#include "Exception.hh"

namespace eudaq {

  template class DLLEXPORT Factory<FileWriter>;
  template DLLEXPORT  std::map<uint32_t, typename Factory<FileWriter>::UP_BASE (*)(std::string&)>&
  Factory<FileWriter>::Instance<std::string&>();
  template DLLEXPORT  std::map<uint32_t, typename Factory<FileWriter>::UP_BASE (*)(std::string&&)>&
  Factory<FileWriter>::Instance<std::string&&>();
  
  FileWriter::FileWriter(){}
  
}
