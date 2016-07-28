#ifndef EVENTSENDERPS_HH_
#define EVENTSENDERPS_HH_

#include"Processor.hh"
#include"DataSender.hh"

namespace eudaq{

  class EventSenderPS: public Processor{
  public:
    EventSenderPS(std::string cmd);
    virtual ~EventSenderPS(){};
    virtual void ProcessUserEvent(EVUP ev);

    void Connect(std::string type, std::string name, std::string server);

    virtual void ProcessUsrCmd(const std::string cmd_name, const std::string cmd_par);
    
  private:
    std::unique_ptr<DataSender> m_sender;
  };

}

#endif

