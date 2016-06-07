#ifndef Processor_add2queue_h__
#define Processor_add2queue_h__




#include "ProcessorBase.hh"



namespace eudaq{



  class DLLEXPORT Processor_add2queue :public ProcessorBase{
  public:


    Processor_add2queue(ConnectionName_ref con_);

    void init() override;
    virtual void initialize() = 0;
    



    ReturnParam ProcessEvent(event_sp ev, ConnectionName_ref con) override;
  
    enum status
    {
      configuring,
      running,
      stopping

    }m_status;

  private:

    virtual ReturnParam add2queue(event_sp& ev) = 0;
    void handelReturn(ReturnParam ret);
    ConnectionName_t m_con;
  };


}
#endif // Processor_add2queue_h__
