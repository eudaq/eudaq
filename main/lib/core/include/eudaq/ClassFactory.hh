#ifndef CLASSFACTORY_H__
#define CLASSFACTORY_H__

#include <map>
#include <vector>
#include <memory>
#include <iostream>
#include <sstream>
#include <utility>

#include"Platform.hh"

#define REGISTER_DERIVED_CLASS(BASE,DERIVED,DERIVED_ID)			\
  namespace{								\
    static auto DUMMY_VAL_ORI_##DERIVED =				\
      eudaq::ClassFactory<BASE, typename BASE::MainType, typename BASE::Parameter_t>:: \
      Register<DERIVED>(DERIVED_ID);					\
  }

#define REGISTER_BASE_CLASS(BASE) namespace{				\
    void DUMMY_FUNCTION_DO_NOT_USE##BASE(){				\
      eudaq::ClassFactory<BASE, typename BASE::MainType, typename BASE::Parameter_t>:: \
	GetInstance();							\
    }									\
  }

#define DEFINE_FACTORY_AND_PTR(BASE, ...)				\
  using Factory_##BASE = eudaq::ClassFactory<BASE, uint32_t, ##__VA_ARGS__ >; \
  using UP_##BASE= std::unique_ptr<BASE, std::function<void(BASE*)> >;	\
  using SP_##BASE= std::shared_ptr<BASE>;				\
  using WP_##BASE= std::weak_ptr<BASE>

#define DEFINE_FACTORY_AND_PTR_WITH_NAME(NAME, BASE, ...)		\
  using Factory_##NAME = eudaq::ClassFactory<BASE, uint32_t, ##__VA_ARGS__ >; \
  using UP_##NAME= std::unique_ptr<BASE, std::function<void(BASE*)> >;	\
  using SP_##NAME= std::shared_ptr<BASE>;				\
  using WP_##NAME= std::weak_ptr<BASE>

#define INIT_FACTORY(BASE, ...)						\
  namespace{								\
    static auto DUMMY_VAL_##BASE =					\
      eudaq::ClassFactory<BASE, uint32_t, ##__VA_ARGS__ >::GetInstance(); \
  }

#define INIT_CLASS(BASE, DERIVED, ...)					\
  namespace{								\
    static auto DUMMY_VAL_##DERIVED =					\
      eudaq::ClassFactory<BASE, uint32_t, ##__VA_ARGS__ >::		\
      Register<DERIVED>(eudaq::cstr2hash(#DERIVED));			\
  }

#define INIT_CLASS_WITH_ID(BASE, DERIVED, ID, ...)			\
  namespace{								\
    static auto DUMMY_VAL_##DERIVED =					\
      eudaq::ClassFactory<BASE, uint32_t, ##__VA_ARGS__ >::		\
      Register<DERIVED>(ID);						\
  }


#define CLASS_FACTORY_THROW(msg)			\
  std::cout<< "[ClassFactory<BASE>::Create" <<":"	\
  << __LINE__<<"]"<<msg<<std::endl

namespace eudaq{
  template <typename BASE, typename ID_t, typename... ARGS>
  class DLLEXPORT ClassFactory{
  public:
    using UP_BASE = std::unique_ptr<BASE, std::function<void(BASE*)> >;
    using MakerFun = UP_BASE (*)(const ARGS&...);
    
    static UP_BASE Create(const ID_t& id, const ARGS&... args){
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
      static int Register(const ID_t& name) {
      GetInstance()[name] = DerivedFactoryFun<DERIVED>;
      return 0;
    }
    
    static std::map<ID_t, MakerFun>& GetInstance(){
      static std::map<ID_t, MakerFun> m;
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
      static UP_BASE DerivedFactoryFun(const ARGS&... args) {
      return UP_BASE(new DERIVED(args...), [](BASE *p) {delete p; });
    }    
  };
  
}

#endif // CLASSFACTORY_H__
