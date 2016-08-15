#include "FileWriter.hh"
#include "FileNamer.hh"
#include "Exception.hh"

namespace eudaq {

  template class DLLEXPORT Factory<FileWriter>;
  template DLLEXPORT  std::map<uint32_t, typename Factory<FileWriter>::UP_BASE (*)(std::string&)>&
  Factory<FileWriter>::Instance<std::string&>();
  template DLLEXPORT  std::map<uint32_t, typename Factory<FileWriter>::UP_BASE (*)(std::string&&)>&
  Factory<FileWriter>::Instance<std::string&&>();
  
  FileWriter::FileWriter() : m_filepattern(FileNamer::default_pattern) {}

  void FileWriter::WriteBaseEvent(const Event& ev){
    auto dev = dynamic_cast<const eudaq::DetectorEvent *>(&ev);
    if (dev){
      WriteEvent(*dev);
    }
    else{
      EUDAQ_THROW("unable to convert event into detector Event. this can happen when you try to use the sync algorithm which does not create detector events.");
    }
  }
 
}
