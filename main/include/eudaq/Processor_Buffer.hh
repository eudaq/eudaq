#ifndef Processor_Buffer_h__
#define Processor_Buffer_h__
#include "ProcessorBase.hh"
#include <deque>



namespace eudaq {


class Proccessor_Buffer :public ProcessorBase {
public:
  Proccessor_Buffer();
  ~Proccessor_Buffer() {}
  virtual ReturnParam ProcessEvent(event_sp ev, ConnectionName_ref con) override;
  virtual void init() override;
  virtual void end() override {}


private:
  std::deque<event_sp> m_queue; 
  size_t m_bufferSize = 100;        //Magic number for now 

};

}


#endif // Processor_Buffer_h__
