#ifndef FACTORY_HH_
#define FACTORY_HH_
#include <map>
#include <memory>
#include <iostream>
#include <utility>
#include <functional>
#include "Platform.hh"

namespace eudaq{
  template <typename BASE>
  class Factory{
  public:
    using UP_BASE = std::unique_ptr<BASE, std::function<void(BASE*)> >;
    
    template <typename ...ARGS>
      static std::unique_ptr<BASE, std::function<void(BASE*)> >
    Create(uint32_t id, ARGS&& ...args);
    
    template <typename... ARGS>
    static std::map<uint32_t, UP_BASE (*)(ARGS&&...)>& GetInstance();
    
    template <typename DERIVED, typename... ARGS>
    static int Register(uint32_t id);
    
  private:
    template <typename DERIVED, typename... ARGS>
      static UP_BASE MakerFun(ARGS&& ...args){
      return UP_BASE(new DERIVED(std::forward<ARGS>(args)...), [](BASE *p) {delete p; });
    }
  };

  template <typename BASE>
  template <typename ...ARGS>
  std::unique_ptr<BASE, std::function<void(BASE*)> >
  Factory<BASE>::Create(uint32_t id, ARGS&& ...args){
    auto it = GetInstance<ARGS&&...>().find(id);
    if (it == GetInstance<ARGS&&...>().end()){
      std::cerr<<"Factory unknown class: <"<<id<<">\n";
      return nullptr;
    }
    return (it->second)(std::forward<ARGS>(args)...);
  };

  template <typename BASE>
  template <typename... ARGS>
  std::map<uint32_t, typename Factory<BASE>::UP_BASE (*)(ARGS&&...)>&
  Factory<BASE>::GetInstance(){
    static std::map<uint32_t, typename Factory<BASE>::UP_BASE (*)(ARGS&&...)> m;
    return m;
  };
    
  template <typename BASE>
  template <typename DERIVED, typename... ARGS>
  int
  Factory<BASE>::Register(uint32_t id){
    std::cout<<"Register ID "<<id; 
    GetInstance<ARGS&&...>()[id] = &MakerFun<DERIVED, ARGS&&...>;
    std::cout<<"   map items: ";
    for(auto& e: GetInstance<ARGS&&...>())
      std::cout<<e.first<<"  ";
    std::cout<<std::endl;
    return 0;
  };

}

#endif
