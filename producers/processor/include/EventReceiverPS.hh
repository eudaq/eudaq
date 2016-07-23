#ifndef EVENTRECEIVERPS_HH_
#define EVENTRECEIVERPS_HH_

#include"Processor.hh"
#include"TransportServer.hh"


namespace eudaq{
  class EventReceiverPS: public Processor{
  public:
    EventReceiverPS(uint32_t psid)
      :Processor("EventReceiverPS", psid){
    }
    EventReceiverPS(std::string cmd)
      :Processor("EventReceiverPS", ""){
      *this<<cmd;
    };

    virtual void ProduceEvent();
    void ProcessUserEvent(EVUP ev);
    
    void SetServer(std::string listenaddress);
    void DataHandler(TransportEvent &ev);
    virtual void ProcessUsrCmd(const std::string cmd_name, const std::string cmd_par);    

  private:
    std::unique_ptr<TransportServer> m_server;
    
  };



}


#endif
