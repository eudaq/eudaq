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
#  include "EUTelAPIXMCDetector.h"
#  include "EUTelSetupDescription.h"
#  include "EUTelEventImpl.h"
#  include "EUTelSparseDataImpl.h"
#  include "EUTelAPIXSparsePixel.h"
#include <list>
using eutelescope::EUTELESCOPE;
#endif

#include <iostream>
#include <string>
#include <vector>


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
  struct CTELHit {
    unsigned int module;
    unsigned int tot;
    unsigned int lv1;
    unsigned int x;
    unsigned int y;
    unsigned long long rceTrigger;
    unsigned int eudetTrigger;
  };
  
  class APIXCTConverterPlugin : public DataConverterPlugin {
  public:
    virtual void Initialize(const Event & e, const Configuration & c);
#if USE_LCIO && USE_EUTELESCOPE
    void ConvertLCIOHeader(lcio::LCRunHeader & header, eudaq::Event const & bore, eudaq::Configuration const & conf) const;
    virtual void GetLCIORunHeader(lcio::LCRunHeader & header, eudaq::Event const & bore, eudaq::Configuration const & conf) const {
      return ConvertLCIOHeader(header, bore, conf);
    }
    bool GetLCIOSubEvent(lcio::LCEvent & result, const Event & source) const;
#endif
    virtual unsigned GetTriggerID(eudaq::Event const &) const;
    virtual bool GetStandardSubEvent(StandardEvent &, const eudaq::Event &) const;

    private:
#if USE_LCIO && USE_EUTELESCOPE
#endif
    std::vector<CTELHit> decodeData(const RawDataEvent & event) const;
    void ConvertPlanes(const RawDataEvent & event, APIXPlanes & result) const;
    APIXCTConverterPlugin() : DataConverterPlugin("APIX-CT"),
			      m_PlaneMask(0), m_nFrames(1) {}
    unsigned m_PlaneMask;
    unsigned m_nFrames;
    
    static APIXCTConverterPlugin const m_instance;
  };
  
#if USE_LCIO && USE_EUTELESCOPE
  void APIXCTConverterPlugin::ConvertLCIOHeader(lcio::LCRunHeader & header, eudaq::Event const & , eudaq::Configuration const & ) const
  {
    eutelescope::EUTelRunHeaderImpl runHeader(&header);
  }
  
  
  bool APIXCTConverterPlugin::GetLCIOSubEvent(lcio::LCEvent & result, const Event & source) const
  {
    return true; //whatever... I guess this just breaks if event is not raw
  }


  

  
  /*!
   * Prepare a collection for a given feid
   * The feids are stored as separate collections in the LCIO file
   * I think this is a good idea, as they will be act as independent detectors in the tracking
   */
#endif

  APIXCTConverterPlugin const APIXCTConverterPlugin::m_instance;

  void APIXCTConverterPlugin::Initialize(const Event & source, const Configuration &) {
    m_PlaneMask = from_string(source.GetTag("PlaneMask"), 1);
    m_nFrames = from_string(source.GetTag("nFrames"), 1);
    //std::cout << " Nrows , NColumns = " << m_NumRows << "  ,  " <<  m_NumColumns << std::endl;
    //std::cout << " Initial row , column = " << m_InitialRow << "  ,  " <<  m_InitialColumn << std::endl;
  }

  bool APIXCTConverterPlugin::GetStandardSubEvent(StandardEvent & result, const Event & source) const {
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
        //std::cout << "APIX-CT plane " << feid << std::endl;
        planes.planes.push_back(StandardPlane(feid, "APIXCT", "APIXCT"));
        planes.feids.push_back(feid);
        planes.planes.back().SetSizeZS(18*8, 160*2, 0, m_nFrames, StandardPlane::FLAG_DIFFCOORDS | StandardPlane::FLAG_ACCUMULATE);
      }
      feid++;
      mask >>= 1;
    }
    ConvertPlanes(ev, planes);
    for (size_t i = 0; i < planes.feids.size(); ++i) {
      result.AddPlane(planes.planes[i]);
    }
   // std::cout << "End of GetStandardSubEvent" << std::endl;
    return true;
  }
  

  
  unsigned APIXCTConverterPlugin::GetTriggerID(eudaq::Event const & ev) const {
    const RawDataEvent & rev = dynamic_cast<const RawDataEvent &>(ev);

    if (rev.NumBlocks() < 2) return (unsigned)-1;
    unsigned eudetTrig = getbigendian<unsigned int>(&rev.GetBlock(0)[32]);
    return eudetTrig;

  }
  
  
  
  void APIXCTConverterPlugin::ConvertPlanes(const RawDataEvent & ev, APIXPlanes & result) const {
    std::vector<CTELHit> hits = decodeData( ev);
    for( unsigned int i =0; i < hits.size(); i++){
      CTELHit hit = hits.at(i);
      int index = result.Find(hit.module);
      if (index == -1) {
	std::cerr << "APIX-MC-ConvertPlugin::ConvertPlanes: Bad index: " << index << std::endl;
      }
      else {
	result.planes.at(index).PushPixel(hit.x, hit.y, hit.tot, false, hit.lv1);
      }
    }
  } 


  std::vector<CTELHit> APIXCTConverterPlugin::decodeData(const RawDataEvent & ev) const{
    std::vector<CTELHit> hits;
    eudaq::RawDataEvent::data_t block0=ev.GetBlock(0);
    unsigned long long rcetrig=getbigendian<unsigned long long>(&block0[16]);
    unsigned eudetTrig = getbigendian<unsigned int>(&block0[32]);
    eudaq::RawDataEvent::data_t block1=ev.GetBlock(1);
    unsigned current_header = 0;

    int first_bxid = 0;

    for (size_t i = 0; i < block1.size(); i += 4) {
      
      unsigned one_line = getbigendian<unsigned int>(&block1[i]);
      //std::cout<<std::hex<<one_line<<std::endl;

      if( (one_line & 0xFF000000) == 0x20000000){
	current_header = one_line;
	if(i==0){
	  first_bxid = (  current_header & 0x000000FF);
	} 
      }
      else if((one_line & 0xF0000000) == 0x80000000){


	//	std::cout<<"data block = "<<std::hex<<one_line<<" with header "<<current_header<<std::endl;

	unsigned int module, l1id;
	int bxid;  //channel number, level-1 id, beam-crossing id

	bxid = (  current_header & 0x000000FF);
	int bxdiff=bxid-first_bxid;
	if(bxdiff<0)bxdiff+=256;
	l1id = ( (current_header>>8) & 0x0000000F);
	module = ( (current_header>>16) & 0x0000000F);

	//	std::cout<<"channel = "<<std::hex<<channel<<" ; level-1 id = "<<l1id<<" ; beam-crossing id = "<<bxid<<std::endl;


	unsigned int feid, tot, col, row;
	row = (  one_line & 0x000000FF);
	col = ( (one_line>>8) & 0x000000FF);
	tot = ( (one_line>>16) & 0x000000FF);
	feid = ( (one_line>>24) & 0x0000000F);

	//	std::cout<<"feid = "<<std::hex<<feid<<" ; tot =  "<<tot<<" ; col = "<<col<<" ; row = "<<row<<std::endl;
	unsigned x, y;
	if(feid<8){
	  x=18*feid+col;
	  y=row;
	} else {
	  x=(15-feid)*18+17-col;
	  y= 319-row;
	}

	CTELHit hit;
	hit.tot = tot;
	hit.x = x;
	hit.y = y;
	hit.lv1 = bxdiff;
	hit.module = module;
	hit.rceTrigger = rcetrig;
	hit.eudetTrigger = eudetTrig;

	
	hits.push_back(hit);

      }
      else{
	std::cout<<"ERROR in CTELConverterPlugin:  invalid data block: "<<std::hex<<one_line<<std::endl;
	break;
      }

      

    }
    return hits;
  }
} //namespace eudaq
