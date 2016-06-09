#ifndef CLASSFACTORY_H__
#define CLASSFACTORY_H__

#include <map>
#include <vector>
#include <memory>
#include <iostream>

#include"Platform.hh"

#define REGISTER_DERIVED_CLASS(BASE,DERIVED,DERIVED_ID) namespace{	\
    static eudaq::RegisterDerived<BASE,DERIVED> reg##DERIVED(DERIVED_ID); \
  }									\
  int DUMMY_VARIABLE_DO_NOT_USE##DERIVED=0 

#define REGISTER_BASE_CLASS(BASE) namespace{				\
    void DUMMY_FUNCTION_DO_NOT_USE##BASE(BASE::MainType mType, BASE::Parameter_ref pType){ \
      eudaq::ClassFactory<BASE>::Create(mType, pType);			\
      eudaq::ClassFactory<BASE>::GetTypes();				\
      eudaq::ClassFactory<BASE>::GetInstance();				\
    }									\
  }									\
  int DUMMY_VARIABLE_DO_NOT_USE##BASE=0

#define CLASS_FACTORY_THROW(msg) std::cout<< "[ClassFactory<BASE>::Create" <<":" << __LINE__<<"]"<<msg<<std::endl;

namespace eudaq{
  
  template <typename BASE>
  class DLLEXPORT ClassFactory{
  public:
    using ID_t = typename BASE::MainType;
    using Parameter_ref = typename BASE::Parameter_ref;
    using factoryfunc = std::unique_ptr<BASE> (*)(Parameter_ref);

    static std::unique_ptr<BASE> Create(const ID_t& name, Parameter_ref params){
      auto it = GetInstance().find(name);
      if (it == GetInstance().end()) {
	CLASS_FACTORY_THROW("unknown class: <" + name + ">");
	return nullptr;
      }
      return (it->second)(params); 
    }
    
    template <typename DERIVED>
      static void Register(const ID_t& name) {
      do_register(name, basefactory<DERIVED>);
    }
    
    static std::map<ID_t, typename ClassFactory<BASE>::factoryfunc>& GetInstance(){  //TODO: const
      static std::map<ID_t, typename ClassFactory<BASE>::factoryfunc> m;
      return m;
    }
    
    static std::vector<ID_t> GetTypes(){ //TODO: const
      std::vector<ID_t> result;
      for (auto& e : GetInstance()) {
	result.push_back(e.first);
      }
      return result;
    }
    
  private:
    template <typename DERIVED>
      static std::unique_ptr<BASE> basefactory(Parameter_ref params) {
      return std::unique_ptr<BASE>(new DERIVED(params));
    }

    static void do_register(const ID_t& name, factoryfunc func){
      GetInstance()[name] = func;
    }
    
  };
  
  template <typename BASE, typename DERIVED>
  class DLLEXPORT RegisterDerived{
  public:
    using ID_t = typename BASE::MainType;
    RegisterDerived(const ID_t& id){
      ClassFactory<BASE>::template Register<DERIVED>(id);
    }
  };
  
}

#endif // CLASSFACTORY_H__
