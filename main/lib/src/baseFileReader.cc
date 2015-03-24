#include <list>
#include "eudaq/baseFileReader.hh"
#include "eudaq/factoryDef.hh"
#include "eudaq/Exception.hh"
#include "eudaq/OptionParser.hh"

namespace eudaq {





  baseFileReader::baseFileReader(Parameter_ref fileName) :
    m_fileName(fileName)
  {
    
  }

  baseFileReader::baseFileReader(const std::string& fileName)  
  {
    m_fileName.Set(getKeyFileName(), fileName);
  }

  std::string baseFileReader::Filename() const
  {
    return m_fileName.Get(getKeyFileName(), "");
  }

  void baseFileReader::Interrupt()
  {

  }

  std::string baseFileReader::InputPattern() const
  {
    return m_fileName.Get(getKeyInputPattern(),"");
  }

  const char* baseFileReader::getKeyFileName()
  {
    return "InputFileName__";
  }

  const char* baseFileReader::getKeyInputPattern()
  {
    return "InputPattern__";
  }

  const char* baseFileReader::getKeySectionName()
  {
    return "FileReaderConfig";
  }

  baseFileReader::Parameter_ref baseFileReader::getParameter() const
  {
    return m_fileName;
  }

  baseFileReader::Parameter_t& baseFileReader::getParameter()
  {
    return m_fileName;
  }




  registerBaseClassDef(baseFileReader);



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


  class FileReaderFactory::Impl{

  public:
    std::unique_ptr<eudaq::Option<std::string>> m_default_filePattern;

  };

  std::unique_ptr<baseFileReader> FileReaderFactory::create(const std::string & filename, const std::string & filepattern)
  {
    // return nullptr;
   
    std::string type,inFileName,inFilePattern;


    if (filename.find_first_not_of("0123456789") == std::string::npos) {
      // filename is run number. using default file reader

      auto splitted_pattern = split_name_identifier(filepattern);
      type = splitted_pattern.first;
      auto shorted_file_pattern = splitted_pattern.second;
      inFileName =filename;
      inFilePattern=shorted_file_pattern;
    }
    else
    {
      auto splitted_filename = split_name_identifier(filename);
      type = splitted_filename.first;
      auto shorted_file_name = splitted_filename.second;
      inFileName = splitted_filename.second;
      inFilePattern =filepattern;
    }

    auto params = split(type, "@");
    type = params[0];
    params.erase(params.begin());
    
    std::stringstream ss;
    ss <<"["<< baseFileReader::getKeySectionName() << "] \n ";
    for (auto& e:params)
    {
      ss << e;
      ss << '\n';
    }
    ss << baseFileReader::getKeyFileName() << " = " << inFileName << " \n ";
    ss << baseFileReader::getKeyInputPattern() << " = " << inFilePattern << " \n ";
   
   // std::cout << ss << std::endl;
    baseFileReader::Parameter_t m(ss, baseFileReader::getKeySectionName());
    return EUDAQ_Utilities::Factory<baseFileReader>::Create(type, m);



  }

  std::unique_ptr<baseFileReader> FileReaderFactory::create(const std::string & filename)
  {
    return create(filename, getDefaultInputpattern());
  }

  std::unique_ptr<baseFileReader> FileReaderFactory::create(eudaq::OptionParser & op)
  {

    if (op.NumArgs() == 1)
    {
      return eudaq::FileReaderFactory::create(op.GetArg(0));

    }
    else
    {

      std::string combinedFiles = "";
        for (size_t i = 0; i < op.NumArgs(); ++i)
        {
          combinedFiles += op.GetArg(i);
          combinedFiles += ',';
          
        }
        combinedFiles += "$multi";

        return eudaq::FileReaderFactory::create(combinedFiles);
    }

  }

  std::string FileReaderFactory::Help_text()
  {
    std::string ret = "\n =========== \n";
    ret += "HELP: File Reader: \n";
    ret += "The file reader has two inputs the \"file name\" and the \"file pattern\" \n";
    ret += "option one:\n";
    ret += "\"file name\" = run number\n";
    ret += "In this case the \"file pattern\" gets expanded to a \"file name\" \n";
    ret += "example: \n";
    ret += "\"file pattern\" = ../data/run$6R.raw \n";
    ret += "\"file name\" = 1\n";
    ret += "this expands to: \n";
    ret += "\"file name\" = ../data/run000001.raw\n";
    ret += "option two:\n";
    ret += "\"file name\" = file path\n";
    ret += "in this case the file pattern gets ignored and and only the \"filename\" is taken.\n \n";
    ret += "The file reader gets chosen by the file extension. \n";
    ret += "one can force the use of a specific \"file reader\" by adding #<extension> \n";
    ret += "example:\n";
    ret += "../data/run000001.raw#raw2\n \n";
    ret += "\"file pattern\" = ../data/run$6R.raw#raw2 \n \n";
    ret += "possible \"file readers\" are " + to_string(EUDAQ_Utilities::Factory<baseFileReader>::GetTypes(), ", ")+"\n";


    return ret;
  }

  

  FileReaderFactory::Impl& FileReaderFactory::getImpl()
  {
    static FileReaderFactory::Impl m_impl;
    return m_impl;
  }

  void FileReaderFactory::addComandLineOptions(eudaq::OptionParser & op)
  {

    getImpl().m_default_filePattern = std::unique_ptr<eudaq::Option<std::string>>(new Option<std::string>(op, "i", "inpattern", getDefaultInputpattern(), "string", "Input filename pattern"));

  }

  std::string FileReaderFactory::getDefaultInputpattern()
  {
    if (getImpl().m_default_filePattern && getImpl().m_default_filePattern->IsSet())
    {
      return getImpl().m_default_filePattern->Value();
    }
    return "../data/run$6R.raw";
  }

}
