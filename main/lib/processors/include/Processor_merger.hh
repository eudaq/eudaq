#ifndef Processor_merger_h__
#define Processor_merger_h__

#include "EventSynchronisationBase.hh"
#include "ProcessorBase.hh"

namespace eudaq{

  class SyncBase;
  class Processor_merger :public ProcessorBase{
  public:
    virtual ReturnParam ProcessEvent(event_sp ev,ConnectionName_ref name) override;
    Processor_merger(const SyncBase::MainType& type_, SyncBase::Parameter_ref param_ = SyncBase::Parameter_t());
    virtual ~Processor_merger(){}
    virtual void init() override;
    virtual void end() override;
  private:
    std::map<ConnectionName_t, unsigned> m_map;
    unsigned m_counter = 0;
    Sync_up m_sync;
    const ConnectionName_t  m_output = random_connection();
    SyncBase::MainType m_type;
    SyncBase::Parameter_t m_param;
  };
}



#endif // Processor_merger_h__
