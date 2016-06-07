#ifndef Processor_inspector_h__
#define Processor_inspector_h__

#include "ProcessorBase.hh"
#include "eudaq/Platform.hh"
#include <type_traits> 
#include <utility>



#define ADD_LAMBDA_PROZESSOR0() ___hidden::lamda_dummy()* [](const eudaq::Event&, ProcessorBase::ConnectionName_ref )
#define ADD_LAMBDA_PROZESSOR1(event_ref)___hidden::lamda_dummy()* [](const eudaq::Event& event_ref, ProcessorBase::ConnectionName_ref )
#define ADD_LAMBDA_PROZESSOR2(event_ref,COnnectionName)  ___hidden::lamda_dummy()* [](const eudaq::Event& event_ref, ProcessorBase::ConnectionName_ref COnnectionName)

namespace eudaq {

class Processor_batch;
class Processor_i_batch;
class Processor_i_batch; 

  class DLLEXPORT Processor_Inspector :public ProcessorBase {
  public:



    Processor_Inspector();

    void init() override {}
    void end() override {}
    virtual ReturnParam inspectEvent(const Event&, ConnectionName_ref con) = 0;



    ReturnParam ProcessEvent(event_sp ev, ConnectionName_ref con) override;
  };

  template <typename T>
  class  processor_T : public Processor_Inspector {
  public:


    processor_T(T&& proc_) : m_proc(std::forward<T>(proc_)) {}
    

    virtual ReturnParam inspectEvent(const Event& e, ConnectionName_ref con) override {
      m_proc(e, con);
      return ProcessorBase::sucess;
    }
  private:
    T m_proc;

  };



  template <typename T>
  Processor_up make_Processor(T&& proc_) {

    return  Processor_up(new processor_T<T>(std::forward<T>(proc_)));
  }


  class DLLEXPORT inspector_view {
  public:
    inspector_view(Processor_Inspector* proc);
    inspector_view(std::unique_ptr<Processor_Inspector> proc);

    inspector_view(ProcessorBase* proc);

    inspector_view(std::unique_ptr<ProcessorBase> proc);

    inspector_view(std::unique_ptr<Processor_i_batch> proc);
    template<typename T>
    void getValue(T& t) {
      if (m_proc_rp) {
        helper_push_i_r_pointer(t, m_proc_rp);
      } else if (m_proc_up) {
        helper_push_i_u_pointer(t, std::move(m_proc_up));
      } else {
        EUDAQ_THROW("unable to extract pointer");
      }
    }

    Processor_Inspector* m_proc_rp = nullptr;
    std::unique_ptr<Processor_Inspector> m_proc_up;
  };


}

namespace ___hidden {
class lamda_dummy {

};
template<typename T>
eudaq::Processor_up operator*(lamda_dummy&, T&& lamda) {
  return eudaq::make_Processor(std::forward<T>(lamda));

}
}
#endif // Processor_inspector_h__
