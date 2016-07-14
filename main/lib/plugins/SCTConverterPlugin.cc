#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/SCT_defs.hh"
#include "eudaq/Configuration.hh"
#include <algorithm>


#if USE_LCIO && USE_EUTELESCOPE
#include "IMPL/LCEventImpl.h"
#include "IMPL/TrackerRawDataImpl.h"
#include "IMPL/LCCollectionVec.h"
#include "lcio.h"
#include "EUTELESCOPE.h"
#include "EUTelRunHeaderImpl.h"
#include "EUTelSetupDescription.h"
#include "EUTelTrackerDataInterfacerImpl.h"
using eutelescope::EUTELESCOPE;
using eutelescope::EUTelTrackerDataInterfacerImpl;
using eutelescope::EUTelGenericSparsePixel;
#endif


namespace eudaq {
  static const uint32_t PLANE_ID_OFFSET_ABC = 10;
  static const uint32_t PLANE_ID_OFFSET_TTC = 30;
  static const char *EVENT_TYPE_ITS_ABC = "ITS_ABC";
  static const char *EVENT_TYPE_ITS_TTC = "ITS_TTC";
  static const char *LCIO_collection_name = "zsdata_strip";
  static const char *LCIO_collection_name_TTC = "zsdata_strip_TTC";

  namespace sct {
    std::string TDC_L0ID() { return "TDC.L0ID"; }
    std::string TLU_TLUID() { return "TLU.TLUID"; }
    std::string TDC_data() { return "TDC.data"; }
    std::string TLU_L0ID() { return "TLU.L0ID"; }
    std::string Timestamp_data() { return "Timestamp.data"; }
    std::string Timestamp_L0ID() { return "Timestamp.L0ID"; }
    std::string Event_L0ID() { return "Event.L0ID"; }
    std::string Event_BCID() { return "Event.BCID"; }
    std::string PTDC_TYPE() { return "PTDC.TYPE"; }
    std::string PTDC_L0ID() { return "PTDC.L0ID"; }
    std::string PTDC_TS() { return "PTDC.TS"; }
    std::string PTDC_BIT() { return "PTDC.BIT"; }
  }
  using namespace sct;
  
#if USE_LCIO && USE_EUTELESCOPE
  void add_data(lcio::LCEvent &result, int ID, double value) {
    result.parameters().setValue(eutelescope::EUTELESCOPE::EVENTTYPE,
                                 eutelescope::kDE);
    LCCollectionVec *zsDataCollection = nullptr;
    auto zsDataCollectionExists = Collection_createIfNotExist(
        &zsDataCollection, result, LCIO_collection_name_TTC);
    auto zsDataEncoder = CellIDEncoder<TrackerDataImpl>(
        eutelescope::EUTELESCOPE::ZSDATADEFAULTENCODING, zsDataCollection);
    zsDataEncoder["sensorID"] = ID;
    zsDataEncoder["sparsePixelType"] = eutelescope::kEUTelGenericSparsePixel;

    auto zsFrame =
        std::unique_ptr<lcio::TrackerDataImpl>(new lcio::TrackerDataImpl());
    zsDataEncoder.setCellID(zsFrame.get());
    zsFrame->chargeValues().resize(4);
    zsFrame->chargeValues()[0] = value;
    zsFrame->chargeValues()[1] = 1;
    zsFrame->chargeValues()[2] = 1;
    zsFrame->chargeValues()[3] = 0;
    zsDataCollection->push_back(zsFrame.release());

    if (!zsDataCollectionExists) {
      if (zsDataCollection->size() != 0)
        result.addCollection(zsDataCollection, LCIO_collection_name_TTC);
      else
        delete zsDataCollection;
    }
  }
  void add_TTC_data(lcio::LCEvent &result, const StandardEvent &tmp_evt) {
    add_data(result, PLANE_ID_OFFSET_TTC + 0, tmp_evt.GetTag(TDC_data(), -1));
  }
  void add_Timestamp_data(lcio::LCEvent &result, const StandardEvent &tmp_evt) {
    add_data(result, PLANE_ID_OFFSET_TTC + 1, tmp_evt.GetTag(Timestamp_data(), 0ULL));
  }
  void add_Timestamp_L0ID(lcio::LCEvent &result, const StandardEvent &tmp_evt) {
    add_data(result, PLANE_ID_OFFSET_TTC + 2, tmp_evt.GetTag(Timestamp_L0ID(), -1));
  }
  void add_TDC_L0ID(lcio::LCEvent &result, const StandardEvent &tmp_evt) {
    add_data(result, PLANE_ID_OFFSET_TTC + 3, tmp_evt.GetTag(TDC_L0ID(), -1));
  }
  void add_TLU_TLUID(lcio::LCEvent &result, const StandardEvent &tmp_evt) {
    add_data(result, PLANE_ID_OFFSET_TTC + 4, tmp_evt.GetTag(TLU_TLUID(), -1));
  }
  void add_PTDC_TYPE(lcio::LCEvent &result, const StandardEvent &tmp_evt) {
    add_data(result, PLANE_ID_OFFSET_TTC + 5, tmp_evt.GetTag(PTDC_TYPE(), -1));
  }
  void add_PTDC_L0ID(lcio::LCEvent &result, const StandardEvent &tmp_evt) {
    add_data(result, PLANE_ID_OFFSET_TTC + 6, tmp_evt.GetTag(PTDC_L0ID(), -1));
  }
  void add_PTDC_TS(lcio::LCEvent &result, const StandardEvent &tmp_evt) {
    add_data(result, PLANE_ID_OFFSET_TTC + 7, tmp_evt.GetTag(PTDC_TS(), -1));
  }
  void add_PTDC_BIT(lcio::LCEvent &result, const StandardEvent &tmp_evt) {
    add_data(result, PLANE_ID_OFFSET_TTC + 8, tmp_evt.GetTag(PTDC_BIT(), -1));
  }
#endif
  
  class SCTConverterPlugin_ITS_ABC : public DataConverterPlugin {
  public:
    virtual void Initialize(const Event &bore, const Configuration &cnf) {
      cnf.SetSection("Converter.ITS");
      m_swap_xy = cnf.Get("SWAP_XY", 0);
      m_abc_ids_types.clear();
      for(int i = 0;; i++){
	std::stringstream ss;
	ss <<"ABC_BLOCKID_"<<i;
	int id=bore.GetTag(ss.str().c_str(), -1);
	ss.str("");
	ss <<"ABC_BLOCKTYPE_"<<i;
	std::string type=bore.GetTag(ss.str().c_str(), std::string(""));
	if(id!=-1 && !type.empty()){
	  m_abc_ids_types[id] = type;
	}
	else
	  break;
      }
    }

    virtual int IsSyncWithTLU(eudaq::Event const &ev,
                              const eudaq::Event &tluEvent) const {
      unsigned triggerID = GetTriggerID(ev);
      auto tlu_triggerID = tluEvent.GetEventNumber();
      return compareTLU2DUT(tlu_triggerID, triggerID + 1);
    }

    virtual bool GetStandardSubEvent(StandardEvent &sev,
                                     const Event &ev) const {
      if (ev.IsBORE() || ev.IsEORE()) {
        return true;
      }
      auto raw = dynamic_cast<const RawDataEvent *>(&ev);
      if (!raw) {
        return false;
      }

      std::string tagname;
      tagname = "Temperature";
      if(raw->HasTag(tagname)){
	sev.SetTag(tagname,raw->GetTag(tagname));
      }
      tagname = "Voltage";
      if(raw->HasTag(tagname)){
	sev.SetTag(tagname,raw->GetTag(tagname));
      }
      
      std::vector<std::string> paralist = raw->GetTagList("PLOT_");
      for(auto &e: paralist){
	double val ;
	val=raw->GetTag(e, val);
	sev.SetTag(e,val);
      }

      size_t  nblocks= raw->NumBlocks();
      if(!nblocks){
	std::cout<< "SCTConverterPlugin: Warning, no block in "<<raw->GetSubType()
		 <<", event number = "<<raw->GetEventNumber()<<"  skipped"<<std::endl;
	return true;
      }
      
      for(size_t n = 0; n<nblocks; n++){
	std::vector<unsigned char> block = raw->GetBlock(n);
	unsigned  blockid = raw->GetID(n);
	std::string type;
	if(m_abc_ids_types.empty()){ //empty
	  type.assign("ABC");
	}
	else{
	  if(m_abc_ids_types.find(blockid)==m_abc_ids_types.end()){
	    continue; // not empty and not find
	  }
	  else{//find
	    type = m_abc_ids_types.find(blockid)->second;
	  }	
	}
	if(type.substr(0, 3)=="abc" || type.substr(0, 3)=="ABC"){
	  std::vector<bool> channels;
	  eudaq::uchar2bool(block.data(), block.data() + block.size(), channels);
	  std::string sensor_type;
	  StandardPlane plane(blockid, EVENT_TYPE_ITS_ABC, type.c_str());
	  if(m_swap_xy)
	    plane.SetSizeZS(1,channels.size(),0);
	  else
	    plane.SetSizeZS(channels.size(), 1, 0);
	  
	  unsigned x = 0;
	  for (size_t i = 0; i < channels.size(); ++i) {
	    if (channels[i] == true) {
	      if(m_swap_xy)
		plane.PushPixel(1, x, 1);
	      else
		plane.PushPixel(x, 1 , 1);
	    }
	    ++x;
	  }
	  sev.AddPlane(plane);
	}
      }
      return true;
    }
    
#if USE_LCIO && USE_EUTELESCOPE
    void GetLCIORunHeader(lcio::LCRunHeader &header,
                          eudaq::Event const & /*bore*/,
                          eudaq::Configuration const & /*conf*/) const {
      eutelescope::EUTelRunHeaderImpl runHeader(&header);
      runHeader.setDAQHWName(EUTELESCOPE::EUDRB);
      runHeader.setEUDRBMode("ZS");
      runHeader.setEUDRBDet("SCT");
      runHeader.setNoOfDetector(m_boards);
      std::vector<int> xMin(m_boards, 0), xMax(m_boards, 1280),
          yMin(m_boards, 0), yMax(m_boards, 4);
      runHeader.setMinX(xMin);
      runHeader.setMaxX(xMax);
      runHeader.setMinY(yMin);
      runHeader.setMaxY(yMax);
    }

    bool GetLCIOSubEvent(lcio::LCEvent &result, const Event &source) const {
      if (source.IsBORE() || source.IsEORE()) {
        return true;
      }
      result.parameters().setValue(eutelescope::EUTELESCOPE::EVENTTYPE,
                                   eutelescope::kDE);
      
      LCCollectionVec *zsDataCollection = nullptr;
      auto zsDataCollectionExists = Collection_createIfNotExist(
          &zsDataCollection, result, LCIO_collection_name);

      StandardEvent tmp_evt;
      GetStandardSubEvent(tmp_evt, source);
      size_t nplanes = tmp_evt.NumPlanes();
      for(size_t n =0; n<nplanes; n++){
	auto plane = tmp_evt.GetPlane(n);
	auto zsDataEncoder = CellIDEncoder<TrackerDataImpl>
	  (eutelescope::EUTELESCOPE::ZSDATADEFAULTENCODING, zsDataCollection);
	zsDataEncoder["sensorID"] = plane.ID() + PLANE_ID_OFFSET_ABC;
	zsDataEncoder["sparsePixelType"] = eutelescope::kEUTelGenericSparsePixel;
	auto zsFrame = std::unique_ptr<lcio::TrackerDataImpl>(new lcio::TrackerDataImpl());
	zsDataEncoder.setCellID(zsFrame.get());	
	ConvertPlaneToLCIOGenericPixel(plane, *zsFrame);
	zsDataCollection->push_back(zsFrame.release());	
      }
      if(!zsDataCollectionExists) {
	if(zsDataCollection->size() != 0)
	  result.addCollection(zsDataCollection, LCIO_collection_name);
	else
	  delete zsDataCollection; // clean up if not storing the collection here
      }
      return true;
    }
#endif

    static SCTConverterPlugin_ITS_ABC m_instance;
  private:
    SCTConverterPlugin_ITS_ABC() : DataConverterPlugin(EVENT_TYPE_ITS_ABC), m_swap_xy(0){}
    unsigned m_boards = 1;  //TODO:: remove
    unsigned m_swap_xy;
    std::map<int, std::string> m_abc_ids_types;
  }; // class SCTConverterPlugin
  
  SCTConverterPlugin_ITS_ABC SCTConverterPlugin_ITS_ABC::m_instance;


  
  using TriggerData_t = uint64_t;
  class SCTConverterPlugin_ITS_TTC : public DataConverterPlugin {
  public:
    virtual void Initialize(const Event &bore, const Configuration &cnf){}
    virtual int IsSyncWithTLU(eudaq::Event const &ev,
                              const eudaq::Event &tluEvent) const {
      unsigned triggerID = GetTriggerID(ev);
      auto tlu_triggerID = tluEvent.GetEventNumber();
      return compareTLU2DUT(tlu_triggerID, triggerID + 1);
    }

    virtual bool GetStandardSubEvent(StandardEvent &sev,
                                     const Event &ev) const {
      if (ev.IsBORE() ||ev.IsEORE()) {
        return true;
      }
      auto raw = dynamic_cast<const RawDataEvent *>(&ev);
      if (!raw) {
        return false;
      }
      auto block = raw->GetBlock(0);
      ProcessTTC(block, sev);
      return true;
    }
    
    template <typename T>
    static void ProcessTTC(const std::vector<unsigned char> &block, T &sev) {
      size_t size_of_TTC = block.size() / sizeof(TriggerData_t);
      const TriggerData_t *TTC = nullptr;
      if (size_of_TTC) {
        TTC = reinterpret_cast<const TriggerData_t *>(block.data());
      }
      size_t j = 0;
      for (size_t i = 0; i < size_of_TTC; ++i) {
        uint64_t data = TTC[i];
        switch (data >> 60) {
	case 0xc:
          ProcessPTDC_data(data, sev);
          break;
        case 0xd:
          ProcessTLU_data(data, sev);
          break;
        case 0xe:
          ProcessTDC_data(data, sev);
          break;
        case 0xf:
          ProcessTimeStamp_data(data, sev);
          break;
	case 0x0:
	  std::cout<<"[SCTConvertPlugin:] TTC type 0x00: maybe de-sync"<<std::endl;
	  break;
        default:
          EUDAQ_THROW("unknown data type");
          break;
        }
      }
    }
    
    template <typename T> static void ProcessPTDC_data(uint64_t data, T &sev) {
      uint32_t TYPE = (uint32_t)(data>>52)& 0xf;
      uint32_t L0ID = (uint32_t)(data>>48)& 0xf;
      uint32_t TS = (uint32_t)(data>>32) & 0xffff;
      uint32_t BIT = (uint32_t)data;
      sev.SetTag(PTDC_TYPE(), TYPE);
      sev.SetTag(PTDC_L0ID(), L0ID);
      sev.SetTag(PTDC_TS(), TS);
      sev.SetTag(PTDC_BIT(), BIT);      
    }
    
    template <typename T> static void ProcessTLU_data(uint64_t data, T &sev) {
      uint32_t hsioID = (uint32_t)(data >> 40) & 0xffff;
      uint64_t TLUID = data & 0xffff;
      sev.SetTag(TLU_TLUID(), TLUID);
      sev.SetTag(TLU_L0ID(), hsioID);
    }
    
    template <typename T> static void ProcessTDC_data(uint64_t data, T &sev) {
      uint32_t L0ID = (uint32_t)(data >> 40) & 0xffff;
      uint64_t TDC = data & 0xfffff;
      sev.SetTag(TDC_data(), TDC);
      sev.SetTag(TDC_L0ID(), L0ID);
    }
    
    template <typename T>
    static void ProcessTimeStamp_data(uint64_t data, T &sev) {
      uint64_t timestamp = data & 0x000000ffffffffffULL;
      uint32_t L0ID = (uint32_t)(data >> 40) & 0xffff;
      sev.SetTag(Timestamp_data(), timestamp);
      sev.SetTag(Timestamp_L0ID(), L0ID);
    }
    
#if USE_LCIO && USE_EUTELESCOPE
    void GetLCIORunHeader(lcio::LCRunHeader &header,
                          eudaq::Event const & /*bore*/,
                          eudaq::Configuration const & /*conf*/) const {
      eutelescope::EUTelRunHeaderImpl runHeader(&header);
      runHeader.setDAQHWName(EUTELESCOPE::EUDRB);
      runHeader.setEUDRBMode("ZS");
      runHeader.setEUDRBDet("SCT_TTC");
      runHeader.setNoOfDetector(m_boards);
      std::vector<int> xMin(m_boards, 0), xMax(m_boards, 1280),
          yMin(m_boards, 0), yMax(m_boards, 4);
      runHeader.setMinX(xMin);
      runHeader.setMaxX(xMax);
      runHeader.setMinY(yMin);
      runHeader.setMaxY(yMax);
    }

    bool GetLCIOSubEvent(lcio::LCEvent &result, const Event &source) const {
      if (source.IsBORE() || source.IsEORE()) {
        return true;
      }
      StandardEvent tmp_evt;
      GetStandardSubEvent(tmp_evt, source);
      add_TTC_data(result, tmp_evt);
      add_Timestamp_data(result, tmp_evt);
      add_Timestamp_L0ID(result, tmp_evt);
      add_TDC_L0ID(result, tmp_evt);
      add_TLU_TLUID(result, tmp_evt);
      add_PTDC_TYPE(result, tmp_evt);
      add_PTDC_L0ID(result, tmp_evt);
      add_PTDC_TS(result, tmp_evt);
      add_PTDC_BIT(result, tmp_evt);
      return true;
    }
#endif
    
    static SCTConverterPlugin_ITS_TTC m_instance;
  private:
    SCTConverterPlugin_ITS_TTC() : DataConverterPlugin(EVENT_TYPE_ITS_TTC) {}
    unsigned m_boards = 1;
  }; // class SCTConverterPlugin
  
  SCTConverterPlugin_ITS_TTC SCTConverterPlugin_ITS_TTC::m_instance;

} // namespace eudaq
