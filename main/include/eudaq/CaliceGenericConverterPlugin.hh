#ifndef CALICEGENERICCONVERTERPLUGIN_HH
#define CALICEGENERICCONVERTERPLUGIN_HH

#include "eudaq/DataConverterPlugin.hh"
#include <iostream>
#include <typeinfo>  // for the std::bad_cast

// All LCIO-specific parts are put in conditional compilation blocks
// so that the other parts may still be used if LCIO is not available.
#if USE_LCIO
#  include "EVENT/LCEvent.h"
#  include "lcio.h"
#  include "IMPL/LCGenericObjectImpl.h"
#endif

namespace eudaq {

#if USE_LCIO
  // LCIO class
  class CaliceLCGenericObject : public lcio::LCGenericObjectImpl {
    public:
      CaliceLCGenericObject() : lcio::LCGenericObjectImpl() {_typeName = "CaliceObject";}
      virtual ~CaliceLCGenericObject(){}
      
      void setTags(std::string &s){_dataDescription = s;}
      std::string getTags()const{return _dataDescription;}
      
      void setDataInt(std::vector<int> &vec){
        _intVec.resize(vec.size());
        std::copy(vec.begin(), vec.end(), _intVec.begin());
      }
      
      const std::vector<int> & getDataInt()const{return _intVec;}
  };
#endif

} // namespace eudaq

#endif // CALICEGENERICCONVERTERPLUGIN_HH
