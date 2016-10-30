#include <list>
#include "eudaq/FileReader.hh"
#include "eudaq/Exception.hh"
#include "eudaq/OptionParser.hh"

namespace eudaq {

  template DLLEXPORT
  std::map<uint32_t, typename Factory<FileReader>::UP (*)(std::string&)>& Factory<FileReader>::Instance<std::string&>();
  template DLLEXPORT
  std::map<uint32_t, typename Factory<FileReader>::UP (*)(std::string&&)>& Factory<FileReader>::Instance<std::string&&>();

  
  FileReader::FileReader(Parameter_ref config) :
    m_config(config)
  {
    
  }

  FileReader::FileReader(const std::string& fileName)  
  {
    m_config.Set(getKeyFileName(), fileName);
  }

  std::string FileReader::Filename() const
  {
    return m_config.Get(getKeyFileName(), "");
  }

  void FileReader::Interrupt()
  {

  }

  std::string FileReader::InputPattern() const
  {
    return m_config.Get(getKeyInputPattern(),"");
  }

  const char* FileReader::getKeyFileName()
  {
    return "InputFileName__";
  }

  const char* FileReader::getKeyInputPattern()
  {
    return "InputPattern__";
  }

  const char* FileReader::getKeySectionName()
  {
    return "FileReaderConfig";
  }

  FileReader::Parameter_ref FileReader::getConfiguration() const
  {
    return m_config;
  }

  FileReader::Parameter_t& FileReader::getConfiguration()
  {
    return m_config;
  }




  fileConfig::fileConfig(const std::string& fileName):m_type(fileName) {

  }


  fileConfig::fileConfig(eudaq::OptionParser & op) {
    if (op.NumArgs() == 1) {
       m_type =  op.GetArg(0);
       return;
    } else {

      std::string combinedFiles = "";
      for (size_t i = 0; i < op.NumArgs(); ++i) {
        if (!combinedFiles.empty()) {
          combinedFiles += ',';
        }
        combinedFiles += op.GetArg(i);


      }
      combinedFiles += "$multi";

      m_type = combinedFiles;
      return;
    }
  }

  std::string fileConfig::get() const {
    return m_type;
  }

  FileReader::Parameter_t FileReader::getConfiguration(const std::string& fileName, const std::string& filePattern)
  {

    auto configuartion = std::string("[") + FileReader::getKeySectionName() + "] \n " + FileReader::getKeyFileName() + "=" + fileName+ "\n " + FileReader::getKeyInputPattern() +"="+ filePattern +"\n";
    return Parameter_t(configuartion);
  }

  std::pair<std::string, std::string> split_name_identifier(const std::string& name){
  
    std::pair<std::string, std::string> ret;
    

    auto index_raute = name.find_last_of('$');
    if (index_raute == std::string::npos)
    {
      // no explicit type definition 
      auto index_dot = name.find_last_of('.');
      if (index_dot != std::string::npos)
      {
        std::string type = name.substr(index_dot + 1);
              

        ret.first = type;
        ret.second = name;
      

      }
      else{
        EUDAQ_THROW("unknown file type ");
      }


    }
    else
    {
      // Explicit definition
      std::string type = name.substr(index_raute + 1);
      std::string name_shorted = name.substr(0, index_raute);
   
      ret.first = type;
      ret.second = name_shorted;
    }

    return ret;
  }


}
