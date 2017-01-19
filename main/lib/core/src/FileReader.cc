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
 
}
