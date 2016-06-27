#ifndef EVENTRECEIVERPS_HH_
#define EVENTRECEIVERPS_HH_

#include"Processor.hh"
#include"TransportServer.hh"


namespace eudaq{
  class EventReceiverPS: public Processor{
  public:
    EventReceiverPS(uint32_t psid)
      :Processor("EventReceiverPS", psid){
      InsertEventType(Event::str2id("_RAW"));
    }
    

    virtual void ProduceEvent();
    void ProcessUserEvent(EVUP ev);
    
    void SetServer(std::string listenaddress);
    void DataHandler(TransportEvent &ev);
    
  private:
    std::unique_ptr<TransportServer> m_server;
    
  };



}


#endif
