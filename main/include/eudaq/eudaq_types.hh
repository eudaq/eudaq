#ifndef sct_types_h__
#define sct_types_h__
#include <string>
#include "eudaq/Platform.hh"
#ifdef _DEBUG


#define necessary_CONVERSION(x) x.value
#define Un_necessary_CONVERSION(x) x.value

#define TYPE_CLASS(name,type) \
class DLLEXPORT name { \
public: \
  explicit name(type param_) :value(param_) {}\
  type value; \
}

#else 
#define necessary_CONVERSION(x) x
#define Un_necessary_CONVERSION(x) x

#define TYPE_CLASS(name,type) \
using name = type;
#define TYPE_CLASS_PTR(name,type) \
using name = type;

#endif // _DEBUG
namespace eudaq_types{
  TYPE_CLASS(inPutString, const std::string&);
  TYPE_CLASS(outPutString, std::string&);
  TYPE_CLASS(outPut_Unsigned, unsigned&);
}

#endif // sct_types_h__
