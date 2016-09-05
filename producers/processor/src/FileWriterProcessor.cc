#include"Processor.hh"
#include"ProcessorManager.hh"
#include"FileWriter.hh"

namespace eudaq{
  class FileWriterProcessor;

  namespace{
    auto dummy0 = Factory<FileWriter>::Register<FileWriterProcessor, std::string&>(cstr2hash("processor"));
    auto dummy1 = Factory<FileWriter>::Register<FileWriterProcessor, std::string&&>(cstr2hash("processor"));
  }
  
  class FileWriterProcessor: public FileWriter{
  public:
    FileWriterProcessor(const std::string &patt):m_pattern(patt){};
    virtual void StartRun(unsigned){};
    virtual void WriteEvent(const DetectorEvent &ev);
    virtual uint64_t FileBytes() const{return 0;};

  private:
    PSSP m_ps;
    std::string m_pattern;
  };
  
  void FileWriterProcessor::WriteEvent(const DetectorEvent &ev){
    if(!m_ps){
      std::cout<<"no processor inside, create EventSenderPS connecting to "<< m_pattern <<std::endl;
      m_ps=Factory<Processor>::Create<std::string&>(eudaq::cstr2hash("EventSenderPS"), m_pattern);
      ProcessorManager& psMan = *ProcessorManager::GetInstance();
      psMan>>m_ps;
    }
    
    *m_ps<<ev.Clone(); //TODO
  }

}
