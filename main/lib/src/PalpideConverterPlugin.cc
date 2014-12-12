#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"

#if USE_LCIO
#  include "IMPL/LCEventImpl.h"
#  include "IMPL/TrackerRawDataImpl.h"
#  include "IMPL/LCCollectionVec.h"
#  include "lcio.h"
#endif

#if USE_EUTELESCOPE
#  include "EUTELESCOPE.h"
#  include "EUTelRunHeaderImpl.h"
#  include "EUTelPalpideDetector.h"
#  include "EUTelSetupDescription.h"
#  include "EUTelEventImpl.h"
#  include "EUTelSparseDataImpl.h"
#  include "EUTelSimpleSparsePixel.h"
#endif

namespace eudaq {
static const char *EVENT_TYPE="pALPIDE";
class PalpideConverterPlugin:public DataConverterPlugin {
public:
  virtual unsigned GetTriggerID(eudaq::Event const &ev) const {
    const RawDataEvent *rev=dynamic_cast<const RawDataEvent*>(&ev);
    if (!rev || rev->NumBlocks()==0) return -1;
    const std::vector<unsigned char> &data=rev->GetBlock(0);
    int nb=data.size();
    if (nb<2) return -1;
    return data[nb-2]<<8|data[nb-1];
  }
  virtual bool GetStandardSubEvent(StandardEvent &sev,const Event &ev) const {
    const RawDataEvent *rev=dynamic_cast<const RawDataEvent*>(&ev);
    if (!rev || rev->NumBlocks()==0) return false;
    const std::vector<unsigned char> &data=rev->GetBlock(0);
    int nb=data.size();
    int npix=(nb-6)/2;
    StandardPlane plane(0,EVENT_TYPE,"pALPIDE");
    plane.SetSizeZS(64,512,npix);
    for (int ipix=0;ipix<npix;++ipix) {
      unsigned short address=data[2*ipix]<<8|data[2*ipix+1];
      int row=(address&0x1FF);
      int col=address>>9&0x3F;
      plane.SetPixel(ipix,col,row,1);
    }
    plane.SetTLUEvent(GetTriggerID(ev));
    sev.AddPlane(plane);
    return true;
  }
#if USE_LCIO &&  USE_EUTELESCOPE
  bool GetLCIOSubEvent(lcio::LCEvent &le, const eudaq::Event &ev) const {
    if (ev.IsBORE() || ev.IsEORE()) return true;
    const eudaq::RawDataEvent *rev=dynamic_cast<const eudaq::RawDataEvent*>(&ev);
    if (!rev || rev->NumBlocks()==0) return false;

    le.parameters().setValue(eutelescope::EUTELESCOPE::EVENTTYPE,eutelescope::kDE);
    LCCollectionVec *zsDataCollection;
    bool zsDataCollectionExists;
    try {
      zsDataCollection=static_cast<LCCollectionVec*>(le.getCollection("zsdata_pALPIDE"));
      zsDataCollectionExists=true;
    }
    catch (lcio::DataNotAvailableException) {
      zsDataCollection=new LCCollectionVec(lcio::LCIO::TRACKERDATA);
      zsDataCollectionExists=false;
    }
    CellIDEncoder<TrackerDataImpl> zsDataEncoder(eutelescope::EUTELESCOPE::ZSDATADEFAULTENCODING,zsDataCollection);
    TrackerDataImpl *zsFrame=new TrackerDataImpl();
    zsDataEncoder["sensorID"       ]=6;
    zsDataEncoder["sparsePixelType"]=eutelescope::kEUTelSimpleSparsePixel;
    zsDataEncoder.setCellID(zsFrame);
    const std::vector<unsigned char> &data=rev->GetBlock(0);
    int nb=data.size();
    int npix=(nb-6)/2;
    for (int ipix=0;ipix<npix;++ipix) {
      unsigned short address=data[2*ipix]<<8|data[2*ipix+1];
      int row=(address&0x1FF);
      int col=address>>9&0x3F;
      zsFrame->chargeValues().push_back(col);
      zsFrame->chargeValues().push_back(row);
      zsFrame->chargeValues().push_back(1);
    }
    zsDataCollection->push_back(zsFrame);
    if (zsDataCollection->size()!=0)
      le.addCollection(zsDataCollection,"zsdata_pALPIDE");
    else if (!zsDataCollectionExists)
      delete zsDataCollection;
    le.parameters().setValue("TRGTIME",atoi(ev.GetTag("TRGTIME").c_str()));
    le.parameters().setValue("VALTIME",atoi(ev.GetTag("VALTIME").c_str()));
    if (le.getEventNumber()==0) {
      LCCollectionVec *pALPIDESetupCollection;
      bool pALPIDESetupExists;
      try {
        pALPIDESetupCollection=static_cast<LCCollectionVec*>(le.getCollection("pALPIDESetup")) ;
        pALPIDESetupExists    =true;
      } catch (lcio::DataNotAvailableException) {
        pALPIDESetupCollection=new LCCollectionVec(lcio::LCIO::LCGENERICOBJECT);
        pALPIDESetupExists    =false;
      }
      pALPIDESetupCollection->push_back(new eutelescope::EUTelSetupDescription(new eutelescope::EUTelPalpideDetector()));
      if (!pALPIDESetupExists) {
        le.addCollection(pALPIDESetupCollection,"pALPIDESetup");
      }
    }
    return true;
  }
#endif
private:
  PalpideConverterPlugin()
   :DataConverterPlugin(EVENT_TYPE)
  {}
  static PalpideConverterPlugin m_instance;
};

PalpideConverterPlugin PalpideConverterPlugin::m_instance;

} // namespace eudaq

