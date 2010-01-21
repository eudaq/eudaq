#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/Exception.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"

#if USE_LCIO
#  include "IMPL/LCEventImpl.h"
#  include "IMPL/TrackerRawDataImpl.h"
#  include "IMPL/TrackerDataImpl.h"
#  include "IMPL/LCCollectionVec.h"
#  include "lcio.h"
#  include "UTIL/CellIDEncoder.h"
#endif

#if USE_EUTELESCOPE
#  include "EUTELESCOPE.h"
#  include "EUTelRunHeaderImpl.h"
//#  include "EUTelAPIXMCDetector.h"
#  include "EUTelSetupDescription.h"
#  include "EUTelEventImpl.h"
#  include "EUTelSparseDataImpl.h"
//#  include "EUTelAPIXSparsePixel.h"
using eutelescope::EUTELESCOPE;
#endif

#include <iostream>
#include <string>
#include <vector>

static const bool UGLY_HACK = true;

namespace eudaq {

  /********************************************/

  struct APIXPlanes {
    std::vector<StandardPlane> planes;
    std::vector<int> feids;
    int Find(int id) {
      std::vector<int>::const_iterator it = std::find(feids.begin(), feids.end(), id);
      if (it == feids.end()) return -1;
      else return it - feids.begin();
    }
  };
  struct APIXHit {
    unsigned int chip;
    unsigned int tot;
    unsigned int lv1;
    unsigned int x;
    unsigned int y;
    unsigned int pllTrigger;
    unsigned int eudetTrigger;
    unsigned int time1;
    unsigned int time2;
  };

  class APIXMCConverterPlugin : public DataConverterPlugin {
  public:
    virtual void Initialize(const Event & e, const Configuration & c);
  
   //virtual lcio::LCEvent * GetLCIOEvent( eudaq::Event const * ee ) const;
// #if USE_LCIO && USE_EUTELESCOPE
//    void ConvertLCIOHeader(lcio::LCRunHeader & header, eudaq::Event const & bore, eudaq::Configuration const & conf) const;
//    virtual void GetLCIORunHeader(lcio::LCRunHeader & header, eudaq::Event const & bore, eudaq::Configuration const & conf) const {
//      return ConvertLCIOHeader(header, bore, conf);
//    }
//    bool GetLCIOSubEvent(lcio::LCEvent & result, const Event & source) const;
// #endif
    virtual unsigned GetTriggerID(eudaq::Event const &) const;
    virtual bool GetStandardSubEvent(StandardEvent &, const eudaq::Event &) const;

  private:
// #if USE_LCIO && USE_EUTELESCOPE
//    void addChipToLCIOCollection(std::vector<eutelescope::EUTelAPIXSparsePixel*> & hits, 
// 				lcio::LCCollectionVec* zsDataCollection,
// 				CellIDEncoder< TrackerDataImpl    >  &zsDataEncoder,
// 				std::vector< eutelescope::EUTelSetupDescription * >  &setupDescription,
// 				lcio::LCEvent  & result, int feid) const;
//    std::vector<eutelescope::EUTelAPIXSparsePixel*> ConvertHits(const std::vector<unsigned char> & data) const;
//#endif
    unsigned int getEudetTrig(unsigned int word) const;
    unsigned int getPLLTrig(unsigned int word) const;
    std::vector<APIXHit> decodeData(const std::vector<unsigned char> & data) const;
    void ConvertPlanes(const std::vector<unsigned char> & data, APIXPlanes & result) const;
    APIXMCConverterPlugin() : DataConverterPlugin("APIX-MC"),
                              m_PlaneMask(0) {}
    unsigned m_PlaneMask;
   
    static APIXMCConverterPlugin const m_instance;
  };

// #if USE_LCIO && USE_EUTELESCOPE
//   void APIXMCConverterPlugin::ConvertLCIOHeader(lcio::LCRunHeader & header, eudaq::Event const & bore, eudaq::Configuration const & conf) const
// {
//     eutelescope::EUTelRunHeaderImpl runHeader(&header);
//   }


//   bool APIXMCConverterPlugin::GetLCIOSubEvent(lcio::LCEvent & result, const Event & source) const
//   {
//     if (source.IsBORE()) { //shouldn't happen
//       return true;
//     } else if (source.IsEORE()) { // nothing to do
//       return true;
//     }
//     // If we get here it must be a data event

//     result.parameters().setValue( eutelescope::EUTELESCOPE::EVENTTYPE, eutelescope::kDE );

//     /* Extract the Sparse pixel hits from the event 
//      * The module data is all saved in one block, so the ev.GetBlock(0) is intentional
//      */
//     const RawDataEvent & ev = dynamic_cast<const RawDataEvent &>(source);
//     std::vector<eutelescope::EUTelAPIXSparsePixel*> apixHits = ConvertHits(ev.GetBlock(0)); 


//     // prepare the collections for the zero supressed events 
//     // set the proper cell encoder, Xero supressed data is stored in SparsePixels, witch are encoded in a LCIO TrackerDataImpl event
//     // See EUTelSparseDataImpl for ways of adding variables
//     lcio::LCCollectionVec* zsDataCollection;
//     try {
//       zsDataCollection = static_cast< LCCollectionVec* > ( result.getCollection( "zsdata" ) );
//     } catch ( lcio::DataNotAvailableException& e ) {
//       zsDataCollection = new LCCollectionVec( lcio::LCIO::TRACKERDATA );
//     }
//     CellIDEncoder< TrackerDataImpl > zsDataEncoder ( eutelescope::EUTELESCOPE::ZSDATADEFAULTENCODING, zsDataCollection );
//     std::vector< eutelescope::EUTelSetupDescription * >  setupDescription;

//     int feid = 0;
//     unsigned mask = m_PlaneMask;
//     while (mask) {
//       if (mask & 1) {
// 	addChipToLCIOCollection(apixHits, zsDataCollection, zsDataEncoder, setupDescription, result, feid);
//       }
//       feid++;
//       mask >>= 1;
//     }

//        if ( apixHits.size() > 0 ) {
// 	 std::cout << "APIX EVENT AT RUN!" << std::endl;
//        }

//        //result.addCollection( zsDataCollection,  "zsdata_apix" );

//     //Delete all the hits
//     for(unsigned int ii = 0; ii < apixHits.size(); ii++){
//       delete apixHits.at(ii);
//     }

//     if ( result.getEventNumber() == 0 ) {
//       // do this only in the first event
//       LCCollectionVec * apixSetupCollection = NULL;
//       bool apixSetupExists = false;
//       try {
// 	apixSetupCollection = static_cast< LCCollectionVec* > ( result.getCollection( "apix_setup" ) ) ;
// 	apixSetupExists = true;
//       } catch (...) {
// 	apixSetupCollection = new LCCollectionVec( lcio::LCIO::LCGENERICOBJECT );
//       }
      
//       for ( size_t iPlane = 0 ; iPlane < setupDescription.size() ; ++iPlane ) {
// 	apixSetupCollection->push_back( setupDescription.at( iPlane ) );
//       }
      
//       if (!apixSetupExists) {
// 	//result.addCollection( apixSetupCollection, "apix_setup" );
//       }
//     }

//     return true; //whatever... I guess this just breaks if event is not raw
//   }

//   /*!
//    * Prepare a collection for a given feid
//    * The feids are stored as separate collections in the LCIO file
//    * I think this is a good idea, as they will be act as independent detectors in the tracking
//    */
//   void APIXMCConverterPlugin::addChipToLCIOCollection(std::vector<eutelescope::EUTelAPIXSparsePixel*> & apixHits, 
// 					              lcio::LCCollectionVec* zsDataCollection,
// 						      CellIDEncoder< TrackerDataImpl    >  &zsDataEncoder,
// 						      std::vector< eutelescope::EUTelSetupDescription * >  &setupDescription,
// 						      lcio::LCEvent & result, int feid) const{
    
//     // describe the setup
//     // The current detector is ...
//     eutelescope::EUTelPixelDetector * currentDetector = new eutelescope::EUTelAPIXMCDetector;
//     currentDetector->setMode( "ZS" );
    
//     if ( result.getEventNumber() == 0 ) {
//       setupDescription.push_back( new eutelescope::EUTelSetupDescription( currentDetector )) ;
//     }

//     zsDataEncoder["sparsePixelType"] = eutelescope::kEUTelAPIXSparsePixel;
//     zsDataEncoder["sensorID"] = feid + 10;//Add 10 to separate from telescope IDs, do not know if it matters
//     std::auto_ptr <lcio::TrackerDataImpl > zsFrame( new lcio::TrackerDataImpl);
//     zsDataEncoder.setCellID(zsFrame.get());
    
//     bool anyHits = false;

//     std::auto_ptr< eutelescope::EUTelSparseDataImpl< eutelescope::EUTelAPIXSparsePixel > >
//       sparseFrame( new eutelescope::EUTelSparseDataImpl< eutelescope::EUTelAPIXSparsePixel > ( zsFrame.get() ) );


//     for(unsigned int ihit=0; ihit < apixHits.size(); ihit++ ){
//       eutelescope::EUTelAPIXSparsePixel* thisHit = apixHits.at(ihit);
//       if ((short)thisHit->getChip() == (short)feid){
//      sparseFrame->addSparsePixel(thisHit);
//      anyHits = true;
//       }
//     }

//     zsDataCollection->push_back( zsFrame.release() );
//     delete currentDetector;
//   }

//   /* To hell with subtriggers and planes :) 
//    * Lets read the APIX hits out as APIX hits
//    * Returns true if any hits are found.
//    */
//   std::vector<eutelescope::EUTelAPIXSparsePixel*> APIXMCConverterPlugin::ConvertHits(const std::vector<unsigned char> & data) const {
//     std::vector<eutelescope::EUTelAPIXSparsePixel*> sparseHits;
//     std::vector<APIXHit> hits = decodeData( data );
//     for( unsigned int i =0; i < hits.size(); i++){
//       APIXHit hit = hits.at(i);
//       eutelescope::EUTelAPIXSparsePixel* sparseHit = new eutelescope::EUTelAPIXSparsePixel(hit.x,hit.y,hit.tot,hit.chip,hit.lv1);
//       sparseHits.push_back(sparseHit);
//     }
//     return sparseHits;
//   }
// #endif

  APIXMCConverterPlugin const APIXMCConverterPlugin::m_instance;

  void APIXMCConverterPlugin::Initialize(const Event & source, const Configuration &) {
    m_PlaneMask = from_string(source.GetTag("PlaneMask"), 0);
    //std::cout << " Nrows , NColumns = " << m_NumRows << "  ,  " <<  m_NumColumns << std::endl;
    //std::cout << " Initial row , column = " << m_InitialRow << "  ,  " <<  m_InitialColumn << std::endl;
  }

  unsigned APIXMCConverterPlugin::GetTriggerID(eudaq::Event const & ev) const {
    const RawDataEvent & rev = dynamic_cast<const RawDataEvent &>(ev);
    if (rev.NumBlocks() < 1) return (unsigned)-1;
    unsigned tid = getlittleendian<unsigned int>(&rev.GetBlock(0)[4]);
    // Events that should have tid=0 have rubbish instead
    // so if number is illegal, convert it to zero.
    if (tid >= 0x8000) return 0;
    return tid;
  }

  bool APIXMCConverterPlugin::GetStandardSubEvent(StandardEvent & result, const Event & source) const {
    if (source.IsBORE()) {
      // shouldn't happen
      return true;
    } else if (source.IsEORE()) {
      // nothing to do
      return true;
    }
    // If we get here it must be a data event
    const RawDataEvent & ev = dynamic_cast<const RawDataEvent &>(source);

    /* Yes this is bit strange...
       in MutiChip-Mode which is used here, all the Data are written in only one block,
       in principal its possible to do it different, but then a lot of data would be doubled.
       The Data in block 1 is exactly what is in the eudet-fifo and the datafifo for on event.
    */
    APIXPlanes planes;
    int feid = 0;
    unsigned mask = m_PlaneMask;
    while (mask) {
      if (mask & 1) {
        //std::cout << "APIX-MC plane " << feid << std::endl;
        planes.planes.push_back(StandardPlane(feid, "APIX", "APIX"));
        planes.feids.push_back(feid);
        if (UGLY_HACK && ( feid == 3 || feid == 4 || feid == 5 || feid == 6 )) {
          planes.planes.back().SetSizeZS(160, 18, 0, 16, StandardPlane::FLAG_DIFFCOORDS | StandardPlane::FLAG_ACCUMULATE);
        } else {
          planes.planes.back().SetSizeZS(18, 160, 0, 16, StandardPlane::FLAG_DIFFCOORDS | StandardPlane::FLAG_ACCUMULATE);
        }
      }
      feid++;
      mask >>= 1;
    }
    ConvertPlanes(ev.GetBlock(0), planes);
    for (size_t i = 0; i < planes.feids.size(); ++i) {
      result.AddPlane(planes.planes[i]);
    }
    
    return true;
  }
  
  
  void APIXMCConverterPlugin::ConvertPlanes(const std::vector<unsigned char> & data, APIXPlanes & result) const {
    std::vector<APIXHit> hits = decodeData( data);
    for( unsigned int i =0; i < hits.size(); i++){
      APIXHit hit = hits.at(i);
      int index = result.Find(hit.chip);
      if (index == -1) {
        std::cerr << "APIX-MC-ConvertPlugin::ConvertPlanes: Bad index: " << index << std::endl;
      }
      else {
        if (UGLY_HACK && (result.feids[index] == 3 || result.feids[index] == 4 || result.feids[index] == 5 || result.feids[index] == 6)) {
          result.planes.at(index).PushPixel(hit.y, hit.x, hit.tot, false, hit.lv1);
        } else {
          result.planes.at(index).PushPixel(hit.x, hit.y, hit.tot, false, hit.lv1);
        }
        result.planes.at(index).SetTLUEvent(hit.eudetTrigger);
      }
    }
  } 

  unsigned int APIXMCConverterPlugin::getPLLTrig(unsigned int word) const {
    if((word & 0xf000000f) == 0x80000001) {
      return (word & 0x00ffff00) >> 8;
    }
    return 0xffffffff;
  }

  unsigned int APIXMCConverterPlugin::getEudetTrig(unsigned int word) const {
    //similar to: if(word & 0x0c000000) >> 26; case 0x1
    if(word & 0x04000000) {
      return word;
    }
    return 0xffffffff;
  }

  std::vector<APIXHit> APIXMCConverterPlugin::decodeData(const std::vector<unsigned char> & data) const{
    std::vector<APIXHit> hits;
    unsigned subtrigger = 0;
    unsigned pllTrig   = getPLLTrig(getlittleendian<unsigned int>(&data[0]));
    if(pllTrig == 0xFFFFFFFF) { std::cout << "ERROR APIX-Converter: first block is not PLL trigger" << std::endl;}
    unsigned eudetTrig   = getlittleendian<unsigned int>(&data[4]);
    if(getPLLTrig(eudetTrig) != 0xFFFFFFFF) { std::cout << "ERROR APIX-Converter: second block is PLL trigger" << std::endl;}
    for (size_t i = 16; i < data.size(); i += 4) {
      unsigned one_line = getlittleendian<unsigned int>(&data[i]);
      bool moduleError(false), moduleEoe(false);
      if((one_line>>25) & 0x1){
        moduleEoe = true;
        subtrigger++;
      }
      if((one_line>>26) & 0x1) { 
        moduleError = true;
      }
      //      if ( (one_line & 0x80000001) == 0x80000001 ) { continue;}
      if ( (one_line & 0x80000001) != 0x80000001 and (not moduleEoe) and (not moduleError) and (subtrigger < 16)) {
        //std::cout << "Found some data" << std::endl;
        APIXHit hit;
        hit.tot = (one_line) & 0xff;
        hit.x = (one_line >> 8) & 0x1f; //column
        hit.y = (one_line >>13) & 0xff; //row
        hit.lv1 = subtrigger;
        hit.chip = (one_line >> 21) & 0xf;
        hit.pllTrigger = pllTrig;
        hit.eudetTrigger = eudetTrig;
        hits.push_back(hit);
      }
    }
    return hits;
  }
} //namespace eudaq
