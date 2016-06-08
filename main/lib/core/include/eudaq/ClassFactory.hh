#ifndef factory_h__
#define factory_h__

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
    using MainType = typename BASE::MainType;
    using Parameter_t = typename BASE::Parameter_t;
    using Parameter_ref = typename BASE::Parameter_ref;
    typedef std::unique_ptr<BASE> (*factoryfunc)(Parameter_ref);
    using map_t = std::map<MainType, typename ClassFactory<BASE>::factoryfunc> ;

    static std::unique_ptr<BASE> Create(const MainType& name, Parameter_ref params);
    template <typename T>
      static void Register(const MainType & name) {
      do_register(name, basefactory<T>);
    }
    static std::vector<MainType> GetTypes();
  private:
    template <typename T>
      static std::unique_ptr<BASE> basefactory(Parameter_ref params) {
      return std::unique_ptr<BASE>(new T(params));
    }
    static void do_register(const MainType & name, factoryfunc func){
      GetInstance()[name] = func;
    }
  public:
    static map_t& GetInstance(); 
  };

  
  template <typename BASE, typename DERIVED>
  class DLLEXPORT RegisterDerived{
  public:
    using MainType = typename BASE::MainType;
    RegisterDerived(const MainType& id){
      ClassFactory<BASE>::template Register<DERIVED>(id);
    }
  };
  
  template <typename BASE>
  std::vector<typename ClassFactory<BASE>::MainType> ClassFactory<BASE>::GetTypes(){
    std::vector<MainType> result;
    for (auto& e : GetInstance()) {
      result.push_back(e.first);
    }
    return result;
  }
  
  template <typename BASE>
  typename std::unique_ptr<BASE> ClassFactory<BASE>::Create(const MainType& name, Parameter_ref params /*= ""*/){
    auto it = GetInstance().find(name);
    if (it == GetInstance().end()) {
      CLASS_FACTORY_THROW("unknown class: <" + name + ">");
      return nullptr;
    }
    return (it->second)(params); 
  }

  template <typename BASE>
  typename ClassFactory<BASE>::map_t& ClassFactory<BASE>::GetInstance(){
    static map_t m;
    return m;
  }
}

#endif // factory_h__
