#include <list>
#include "eudaq/FileReader.hh"
#include "eudaq/Exception.hh"
#include "eudaq/OptionParser.hh"

namespace eudaq {

  template DLLEXPORT
  std::map<uint32_t, typename Factory<FileReader>::UP (*)(std::string&)>& Factory<FileReader>::Instance<std::string&>();
  template DLLEXPORT
  std::map<uint32_t, typename Factory<FileReader>::UP (*)(std::string&&)>& Factory<FileReader>::Instance<std::string&&>();

  FileReader::FileReader(){
  }

  FileReader::~FileReader(){ 
  }

  FileReaderSP FileReader::Make(std::string type, std::string path){
      auto fw = eudaq::Factory<eudaq::FileReader>::MakeShared(eudaq::str2hash(type), path);
      if(!fw)
	EUDAQ_THROW("FileReader: There is no FileReader regiested by "+ type);
      return fw;
  }
}
