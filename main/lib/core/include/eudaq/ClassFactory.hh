#ifndef CLASSFACTORY_H__
#define CLASSFACTORY_H__

#include <map>
#include <vector>
#include <memory>
#include <iostream>
#include <sstream>
#include <utility>

#include"Platform.hh"

#define REGISTER_DERIVED_CLASS(BASE,DERIVED,DERIVED_ID) namespace{	\
    static eudaq::RegisterDerived<BASE, typename BASE::MainType, DERIVED, typename BASE::Parameter_t> reg##DERIVED(DERIVED_ID); \
  }									\
  int DUMMY_VARIABLE_DO_NOT_USE##DERIVED=0 

#define REGISTER_BASE_CLASS(BASE) namespace{				\
    void DUMMY_FUNCTION_DO_NOT_USE##BASE(BASE::MainType mType, BASE::Parameter_t pType){ \
      eudaq::ClassFactory<BASE, typename BASE::MainType, typename BASE::Parameter_t>::Create(mType, pType); \
      eudaq::ClassFactory<BASE, typename BASE::MainType, typename BASE::Parameter_t>::GetTypes(); \
      eudaq::ClassFactory<BASE, typename BASE::MainType, typename BASE::Parameter_t>::GetInstance(); \
    }									\
  }									\
  int DUMMY_VARIABLE_DO_NOT_USE##BASE=0

#define CLASS_FACTORY_THROW(msg) std::cout<< "[ClassFactory<BASE>::Create" <<":" << __LINE__<<"]"<<msg<<std::endl;

namespace eudaq{
  template <typename BASE, typename ID_t, typename... ARGS>
  class DLLEXPORT ClassFactory{
  public:
    using factoryfunc = std::unique_ptr<BASE> (*)(const ARGS&...);
    
    static std::unique_ptr<BASE> Create(const ID_t& id, const ARGS&... args){
      auto it = GetInstance().find(id);
      if (it == GetInstance().end()) {
	std::stringstream ss;
	ss<<"unknown class: <"<<id<<">";
	CLASS_FACTORY_THROW(ss.str());
	return nullptr;
      }
      return (it->second)(args...);
    }
    
    template <typename DERIVED>
      static void Register(const ID_t& name) {
      do_register(name, DerivedFactoryFun<DERIVED>);
    }
    
    static std::map<ID_t,factoryfunc>& GetInstance(){
      static std::map<ID_t,factoryfunc> m;
      return m;
    }
    
    static std::vector<ID_t> GetTypes(){
      std::vector<ID_t> result;
      for (auto& e : GetInstance()) {
	result.push_back(e.first);
      }
      return result;
    }
    
  private:
    template <typename DERIVED>
      static std::unique_ptr<BASE> DerivedFactoryFun(const ARGS&... args) {
      return std::unique_ptr<BASE>(new DERIVED(args...));
    }

    static void do_register(const ID_t& name, factoryfunc func){
      GetInstance()[name] = func;
    }
    
  };


  
  template <typename BASE, typename ID_t, typename DERIVED, typename... ARGS>
  class DLLEXPORT RegisterDerived{
  public:
    RegisterDerived(const ID_t& id){
      ClassFactory<BASE, ID_t, ARGS...>::template Register<DERIVED>(id);
    }
  };
  
}

#endif // CLASSFACTORY_H__
