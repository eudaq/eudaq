#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/Exception.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/DEPFETEvent.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"
#include "eudaq/depfetInterpreter.h"
#if USE_LCIO
#  include "IMPL/LCEventImpl.h"
#  include "IMPL/TrackerRawDataImpl.h"
#  include "IMPL/TrackerDataImpl.h"
#  include "IMPL/LCCollectionVec.h"
#  include "UTIL/CellIDEncoder.h"
#  include "lcio.h"
#endif

#if USE_EUTELESCOPE
#  include "EUTELESCOPE.h"
#  include "EUTelRunHeaderImpl.h"
#  include "EUTelDEPFETDetector.h"
#  include "EUTelSetupDescription.h"
#  include "EUTelEventImpl.h"
#  include "EUTelTrackerDataInterfacerImpl.h"
#  include "EUTelGenericSparsePixel.h"
using eutelescope::EUTELESCOPE;
#endif

#include <iostream>
#include <string>
#include <vector>

namespace eudaq {



  /********************************************/

  class DEPFETConverterPlugin : public DataConverterPlugin {
    public:
      //virtual lcio::LCEvent * GetLCIOEvent( eudaq::Event const * ee ) const;

      virtual bool GetStandardSubEvent(StandardEvent &, const eudaq::Event &) const;

      virtual unsigned GetTriggerID(Event const & ev) const {
        const RawDataEvent & rawev = dynamic_cast<const RawDataEvent &>(ev);
        if (rawev.NumBlocks() < 1) return (unsigned)-1;
        return getlittleendian<unsigned>(&rawev.GetBlock(0)[4]);
      }
    private:
      DEPFETConverterPlugin(const std::string name) : DataConverterPlugin(name) {interpreter=DepfetInterpreter();
                                                                interpreter.setSkipRaw(true);
                                                                            }
      DepfetInterpreter interpreter;


      static DEPFETConverterPlugin const m_instance;
      static DEPFETConverterPlugin const m_instance2;
  };

  DEPFETConverterPlugin const DEPFETConverterPlugin::m_instance("DEPFET");
  DEPFETConverterPlugin const DEPFETConverterPlugin::m_instance2("DEPFE5");

  bool DEPFETConverterPlugin::GetStandardSubEvent(StandardEvent & result, const Event & source) const {
    if (source.IsBORE()) {
      //FillInfo(source);
      return true;
    } else if (source.IsEORE()) {
      // nothing to do
      return true;
    }

    // If we get here it must be a data event
    const RawDataEvent & ev = dynamic_cast<const RawDataEvent &>(source);
    auto all_events=std::vector<depfet_event> ();
    std::vector<unsigned> ids;
    for (size_t i = 0; i < ev.NumBlocks(); ++i) {
        auto & data=ev.GetBlock(i);
        interpreter.constInterprete(all_events, &(data[0]),data.size());
        if(all_events.size()==0){
            std::cout<<"No event for block ID"<<ev.GetID(i)<<std::endl;
            continue;
        }
        if(all_events.size()>1){
            std::cout<<"More than one event for block ID. Throwing away the additional ones"<<ev.GetID(i)<<std::endl;
        }
        StandardPlane plane(ev.GetID(i), "DEPFET", "DEPFET");
        plane.SetSizeZS(768,256,all_events[0].zs_data.size());
        plane.SetTLUEvent(all_events[0].triggerNr);
        for(unsigned i=0;i<all_events[0].zs_data.size();i++){
            plane.SetPixel(i,all_events[0].zs_data[i].row,all_events[0].zs_data[i].col,all_events[0].zs_data[i].val);
        }
        result.AddPlane(plane);
     }
    return true;
  }

  } //namespace eudaq
