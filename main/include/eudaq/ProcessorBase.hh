#ifndef ProcessorBase_h__
#define ProcessorBase_h__



#include "eudaq/Platform.hh"
#include "eudaq/Event.hh"
#include "eudaq/Configuration.hh"

#include <memory>
#include <map>
#include <string>





namespace eudaq {


class ProcessorBase;


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
private:
  Processor_rp m_next = nullptr;
};

DLLEXPORT ProcessorBase::ConnectionName_t default_connection();
DLLEXPORT ProcessorBase::ConnectionName_t random_connection();

template<class T, class... Args>
Processor_up make_Processor_up(Args&&... args) {
  auto p = new T(std::forward<Args>(args)...);
  return Processor_up(p);
}

}
#endif // ProcessorBase_h__
