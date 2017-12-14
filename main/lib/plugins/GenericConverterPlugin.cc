#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"
#include <iostream>
#include <fstream>
#include <string>
#include "StandardEvent.pb.h"
using namespace std;

// All LCIO-specific parts are put in conditional compilation blocks
// so that the other parts may still be used if LCIO is not available.
#if USE_LCIO
#  include "IMPL/LCEventImpl.h"
#  include "IMPL/TrackerRawDataImpl.h"
#  include "IMPL/TrackerDataImpl.h"
#  include "IMPL/LCCollectionVec.h"
#  include "UTIL/CellIDEncoder.h"
#  include "lcio.h"
#endif

namespace eudaq {

  static const char* EVENT_TYPE = "GENERIC";

  class GenericConverterPlugin : public DataConverterPlugin {

    public:

      virtual bool GetStandardSubEvent(StandardEvent & sev, const Event & ev) const {	    
        if (ev.IsBORE()) {
            return true;
        } else if (ev.IsEORE()) {
            return true;
        }

        const RawDataEvent & ev_raw = dynamic_cast<const RawDataEvent &>(ev);
        if (ev_raw.NumBlocks() != 1)
            return false;
        
        const std::vector<unsigned char> & data = ev_raw.GetBlock(0);
    
        pb::StandardEvent event;
        event.ParseFromArray( data.data(), data.size());
        
        //cout << event.DebugString() << endl;

        for (int i = 0; i < event.plane_size(); i++) {
            const pb::StandardPlane& pb_plane = event.plane(i);

            StandardPlane plane(pb_plane.id(), pb_plane.type(), pb_plane.sensor());
            plane.SetTLUEvent(pb_plane.tluevent());
            
            int flags = 0;
            const pb::Flags& pb_flags = pb_plane.flags();

            if(pb_flags.zs())
                flags |= StandardPlane::FLAG_ZS;
            if(pb_flags.needcds())
                flags |= StandardPlane::FLAG_NEEDCDS;
            if(pb_flags.negative())
                flags |= StandardPlane::FLAG_NEGATIVE;
            if(pb_flags.accumulate())
                flags |= StandardPlane::FLAG_ACCUMULATE;
            if(pb_flags.withpivot())
                flags |= StandardPlane::FLAG_WITHPIVOT;
            if(pb_flags.withsubmat())
                flags |= StandardPlane::FLAG_WITHSUBMAT;
            if(pb_flags.diffcoords())
                flags |= StandardPlane::FLAG_DIFFCOORDS;

            //TODO: Add somethign smarted then this for raw data (need protocol change?)

            if (pb_flags.zs())
                plane.SetSizeZS(pb_plane.xsize(), pb_plane.ysize(), 0, pb_plane.frame_size(), flags);
            else
                plane.SetSizeRaw(pb_plane.xsize(), pb_plane.ysize(), pb_plane.frame_size(), flags);
            
            for (int iframe = 0; iframe < pb_plane.frame_size(); iframe++) {
                const pb::Frame& pb_frame = pb_plane.frame(iframe);
                
                for (int i = 0; i < pb_frame.pixel_size(); i++) {
                    const pb::Pixel& pb_pix = pb_frame.pixel(i);
                    plane.PushPixel(pb_pix.x(), pb_pix.y(), pb_pix.val(), pb_pix.pivot(), iframe);
                }
            }
            
            sev.AddPlane(plane);        
        }

        return true;


      }

#if USE_LCIO
     virtual void GetLCIORunHeader(lcio::LCRunHeader & , eudaq::Event const & , eudaq::Configuration const & ) const; 
     virtual bool GetLCIOSubEvent(lcio::LCEvent & lev, const Event & ev) const; 
#endif

    private:

      GenericConverterPlugin()
        : DataConverterPlugin(EVENT_TYPE)
      {}

      static GenericConverterPlugin m_instance;
  };

  GenericConverterPlugin GenericConverterPlugin::m_instance;

#if USE_LCIO
void GenericConverterPlugin::GetLCIORunHeader(lcio::LCRunHeader & header, eudaq::Event const & , eudaq::Configuration const & ) const {}
 

bool GenericConverterPlugin::GetLCIOSubEvent(lcio::LCEvent & lev, const Event & ev) const {
 
  if (ev.IsBORE()) {
            return true;
  } else if (ev.IsEORE()) {
            return true;
  }
   
  const RawDataEvent & ev_raw = dynamic_cast<const RawDataEvent &>(ev);
  if (ev_raw.NumBlocks() != 1)
    return false;
        
  const std::vector<unsigned char> & data = ev_raw.GetBlock(0);
    
  pb::StandardEvent event;
  event.ParseFromArray( data.data(), data.size());
        
  //cout << event.DebugString() << endl; 

  LCCollectionVec * ZSDataCollection = new LCCollectionVec(lcio::LCIO::TRACKERDATA);
  CellIDEncoder<TrackerDataImpl> ZSDataEncoder( "sensorID:6,sparsePixelType:5", ZSDataCollection);   
  
  for (int i = 0; i < event.plane_size(); i++) {

    const pb::StandardPlane& pb_plane = event.plane(i);

    TrackerDataImpl* zspixels = new TrackerDataImpl; 
    ZSDataEncoder["sensorID"] = pb_plane.id(); 
    ZSDataEncoder["sparsePixelType"] =0; 
    ZSDataEncoder.setCellID(zspixels); 
             
       
    for (int iframe = 0; iframe < pb_plane.frame_size(); iframe++) {
      const pb::Frame& pb_frame = pb_plane.frame(iframe);
                
      for (int i = 0; i < pb_frame.pixel_size(); i++) {
        const pb::Pixel& pb_pix = pb_frame.pixel(i);
        zspixels->chargeValues().push_back(pb_pix.x()); 
        zspixels->chargeValues().push_back(pb_pix.y()); 
        zspixels->chargeValues().push_back(pb_pix.val()); 
      }
    }
            
    ZSDataCollection->push_back(zspixels);
  }
  
  lev.addCollection( ZSDataCollection, "zsdata_generic" );  
 
  return true;

}
#endif



} // namespace eudaq
