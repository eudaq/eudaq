#ifndef FACTORY_HH_
#define FACTORY_HH_
#include <map>
#include <memory>
#include <iostream>
#include <utility>
#include <functional>
#include <cstdint>

namespace eudaq{

  template <typename BASE>
  class Factory{
  public:
    using UP_BASE = std::unique_ptr<BASE, std::function<void(BASE*)> >;
    using SP_BASE = std::shared_ptr<BASE>;
    using WP_BASE = std::weak_ptr<BASE>;
    using SPC_BASE = std::shared_ptr<const BASE>;

    using UP = std::unique_ptr<BASE, std::function<void(BASE*)> >;
    using SP = std::shared_ptr<BASE>;
    using WP = std::weak_ptr<BASE>;
    using SPC = std::shared_ptr<const BASE>;
    
    template <typename ...ARGS>
    static typename Factory<BASE>::UP_BASE
    MakeUnique(std::uint32_t id, ARGS&& ...args);

    template <typename ...ARGS>
    static typename Factory<BASE>::SP_BASE
    MakeShared(std::uint32_t id, ARGS&& ...args);

    template <typename ...ARGS>
    static typename Factory<BASE>::UP_BASE
    Create(std::uint32_t id, ARGS&& ...args);

    template <typename... ARGS>
      static std::map<std::uint32_t, UP_BASE (*)(ARGS&&...)>&
    Instance();
    
    template <typename DERIVED, typename... ARGS>
    static std::uint64_t
    Register(std::uint32_t id);
    
  private:
    template <typename DERIVED, typename... ARGS>
      static UP_BASE MakerFun(ARGS&& ...args){
      return UP_BASE(new DERIVED(std::forward<ARGS>(args)...), [](BASE *p) {delete p; });
    }
  };

  template <typename BASE>
  template <typename ...ARGS>
  typename Factory<BASE>::UP_BASE
  Factory<BASE>::MakeUnique(std::uint32_t id, ARGS&& ...args){
    auto &ins = Instance<ARGS&&...>();
    auto it = ins.find(id);
    if (it == ins.end()){
      std::cerr<<"Factory<"<<static_cast<const void *>(&ins)<<">: "
	       <<" Unknown class ID: <"<<id<<">\n";
      return nullptr;
    }
    return (it->second)(std::forward<ARGS>(args)...);
  }

  template <typename BASE>
  template <typename ...ARGS>
  typename Factory<BASE>::SP_BASE
  Factory<BASE>::MakeShared(std::uint32_t id, ARGS&& ...args){
    SP_BASE sp = MakeUnique(id, std::forward<ARGS>(args)...);
    return sp;
  }

  
  template <typename BASE>
  template <typename ...ARGS>
  typename Factory<BASE>::UP_BASE
  Factory<BASE>::Create(std::uint32_t id, ARGS&& ...args){
    return MakeUnique(id, std::forward<ARGS>(args)...);
  }
  
  template <typename BASE>
  template <typename... ARGS>
  std::map<std::uint32_t, typename Factory<BASE>::UP_BASE (*)(ARGS&&...)>&
  Factory<BASE>::Instance(){
    static std::map<std::uint32_t, typename Factory<BASE>::UP_BASE (*)(ARGS&&...)> m;
    static bool init = true;
    if(init){
      // std::cout<<"Instance a new Factory<"<<static_cast<const void *>(&m)<<">"<<std::endl;
      init=false;
    }
    return m;
  }
    
  template <typename BASE>
  template <typename DERIVED, typename... ARGS>
  std::uint64_t
  Factory<BASE>::Register(std::uint32_t id){
    auto &ins = Instance<ARGS&&...>();
    // std::cout<<"Register ID "<<id <<"  to Factory<"
    // 	     <<static_cast<const void *>(&ins)<<">    ";
    ins[id] = &MakerFun<DERIVED, ARGS&&...>;
    // std::cout<<"   map items: ";
    // for(auto& e: ins)
    //   std::cout<<e.first<<"  ";
    // std::cout<<std::endl;
    return reinterpret_cast<std::uintptr_t>(&ins);
  }

}

#endif
