#ifndef Processor_inspector_h__
#define Processor_inspector_h__

#include "eudaq/ProcessorBase.hh"
#include "eudaq/Platform.hh"
#include <type_traits> 
#include <utility>



#define ADD_LAMBDA_PROZESSOR0() ___hidden::lamda_dummy()* [](const eudaq::Event&, ProcessorBase::ConnectionName_ref )
#define ADD_LAMBDA_PROZESSOR1(event_ref)___hidden::lamda_dummy()* [](const eudaq::Event& event_ref, ProcessorBase::ConnectionName_ref )
#define ADD_LAMBDA_PROZESSOR2(event_ref,COnnectionName)  ___hidden::lamda_dummy()* [](const eudaq::Event& event_ref, ProcessorBase::ConnectionName_ref COnnectionName)

namespace eudaq {



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
