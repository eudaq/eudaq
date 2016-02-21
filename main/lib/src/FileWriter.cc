#include "eudaq/FileWriter.hh"
#include "eudaq/FileNamer.hh"
#include "eudaq/Exception.hh"
#include "eudaq/factoryDef.hh"
#include "eudaq/OptionParser.hh"

template class Class_factory_Utilities::Factory<eudaq::FileWriter>;

namespace eudaq {

  
  FileWriter::FileWriter() : m_filepattern(FileNamer::default_pattern) {}

  void FileWriter::WriteBaseEvent(const Event& ev)
  {
    auto dev = dynamic_cast<const eudaq::DetectorEvent *>(&ev);
    if (dev){
      WriteEvent(*dev);
    }
    else
    {
      EUDAQ_THROW("unable to convert event into detector Event. this can happen when you try to use the sync algorithm which does not create detector events.");
    }
  }


  registerBaseClassDef(FileWriter);

  class FileWriterFactory::Impl{
  public:
    std::unique_ptr<eudaq::Option<std::string>> DefaultType,DefaultOutputPattern;

  };

  std::unique_ptr<FileWriter> FileWriterFactory::Create(const std::string & name, const std::string & params /*= ""*/)
  {
    return Class_factory_Utilities::Factory<FileWriter>::Create(name, params);
  }

  std::unique_ptr<FileWriter> FileWriterFactory::Create()
  {
    auto fWriter=Create(getDefaultType());
    fWriter->SetFilePattern(getDefaultOutputPattern());

    return fWriter;
  }

  std::vector<std::string> FileWriterFactory::GetTypes()
  {
    return Class_factory_Utilities::Factory<FileWriter>::GetTypes();
  }



 

  std::string  FileWriterFactory::Help_text()
  {
    return std::string("Available output types are: " + to_string(eudaq::FileWriterFactory::GetTypes(), ", "));

  }

  void FileWriterFactory::addComandLineOptions(OptionParser & op)
  {
    getImpl().DefaultOutputPattern = std::unique_ptr<eudaq::Option<std::string>>(new eudaq::Option<std::string>(op, "o", "outpattern", "test$6R$X", "string", "Output filename pattern"));

    getImpl().DefaultType = std::unique_ptr<eudaq::Option<std::string>>(new eudaq::Option<std::string>(op, "t", "type", getDefaultType(), "name", "Output file type"));

  }

  std::string FileWriterFactory::getDefaultType()
  {
    if (getImpl().DefaultType && getImpl().DefaultType->IsSet())
    {
      return getImpl().DefaultType->Value();
    }
    return "native";
  }

  std::string FileWriterFactory::getDefaultOutputPattern()
  {
    if (getImpl().DefaultOutputPattern && getImpl().DefaultOutputPattern->IsSet())
    {
      return getImpl().DefaultOutputPattern->Value();
    }
    return "test$6R$X";

  }

  FileWriterFactory::Impl& FileWriterFactory::getImpl()
  {
    static FileWriterFactory::Impl m_impl;
    return m_impl;
  }

 
}
