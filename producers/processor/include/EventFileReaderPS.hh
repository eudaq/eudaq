#ifndef EVENTFILEREADERPS_HH_
#define EVENTFILEREADERPS_HH_

#include"Processor.hh"
#include"FileSerializer.hh"


namespace eudaq{
  class EventFileReaderPS:public Processor{
  public:
    EventFileReaderPS(uint32_t psid);

    virtual ~EventFileReaderPS(){};

    virtual void ProcessUserEvent(EVUP ev);
    void OpenFile(std::string filename);
    virtual void ProduceEvent();
    
  private:
    std::unique_ptr<FileDeserializer> m_des;    
  };

}



#endif
