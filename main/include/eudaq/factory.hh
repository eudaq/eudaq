#ifndef factory_h__
#define factory_h__

#include <map>
#include <vector>
#include <memory>

#define registerClass(baseClass,derivedClass,identifier)    namespace {static EUDAQ_Utilities::Registerbase<baseClass,derivedClass> reg##derivedClass(identifier); } int EUDAQ_DUMMY_VARIABLE_DO_NOT_USE##derivedClass=0 


#define RegisterSmartClass(baseClass,derivedClass)  registerClass(baseClass,derivedClass,derivedClass::id())
//#define RegisterBaseClass(baseClass) baseClass::u_pointer create_##baseClass(const baseClass::Parameter_t & par){return EUDAQ_Utilities::Factory<baseClass>::Create(par);}



namespace EUDAQ_Utilities{

  template <typename baseClassType>
  class  Factory {
  public:

    using MainType = typename baseClassType::MainType;
    using MainType_V = std::vector < MainType > ;
    using Parameter_t = typename baseClassType::Parameter_t;
    using Parameter_ref = typename baseClassType::Parameter_ref;
    using u_pointer = std::unique_ptr < baseClassType > ;
    typedef u_pointer (*factoryfunc)(Parameter_ref);
    using map_t = std::map < typename MainType, typename Factory<baseClassType>::factoryfunc > ;



    static u_pointer Create(const MainType & name, Parameter_ref params);
    template <typename T>
    static void Register(const MainType &  name) {
      do_register(name, basefactory<T>);
    }
    static MainType_V GetTypes();
  private:
    template <typename T>
    static u_pointer basefactory(Parameter_ref params) {
      return u_pointer(new T(params));
    }
    static void do_register(const MainType & name, factoryfunc func){
      getInstance()[name] = func;
    }

    static map_t& getInstance(){
      static map_t m;
      return m;
    }

  };


  template <typename baseClass, typename DerivedClass>
  class  Registerbase {
  public:
    using MainType = typename baseClass::MainType;

    Registerbase(const MainType & name) {
      Factory<baseClass>::Register<DerivedClass>(name);
    }
  };

//   template<typename BaseClass>
//   typename BaseClass::u_pointer create(typename BaseClass::MainType& name, 
//                                        typename BaseClass::Parameter_ref param){
// 
//     return Factory<BaseClass>::Create(name, param);
//   }
}
#endif // factory_h__
