#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/Exception.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"
#include <map>

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
  // The event type for which this converter plugin will be registered
  // Modify this to match your actual event type (from the Producer)
  static const char* EVENT_TYPE = "APIX-CT";
  static int dbg = 0;

  static const unsigned int NCOL=80;
  static const unsigned int NROW=336;
  static int chip_id_offset = 20;

  class FormattedRecord{
  public:
    struct Generic
    {
      
      unsigned int unused2: 29;
      unsigned int headerkey: 1;
      unsigned int headertwokey: 1;
      unsigned int datakey: 1;
      
    };
    struct Header
    {
      
      unsigned int bxid: 13;
      unsigned int l1id: 12;
      unsigned int link: 4;
      unsigned int key: 1;
      unsigned int unused1: 2;
      
    };
    
    
    struct HeaderTwo
    {

      unsigned int rce: 8;
      unsigned int unused2: 22;
      unsigned int key: 1;
      unsigned int unused1: 1;

    };


    struct Data
    {
      
      unsigned int row: 9;
      unsigned int col: 7;
      unsigned int tot: 8;
      unsigned int fe: 4;
      unsigned int unused1: 3;
      unsigned int key: 1;
      
    };

  
      union Record
      {
	unsigned int  ui;
	Generic       ge;
	Header        he;
	Data          da;
	HeaderTwo     ht;
      };

    enum type{HEADER=0x20000000, HEADERTWO=0x40000000, DATA=0x80000000};
    
    FormattedRecord(FormattedRecord::type ty){
      m_record.ui=ty;
    }
    FormattedRecord(unsigned& wd){
      m_record.ui=wd;
    }
    
    bool isHeader(){return m_record.ge.headerkey;}
    bool isHeaderTwo(){return m_record.ge.headertwokey;}
    bool isData(){return m_record.ge.datakey;}
    
    unsigned getLink(){return m_record.he.link;}
    void setLink(unsigned link){m_record.he.link=link;}
    unsigned getBxid(){return m_record.he.bxid;}
    void setBxid(unsigned bxid){m_record.he.bxid=bxid;}
    unsigned getL1id(){return m_record.he.l1id;}
    void setL1id(unsigned l1id){m_record.he.l1id=l1id;}
    
    unsigned getRCE(){return m_record.ht.rce;}
    void setRCE(unsigned rce){m_record.ht.rce=rce;}
    
    unsigned getFE(){return m_record.da.fe;}
    void setFE(unsigned fe){m_record.da.fe=fe;}
    unsigned getToT(){return m_record.da.tot;}
    void setToT(unsigned tot){m_record.da.tot=tot;}
    unsigned getCol(){return m_record.da.col;}
    void setCol(unsigned col){m_record.da.col=col;}
    unsigned getRow(){return m_record.da.row;}
    void setRow(unsigned row){m_record.da.row=row;}

    unsigned getWord(){return m_record.ui;}

  private:
    Record m_record;
  };

  /********************************************/

  struct CTELHit {
    unsigned int link;
    unsigned int tot;
    unsigned int lv1;
    unsigned int col;
    unsigned int row;
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
    void ConvertPlanes(const RawDataEvent & event, std::map<int, StandardPlane> & result) const;
    APIXCTConverterPlugin() : DataConverterPlugin(EVENT_TYPE), m_nFrames(1), \
                              m_feToSensorid(*new std::map<int, int>), 
                              m_fepos(*new std::map<int, int>), 
                              m_sensorids(*new std::vector<int>) {}
    virtual ~APIXCTConverterPlugin(){ std::cout<<"~APIXCTConverterPlugin done" << std::endl;}
//       delete &m_feToSensorid;
//       delete &m_fepos;
//       delete &m_sensorids;
//    }
    unsigned m_nFrames;
    std::map<int, int> &m_feToSensorid;
    std::map<int, int> &m_fepos;
    std::vector<int> &m_sensorids;
    
    static APIXCTConverterPlugin  m_instance;
  };



#if USE_LCIO && USE_EUTELESCOPE
  void APIXCTConverterPlugin::ConvertLCIOHeader(lcio::LCRunHeader & header, eudaq::Event const & , eudaq::Configuration const & ) const
  {
    eutelescope::EUTelRunHeaderImpl runHeader(&header);
  }
  
  
  bool APIXCTConverterPlugin::GetLCIOSubEvent(lcio::LCEvent & lcioEvent, const Event & eudaqEvent) const
  {
    if (eudaqEvent.IsBORE()) {
      if(dbg>0) std::cout << " APIXCTConverterPlugin::GetLCIOSubEvent BORE " << std::endl;
      // shouldn't happen
      return true;
    } else if (eudaqEvent.IsEORE()) {
      if(dbg>0) std::cout << " APIXCTConverterPlugin::GetLCIOSubEvent EORE " << std::endl;
      // nothing to do
      return true;
    }
    
    // set type of the resulting lcio event
    lcioEvent.parameters().setValue( eutelescope::EUTELESCOPE::EVENTTYPE, eutelescope::kDE );
    // pointer to collection which will store data
    LCCollectionVec * zsDataCollection;
    
    // it can be already in event or has to be created
    bool zsDataCollectionExists = false;
    try {
      zsDataCollection = static_cast< LCCollectionVec* > ( lcioEvent.getCollection( "zsdata_apix" ) );
      zsDataCollectionExists = true;
    } catch ( lcio::DataNotAvailableException& e ) {
      zsDataCollection = new LCCollectionVec( lcio::LCIO::TRACKERDATA );
    }
    
    //	create cell encoders to set sensorID and pixel type
    CellIDEncoder< TrackerDataImpl > zsDataEncoder   ( eutelescope::EUTELESCOPE::ZSDATADEFAULTENCODING, zsDataCollection  );
    
    // this is an event as we sent from Producer
    // needs to be converted to concrete type RawDataEvent
    const RawDataEvent & ev_raw = dynamic_cast <const RawDataEvent &> (eudaqEvent);
    
    std::vector< eutelescope::EUTelSetupDescription * >  setupDescription;
    std::vector<CTELHit> hits = decodeData( ev_raw);
    for (size_t sensor=0;sensor<m_sensorids.size();sensor++){
      if (lcioEvent.getEventNumber() == 0) {
	eutelescope::EUTelPixelDetector * currentDetector = new eutelescope::EUTelAPIXMCDetector(2);
	currentDetector->setMode( "ZS" );
	
	setupDescription.push_back( new eutelescope::EUTelSetupDescription( currentDetector )) ;
      }
      
      std::list<eutelescope::EUTelAPIXSparsePixel*> tmphits;
      
      zsDataEncoder["sensorID"] = m_sensorids[sensor] + chip_id_offset;
      zsDataEncoder["sparsePixelType"] = eutelescope::kEUTelAPIXSparsePixel;
      
      // prepare a new TrackerData object for the ZS data
      // it contains all the hits for a particular sensor in one event
      std::auto_ptr<lcio::TrackerDataImpl > zsFrame( new lcio::TrackerDataImpl );
      // set some values of "Cells" for this object
      zsDataEncoder.setCellID( zsFrame.get() );
      
      // this is the structure that will host the sparse pixel
      // it helps to decode (and later to decode) parameters of all hits (x, y, charge, ...) to
      // a single TrackerData object (zsFrame) that will correspond to a single sensor in one event
      std::auto_ptr< eutelescope::EUTelSparseDataImpl< eutelescope::EUTelAPIXSparsePixel > >
	sparseFrame( new eutelescope::EUTelSparseDataImpl< eutelescope::EUTelAPIXSparsePixel > ( zsFrame.get() ) );
      
      for (size_t i=0;i<hits.size();i++){
	if(m_feToSensorid[hits[i].link]==m_sensorids[sensor]){
	  int col=m_fepos[hits[i].link]*NCOL+hits[i].col; //left or right on 2-chip module
	  eutelescope::EUTelAPIXSparsePixel *thisHit = new eutelescope::EUTelAPIXSparsePixel( col, hits[i].row, hits[i].tot, m_sensorids[sensor] + chip_id_offset, hits[i].lv1-1);
	  sparseFrame->addSparsePixel( thisHit );
	  tmphits.push_back( thisHit );
	}
      }
      
      // write TrackerData object that contains info from one sensor to LCIO collection
      zsDataCollection->push_back( zsFrame.release() );
      
      // clean up
      for( std::list<eutelescope::EUTelAPIXSparsePixel*>::iterator it = tmphits.begin(); it != tmphits.end(); it++ ){
	delete (*it);
      }
    }
    
    // add this collection to lcio event
    if ( ( !zsDataCollectionExists )  && ( zsDataCollection->size() != 0 ) ) lcioEvent.addCollection( zsDataCollection, "zsdata_apix" );
    
    if (lcioEvent.getEventNumber() == 0) {
      // do this only in the first event
      LCCollectionVec * apixSetupCollection = NULL;
      
      bool apixSetupExists = false;
      try {
	apixSetupCollection = static_cast< LCCollectionVec* > ( lcioEvent.getCollection( "apix_setup" ) ) ;
	apixSetupExists = true;
      } catch (...) {
	apixSetupCollection = new LCCollectionVec( lcio::LCIO::LCGENERICOBJECT );
      }
      
      for ( size_t iPlane = 0 ; iPlane < setupDescription.size() ; ++iPlane ) {
	apixSetupCollection->push_back( setupDescription.at( iPlane ) );
      }
      
      if (!apixSetupExists) lcioEvent.addCollection( apixSetupCollection, "apix_setup" );
    }
    return true;
    
  }
  
  /*!
   * Prepare a collection for a given feid
   * The feids are stored as separate collections in the LCIO file
   * I think this is a good idea, as they will be act as independent detectors in the tracking
   */
#endif

  void APIXCTConverterPlugin::Initialize(const Event & source, const Configuration &) {
    int nFrontends = from_string(source.GetTag("nFrontends"), 0);
    m_nFrames = from_string(source.GetTag("consecutive_lvl1"), 1);
    char tagname[128];
    m_feToSensorid.clear();
    m_fepos.clear();
    m_sensorids.clear();
    for(int i=0;i<nFrontends;i++){
      sprintf(tagname, "OutLink_%d", i);
      int link=from_string(source.GetTag(tagname), 0);
      sprintf(tagname, "SensorId_%d", i);
      int sid=from_string(source.GetTag(tagname), 0);
      sprintf(tagname, "Position_%d", i);
      int pos=from_string(source.GetTag(tagname), 0);
      m_feToSensorid[link]=sid;
      m_fepos[link]=pos;
      bool found=false;
      for(size_t j=0;j<m_sensorids.size();j++){
	if(m_sensorids[j]==sid){
           found=true;
           break;
        }
      }
      if(found==false)m_sensorids.push_back(sid);
    }
    std::cout<<nFrontends<<" frontends and "<<m_sensorids.size()<<" sensors."<<std::endl;
    //std::cout << " Nrows , NColumns = " << m_NumRows << "  ,  " <<  m_NumColumns << std::endl;
    //std::cout << " Initial row , column = " << m_InitialRow << "  ,  " <<  m_InitialColumn << std::endl;
  }

  bool APIXCTConverterPlugin::GetStandardSubEvent(StandardEvent & result, const Event & source) const {
    if (source.IsBORE()) {
      if(dbg>0) std::cout << " APIXCTConverterPlugin::GetStandardSubEvent BORE " << std::endl;
      // shouldn't happen
      return true;
    } else if (source.IsEORE()) {
      if(dbg>0) std::cout << " APIXCTConverterPlugin::GetStandardLCIOSubEvent EORE " << std::endl;
      // nothing to do
      return true;
    }
    // If we get here it must be a data event
    const RawDataEvent & ev = dynamic_cast<const RawDataEvent &>(source);

    unsigned eudetTrig = getlittleendian<unsigned int>(&ev.GetBlock(0)[36])&0x7fff;
    //std::cout<<"eudet ID "<<eudetTrig<<std::endl;
    std::map<int, StandardPlane> planes; //sensor id to plane
    for(size_t i=0;i<m_sensorids.size();i++){
      int sensorid=m_sensorids[i];
      StandardPlane plane(sensorid, EVENT_TYPE, "APIXCT");
      plane.SetSizeZS(2*NCOL, NROW, 0, m_nFrames, StandardPlane::FLAG_DIFFCOORDS | StandardPlane::FLAG_ACCUMULATE);
      plane.SetTLUEvent(eudetTrig);
      planes[sensorid]=plane;
    }
    ConvertPlanes(ev, planes);
    for (size_t i = 0; i < m_sensorids.size(); i++) {
      int sensorid=m_sensorids[i];
      result.AddPlane(planes[sensorid]);
    }
   // std::cout << "End of GetStandardSubEvent" << std::endl;
    return true;
  }
  

  
  unsigned APIXCTConverterPlugin::GetTriggerID(eudaq::Event const & ev) const {
    const RawDataEvent & rev = dynamic_cast<const RawDataEvent &>(ev);
    if (rev.IsBORE() || rev.IsEORE()) return 0;

    if (rev.NumBlocks() <1) return (unsigned)-1;
    unsigned eudetTrig = getlittleendian<unsigned int>(&rev.GetBlock(0)[36])&0x7fff;
    return eudetTrig;

  }
  
  
  void APIXCTConverterPlugin::ConvertPlanes(const RawDataEvent & ev, std::map<int, StandardPlane>& result) const {
    std::vector<CTELHit> hits = decodeData( ev);
    for( unsigned int i =0; i < hits.size(); i++){
      CTELHit hit = hits.at(i);
      int sid=m_feToSensorid[hit.link];
      if (result.find(sid)==result.end()) {
	std::cerr << "APIX-MC-ConvertPlugin::ConvertPlanes: Bad index: " << sid << std::endl;
      }
      else {
	int col=m_fepos[hit.link]*NCOL+hit.col; //left or right on 2-chip module
	result[sid].PushPixel(col, hit.row, hit.tot, false, hit.lv1);
      }
    }
  } 

  std::vector<CTELHit> APIXCTConverterPlugin::decodeData(const RawDataEvent & ev) const{
    std::vector<CTELHit> hits;
    eudaq::RawDataEvent::data_t block0=ev.GetBlock(0);
    unsigned long long rcetrig=getlittleendian<unsigned long long>(&block0[20]);
    unsigned long long deadtime=getlittleendian<unsigned long long>(&block0[28]);
    unsigned eudetTrig = getlittleendian<unsigned int>(&block0[36])&0x7fff;
    unsigned triggerword = (getlittleendian<unsigned int>(&block0[36])&0xff0000)>>16;
    unsigned hitbusword = (getlittleendian<unsigned int>(&block0[36])&0xf000000)>>24;
    //std::cout<<"TLU word: "<<eudetTrig<<std::endl;
    //std::cout<<"Trigger word: "<<triggerword<<std::endl;
    //std::cout<<"Hitbus word: "<<hitbusword<<std::endl;
    //std::cout<<"Trigger time: "<<rcetrig<<std::endl;
    //std::cout<<"Deadtime: "<<deadtime<<std::endl;
    if(rcetrig==0 && triggerword==0){
      std::cout<<"Event not valid. Not filling Planes."<<std::endl;
      return hits;
    }
    eudaq::RawDataEvent::data_t pixblock=ev.GetBlock(1);
    int oldl1id[16];
    int link=-1;
    int l1id;
    int ntrg;
    int bxid;
    int firstbxid[16];
    int oldbxid[16];
    int bxdiff;
    for (int i=0;i<16;i++){
      oldl1id[i]=-1;
      firstbxid[i]=-1;
    }
    unsigned int indx=0;
    while (indx<pixblock.size()){
      unsigned currentu=getlittleendian<unsigned>(&pixblock[indx]);
      FormattedRecord current(currentu);
      if(current.isHeader()){
	ntrg++; //really, this is the total number of triggers summed over all the modules
	link=current.getLink();
	l1id=current.getL1id();
	bxid=current.getBxid()&0xff;
	//std::cout<<"l1id "<<l1id<<" ; bxid "<<bxid<<" ; link "<<link<<std::endl;
	if(oldl1id[link]!=l1id){
	  oldl1id[link]=l1id;
	  firstbxid[link]=bxid;
	}
	bxdiff=bxid-firstbxid[link];
	if(bxdiff<0)bxdiff+=256;
      }else if(current.isHeaderTwo()){
	int rce = current.getRCE();
	//std::cout<<"RCE header. Number is "<<rce<<std::endl; 
      }else if(current.isData()){
	//int chip=current.getFE();
	int tot=current.getToT();
	int col=current.getCol();
	int row=current.getRow();
	CTELHit hit;
	hit.tot = tot;
	hit.col = col;
	hit.row = row;
	hit.lv1 = bxdiff;
	hit.link = link;
	hit.rceTrigger = rcetrig;
	hit.eudetTrigger = eudetTrig;
	hits.push_back(hit);
      } else{
	std::cout<<"ERROR in CTELConverterPlugin:  invalid data block. "<<std::endl;
	break;
      }
      indx+=4;
    }
    return hits;
  }

  // Instantiate the converter plugin instance
  APIXCTConverterPlugin APIXCTConverterPlugin::m_instance;


} //namespace eudaq
