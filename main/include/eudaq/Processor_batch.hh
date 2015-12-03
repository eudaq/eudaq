#ifndef Processor_batch_h__
#define Processor_batch_h__


#include "eudaq/ProcessorBase.hh"
#include "eudaq/Platform.hh"
#include <memory>
#include "Processor_inspector.hh"
namespace eudaq {
using processor_i_up = std::unique_ptr<Processor_Inspector>;
using processor_i_rp = Processor_Inspector*;
class DLLEXPORT Processor_batch :public ProcessorBase {

public:
  virtual ReturnParam ProcessEvent(event_sp ev, ConnectionName_ref con) override;
  Processor_batch();
  virtual ~Processor_batch();
  void init() override;
  void end() override;
  void pushProcessor(Processor_up processor);
  void pushProcessor(Processor_rp processor);
  void wait() override;
  void run();
  void reset();
private:
  std::unique_ptr<std::vector<Processor_up>> m_processors;
  std::vector<Processor_rp> m_processors_rp;
  Processor_rp m_last = nullptr ,m_first =nullptr;
};

using Processor_batch_up = std::unique_ptr<Processor_batch>;

DLLEXPORT Processor_batch& operator>> (Processor_batch& batch, Processor_up proc);
DLLEXPORT Processor_batch& operator>> (Processor_batch& batch, processor_i_up proc);
DLLEXPORT Processor_batch& operator>> (Processor_batch& batch, Processor_rp proc);


DLLEXPORT Processor_batch_up operator>> (Processor_up batch, Processor_up proc);
DLLEXPORT Processor_batch_up operator>> (Processor_up batch, Processor_rp proc);

DLLEXPORT Processor_batch_up operator>> (Processor_batch_up batch, Processor_rp proc);
DLLEXPORT Processor_batch_up operator>> (Processor_batch_up batch, Processor_up proc);
DLLEXPORT Processor_batch_up operator>> (Processor_batch_up batch, processor_i_up proc);

DLLEXPORT std::unique_ptr<Processor_batch> make_batch();
template <typename T>
Processor_batch& operator>> (Processor_batch& batch, T&& lamdbaProcessor) {
  return batch >> make_Processor(std::forward<T>(lamdbaProcessor));
}

template <typename T>
Processor_batch_up operator>> (Processor_up proc1, T&& lamdbaProcessor) {
  auto batch = Processor_batch_up(new Processor_batch());
  batch->pushProcessor(std::move(proc1));
  *batch>>std::forward<T>(lamdbaProcessor);
  return batch;
}
template <typename T>
Processor_batch& operator>> (Processor_batch& batch, T* Processor) {
  return batch >> dynamic_cast<Processor_rp> (Processor);
}


class Processor_i_batch :public Processor_Inspector {
public:
  virtual ReturnParam inspectEvent(const Event& ev, ConnectionName_ref con) override;
  Processor_i_batch();
  void init() override;
  void end() override;
  void wait() override;
  void pushProcessor(processor_i_up processor);
  void pushProcessor(processor_i_rp processor);

  void reset();
private:
  std::unique_ptr<std::vector<processor_i_up>> m_processors;
  std::vector<processor_i_rp> m_processors_rp;
  Processor_rp m_last = nullptr, m_first = nullptr;
};
template<typename... Proc>
inline void push_to_vector(std::vector<Processor_up>& processors, std::vector<Processor_rp>& processors_rp, Proc&&... p) {

}

template<typename P1,typename... Proc>
inline void push_to_vector(std::vector<Processor_up>& processors, std::vector<Processor_rp>& processors_rp, P1 ip,  Proc&&... p) {
  processors_rp.push_back(ip.get());
  processors.push_back(std::move(ip));
  push_to_vector(processors, processors_rp, std::forward<Proc>(p)...);
}
template<typename P1>
inline void push_to_vector(std::vector<Processor_up>& processors, std::vector<Processor_rp>& processors_rp, P1 ip) {
  processors_rp.push_back(ip.get());
  processors.push_back(std::move(ip));
}
template<typename P1,typename... Proc>
inline void push_to_vector(std::vector<Processor_up>& processors, std::vector<Processor_rp>& processors_rp, P1* ip,  Proc&&... p) {
  processors_rp.push_back(ip);
  push_to_vector(processors, processors_rp, std::forward<Proc>(p)...);
}
template<typename P1>
inline void push_to_vector(std::vector<Processor_up>& processors, std::vector<Processor_rp>& processors_rp, P1* ip) {
  processors_rp.push_back(ip);
}
template<typename... Proc>
inline void push_to_vector(std::vector<Processor_up>& processors, std::vector<Processor_rp>& processors_rp, Processor_up ip, Proc&&... p) {
  processors_rp.push_back(ip.get());
  processors.push_back(std::move(ip));
  push_to_vector(processors, processors_rp, std::forward<Proc>(p)...);
}

template<typename... Proc>
inline void push_to_vector(std::vector<Processor_up>& processors, std::vector<Processor_rp>& processors_rp, processor_i_rp ip, Proc&&... p) {
  processors_rp.push_back(ip.get());
  processors.push_back(std::move(ip));
  push_to_vector(processors, processors_rp, std::forward<Proc>(p)...);
}

template<>
inline void push_to_vector(std::vector<Processor_up>& processors, std::vector<Processor_rp>& processors_rp, Processor_up ip) {
  processors_rp.push_back(ip.get());
  processors.push_back(std::move(ip));
}


template<typename... Proc>
inline void push_to_vector(std::vector<Processor_up>& processors, std::vector<Processor_rp>& processors_rp, Processor_rp ip, Proc&&... p) {
  processors_rp.push_back(ip);
  push_to_vector(processors, processors_rp, p...);
}

template<>
inline void push_to_vector(std::vector<Processor_up>& processors, std::vector<Processor_rp>& processors_rp, Processor_rp ip) {
  processors_rp.push_back(ip);
 }


class DLLEXPORT Processor_batch_splitter :public ProcessorBase {
public:
  template<typename... Proc>
  Processor_batch_splitter(Proc&&... p)  {
    m_processors =  std::unique_ptr<std::vector<Processor_up>>(new std::vector<Processor_up>());
    push_to_vector(*m_processors, m_processors_rp, std::forward<Proc>(p)...);
  }

  virtual ReturnParam ProcessEvent(event_sp ev, ConnectionName_ref con) override {
    for (auto& e: m_processors_rp) {
      auto ret = e->ProcessEvent(ev, con);
      if (ret !=  ProcessorBase::sucess){
        return ret;
      }
    }
    return processNext(std::move(ev),con);
  }
  std::unique_ptr<std::vector<Processor_up>> m_processors;
  std::vector<Processor_rp> m_processors_rp;
  void init() override;
  void end() override;
};

template<typename... Proc>
Processor_up splitter(Proc&&... p) {
  return Processor_up(new Processor_batch_splitter(std::forward<Proc>(p)...));
}
}
#endif // Processor_batch_h__