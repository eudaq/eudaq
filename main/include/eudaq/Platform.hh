#ifndef EUDAQ_INCLUDED_Platform
#define EUDAQ_INCLUDED_Platform
#include <memory>

#ifndef EUDAQ_PLATFORM
#define EUDAQ_PLATFORM PF_WIN32
#endif

#define PF_LINUX 1
#define PF_MACOSX 2
#define PF_CYGWIN 3
#define PF_WIN32 4

#define EUDAQ_PLATFORM_IS(P) (EUDAQ_PLATFORM == PF_##P)

#ifdef WIN32
#define DLLEXPORT __declspec(dllexport)
#include <stdint.h>

#else

#define DLLEXPORT

#endif
template <class T, class... Args>
std::unique_ptr<T> __make_unique(Args&&... args){
  return std::unique_ptr<T>(new T(args...));
}


template <class derived,class base, class... Args>
std::unique_ptr<base> __make_unique_base(Args&&... args) {
  return std::unique_ptr<base>(dynamic_cast<base>(new derived(args...)));
}
#endif // EUDAQ_INCLUDED_Platform
