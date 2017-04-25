#ifndef ProcessorBase_h__
#define ProcessorBase_h__



#include "eudaq/Platform.hh"
#include "eudaq/Event.hh"
#include "eudaq/Configuration.hh"

#include <memory>
#include <map>
#include <string>
#include "eudaq/Exception.hh"





namespace eudaq {


  class ProcessorBase;
  class Processor_batch_splitter;
  class inspector_view;
  class Processor_i_batch;
  class Processor_Inspector;

  using Processor_sp = std::shared_ptr < ProcessorBase >;
  using Processor_up = std::unique_ptr < ProcessorBase >;

  using Processor_rp = ProcessorBase*;  //reference pointer 






  class DLLEXPORT ProcessorBase {
  public:
    enum ReturnParam :int {
      sucess,
      ret_error,
      stop,
      busy_retry,
      busy_skip,
      skip_event

    };

    using MainType = std::string;

    using ConnectionName_t = int;
    using ConnectionName_ref = const ConnectionName_t &;

    ProcessorBase();
    //ProcessorBase(Parameter_ref name);
    virtual ~ProcessorBase() {};
    virtual void init() = 0;
    virtual ReturnParam ProcessEvent(event_sp ev, ConnectionName_ref con) = 0;
    virtual void end() = 0;
    virtual void wait() {}

    void AddNextProcessor(Processor_rp next);

  protected:
    ReturnParam processNext(event_sp ev, ConnectionName_ref con);
    Processor_rp m_next = nullptr;
  };

  DLLEXPORT ProcessorBase::ConnectionName_t default_connection();
  DLLEXPORT ProcessorBase::ConnectionName_t random_connection();

  template<class T, class... Args>
  Processor_up make_Processor_up(Args&&... args) {
    auto p = new T(std::forward<Args>(args)...);
    return Processor_up(p);
  }






  class DLLEXPORT processor_view {
  public:
    processor_view(ProcessorBase* base_r);
    processor_view(std::unique_ptr<ProcessorBase> base_u);
    processor_view(std::unique_ptr<Processor_batch_splitter> batch_split);
    processor_view(std::unique_ptr<Processor_i_batch> proc);
    processor_view(std::unique_ptr<Processor_Inspector> proc);
    template <typename T>
    void getValue(T& batch) {
      if (m_proc_rp) {
        batch.pushProcessor(m_proc_rp);
      } else if (m_proc_up) {
        batch.pushProcessor(std::move(m_proc_up));
      }
      else {
        EUDAQ_THROW("unable to extract pointer");
      }
    }
    Processor_up m_proc_up;
    Processor_rp m_proc_rp = nullptr;
  };


}
#endif // ProcessorBase_h__
