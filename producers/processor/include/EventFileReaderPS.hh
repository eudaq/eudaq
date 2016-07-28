#ifndef EVENTFILEREADERPS_HH_
#define EVENTFILEREADERPS_HH_

#include"Processor.hh"
#include"FileSerializer.hh"


namespace eudaq{
  class EventFileReaderPS:public Processor{
  public:
    EventFileReaderPS(std::string cmd);
    virtual ~EventFileReaderPS(){};
    virtual void ProcessUserEvent(EVUP ev);
    void OpenFile(std::string filename);
    virtual void ProduceEvent();
    virtual void ProcessUsrCmd(const std::string cmd_name, const std::string cmd_par);    
  private:
    std::unique_ptr<FileDeserializer> m_des;    
  };

}



#endif
