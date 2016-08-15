#include "eudaq/FileSerializer.hh"
#include "eudaq/DetectorEvent.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/baseFileReader.hh"

#include <string>

#include <memory>
#include <iosfwd>

namespace eudaq {


  class DLLEXPORT FileReaderFileList : public baseFileReader {
  public:
   // FileReaderFileList(const std::string & filename, const std::string & filepattern = "") :baseFileReader(filename){}

    FileReaderFileList(Parameter_ref param);


    ~FileReaderFileList(){}
    virtual  unsigned RunNumber() const override;
    virtual bool NextEvent(size_t skip = 0) override;
    virtual std::shared_ptr<eudaq::Event> getEventPtr() override;
    virtual std::shared_ptr<eudaq::Event> GetNextEvent()override  ;
    virtual void Interrupt() override;
    virtual const eudaq::Event & GetEvent() const override;

  private:

    FileReader_up m_reader;


  };

  FileReaderFileList::FileReaderFileList(Parameter_ref param) :baseFileReader(param)
  {
    

    

      
      
    std::ifstream inFile(Filename());
    std::cout << Filename() << std::endl;

    std::string path = Filename();
    auto i=path.find_last_of("\\/");
    if (i!=std::string::npos)
    {
      path = path.substr(0, i+1);
    }
    else{
      path = "";
    }
    
      std::string  file;
      std::string line;


      while (getline(inFile, line, '\n'))
      {
        line =eudaq::trim(line);
       
        if (!file.empty())
        {
          file += ',';
        }

        file += path+line;
      }
      file += "$concat";

      m_reader = eudaq::FileReaderFactory::create(file);

  }

  RegisterFileReader(FileReaderFileList, "list");

  unsigned FileReaderFileList::RunNumber() const
  {
    return m_reader->RunNumber();
  }

  bool FileReaderFileList::NextEvent(size_t skip /*= 0*/)
  {
    return m_reader->NextEvent(skip);
  }

  std::shared_ptr<eudaq::Event> FileReaderFileList::getEventPtr()
  {
   return m_reader->getEventPtr();
  }

  std::shared_ptr<eudaq::Event> FileReaderFileList::GetNextEvent()
  {
    if (!NextEvent()) {
      return nullptr;
    }

    return m_reader->getEventPtr();
  }

  void FileReaderFileList::Interrupt()
  {
    m_reader->Interrupt();
  }

  const eudaq::Event & FileReaderFileList::GetEvent() const
  {
    return m_reader->GetEvent();
  }


}

