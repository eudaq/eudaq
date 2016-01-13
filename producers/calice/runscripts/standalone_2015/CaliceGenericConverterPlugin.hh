#ifndef CALICEGENERICCONVERTERPLUGIN_HH
#define CALICEGENERICCONVERTERPLUGIN_HH

#include "eudaq/DataConverterPlugin.hh"
#include <iostream>
#include <typeinfo>  // for the std::bad_cast

#define USE_LCIO 1

// All LCIO-specific parts are put in conditional compilation blocks
// so that the other parts may still be used if LCIO is not available.
#if USE_LCIO
#  include "EVENT/LCEvent.h"
#  include "lcio.h"
#  include "IMPL/LCGenericObjectImpl.h"
#endif

namespace calice_eudaq {

#if USE_LCIO
  // LCIO class
  class CaliceLCGenericObject : public lcio::LCGenericObjectImpl {
    public:
      CaliceLCGenericObject() : lcio::LCGenericObjectImpl() {_typeName = "CaliceArray";}
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
  
  // The event type for which this converter plugin will be registered
  // Modify this to match your actual event type (from the Producer)
  static const char* EVENT_TYPE = "CaliceObject";

  // Declare a new class that inherits from DataConverterPlugin
  class CaliceGenericConverterPlugin : public eudaq::DataConverterPlugin {

    public:

      // This is called once at the beginning of each run.
      // You may extract information from the BORE and/or configuration
      // and store it in member variables to use during the decoding later.
      virtual void Initialize(const eudaq::Event & bore, const eudaq::Configuration & cnf);

      // This should return the trigger ID (as provided by the TLU)
      // if it was read out, otherwise it can either return (unsigned)-1,
      // or be left undefined as there is already a default version.

      // not supported
      //virtual unsigned GetTriggerID(const Event & ev) const;

      // Here, the data from the RawDataEvent is extracted into a StandardEvent.
      // The return value indicates whether the conversion was successful.
      // Again, this is just an example, adapted it for the actual data layout.
      
      // not supported
      //virtual bool GetStandardSubEvent(StandardEvent & sev, const Event & ev) const {return false;}

#if USE_LCIO
      // This is where the conversion to LCIO is done
      
      // obsolete
      //virtual lcio::LCEvent * GetLCIOEvent(const Event * /*ev*/) const {return 0;}

      // main converter func
      virtual bool GetLCIOSubEvent(lcio::LCEvent & /*result*/, eudaq::Event const & /*source*/) const;
      void strToMap(const std::string &s, std::map<std::string, std::string> &map)const;
      void mapToStr(const std::map<std::string, std::string> &map, std::string &s)const;
#endif

    private:

      // The constructor can be private, only one static instance is created
      // The DataConverterPlugin constructor must be passed the event type
      // in order to register this converter for the corresponding conversions
      // Member variables should also be initialized to default values here.
      CaliceGenericConverterPlugin();

      // The single instance of this converter plugin
      static CaliceGenericConverterPlugin m_instance;
  }; // class CaliceGenericConverterPlugin

} // namespace eudaq

#endif // CALICEGENERICCONVERTERPLUGIN_HH
