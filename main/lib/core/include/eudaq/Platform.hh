#ifndef EUDAQ_INCLUDED_Platform
#define EUDAQ_INCLUDED_Platform
#include <memory>

template <typename T, typename... Args> 
T __check(Args&&... args); 

#define PF_LINUX 1
#define PF_MACOSX 2
#define PF_CYGWIN 3
#define PF_WIN32 4

#define EUDAQ_PLATFORM_IS(P) (EUDAQ_PLATFORM == PF_##P)

#if EUDAQ_PLATFORM_IS(WIN32)

#ifdef EUDAQ_CORE_EXPORTS
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT __declspec(dllimport)
#endif

#else

#define DLLEXPORT
#endif




#if EUDAQ_PLATFORM_IS(WIN32)
#include <stdint.h>
#endif



// used for reference counted TObject 
// Use this only if you know the object is counted somewehere else
#define T_NEW new
template <class T, class... Args>
std::unique_ptr<T> __make_unique(Args&&... args){
  return std::unique_ptr<T>(new T(args...));
}


template <class derived,class base, class... Args>
std::unique_ptr<base> __make_unique_base(Args&&... args) {
  return std::unique_ptr<base>(dynamic_cast<base>(new derived(args...)));
}
#endif // EUDAQ_INCLUDED_Platform
