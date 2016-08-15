#include "eudaq/FileSerializer.hh"
#include "eudaq/DetectorEvent.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/baseFileReader.hh"

#include <string>

#include <memory>

namespace eudaq {


  class DLLEXPORT FileReaderConcatenate : public baseFileReader {
  public:
    FileReaderConcatenate(const std::string & filename, const std::string & filepattern = "") :baseFileReader(filename){}

    FileReaderConcatenate(Parameter_ref param);


    ~FileReaderConcatenate(){}
    virtual  unsigned RunNumber() const override;
    virtual bool NextEvent(size_t skip = 0) override;
    virtual std::shared_ptr<eudaq::Event> getEventPtr() override;
    virtual std::shared_ptr<eudaq::Event> GetNextEvent()override  ;
    virtual void Interrupt() override;
    virtual const eudaq::Event & GetEvent() const override;

  private:
    bool openFile();
    FileReader_up m_reader;

    std::vector<std::string> m_names;
  };

  FileReaderConcatenate::FileReaderConcatenate(Parameter_ref param) :baseFileReader(param)
  {
    

    

      std::string delimiter = ",";


      m_names = split(Filename(), delimiter, true);
      if (m_names.empty())
      {
        EUDAQ_THROW("file name empty" + Filename());
      }

  openFile();

  }

  RegisterFileReader(FileReaderConcatenate, "concat");

  unsigned FileReaderConcatenate::RunNumber() const
  {
    return m_reader->RunNumber();
  }

  bool FileReaderConcatenate::NextEvent(size_t skip /*= 0*/)
  {
    if (!m_reader->NextEvent(skip))
   {
     return openFile();
    
   }
    
    return true;
  
  }

  std::shared_ptr<eudaq::Event> FileReaderConcatenate::getEventPtr()
  {
   return m_reader->getEventPtr();
  }

  std::shared_ptr<eudaq::Event> FileReaderConcatenate::GetNextEvent()
  {
    if (!NextEvent()) {
      return nullptr;
    }

    return m_reader->getEventPtr();
  }

  void FileReaderConcatenate::Interrupt()
  {
    m_reader->Interrupt();
  }

  const eudaq::Event & FileReaderConcatenate::GetEvent() const
  {
    return m_reader->GetEvent();
  }

  bool FileReaderConcatenate::openFile()
  {
    if (m_names.empty())
    {
      return false;
    }

    auto name = m_names.back();
    std::cout << "opening file " << name << std::endl;
    m_reader = std::move(FileReaderFactory::create(name));
    m_names.pop_back();
    return true;
  }

}

