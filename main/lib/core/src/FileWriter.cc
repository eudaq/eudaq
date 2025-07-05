#include "eudaq/FileWriter.hh"
#include "eudaq/FileNamer.hh"
#include "eudaq/Exception.hh"

namespace eudaq {

  template class DLLEXPORT Factory<FileWriter>;
  template DLLEXPORT  std::map<uint32_t, typename Factory<FileWriter>::UP_BASE (*)(std::string&)>&
  Factory<FileWriter>::Instance<std::string&>();
  template DLLEXPORT  std::map<uint32_t, typename Factory<FileWriter>::UP_BASE (*)(std::string&&)>&
  Factory<FileWriter>::Instance<std::string&&>();
  
  FileWriter::FileWriter(){}

  FileWriterSP FileWriter::Make(std::string type, std::string path){
    auto fw = eudaq::Factory<eudaq::FileWriter>::MakeShared(eudaq::str2hash(type), path);
    if(!fw)
      EUDAQ_THROW("FileWriter: There is no FileWriter regiested by "+ type);
    return fw;
  }

}
