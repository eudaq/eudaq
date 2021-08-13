#include "eudaq/LCEventConverter.hh"
#include "eudaq/RawEvent.hh"

#include "IMPL/TrackerRawDataImpl.h"
#include "IMPL/TrackerDataImpl.h"
#include "UTIL/CellIDEncoder.h"

#include "ItsAbcCommon.h"

#include <regex>

class ItsAbcRawEvent2LCEventConverter: public eudaq::LCEventConverter{
  typedef std::vector<uint8_t>::const_iterator datait;
public:
  bool Converting(eudaq::EventSPC d1, eudaq::LCEventSP d2, eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("ITS_ABC");
  static const uint32_t m_id1_factory = eudaq::cstr2hash("ITS_ABC_DUT");
  static const uint32_t m_id2_factory = eudaq::cstr2hash("ITS_ABC_Timing");
  static const uint32_t PLANE_ID_OFFSET_ABC = 10;
private:
};

void addHit(lcio::TrackerDataImpl *zsFrame, int x, int y=0, int charge=1, int time=0);


namespace{
  auto dummy0 = eudaq::Factory<eudaq::LCEventConverter>::
    Register<ItsAbcRawEvent2LCEventConverter>(ItsAbcRawEvent2LCEventConverter::m_id_factory);
  auto dummy1 = eudaq::Factory<eudaq::LCEventConverter>::
    Register<ItsAbcRawEvent2LCEventConverter>(ItsAbcRawEvent2LCEventConverter::m_id1_factory);
  auto dummy2 = eudaq::Factory<eudaq::LCEventConverter>::
    Register<ItsAbcRawEvent2LCEventConverter>(ItsAbcRawEvent2LCEventConverter::m_id2_factory);
}

typedef std::map<uint32_t, std::pair<uint32_t, uint32_t>> BlockMap;
// Normally, will be exactly the same every event, so remember
static std::map<uint32_t, std::unique_ptr<BlockMap>> map_registry;

void addHit(lcio::TrackerDataImpl *zsFrame, int x, int y, int charge, int time) {
	zsFrame->chargeValues().push_back(x);//x
	zsFrame->chargeValues().push_back(y);//y
	zsFrame->chargeValues().push_back(charge);//signal
	zsFrame->chargeValues().push_back(time);//time
}


bool ItsAbcRawEvent2LCEventConverter::Converting(eudaq::EventSPC d1, eudaq::LCEventSP d2,
						 eudaq::ConfigSPC conf) const{
  auto raw = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  if (!raw){
    EUDAQ_THROW("dynamic_cast error: from eudaq::Event to eudaq::RawEvent");
  }

  std::string block_dsp = raw->GetTag("ABC_EVENT", "");
  if(block_dsp.empty()){
    return true;
  }


  uint32_t deviceId = d1->GetStreamN();
//  std::cout << "deviceId ABC\t" <<  deviceId << std::endl;
//block_map for the ITk strip timing plane
  ItsAbc::ItsAbcBlockMap AbcBlocks(block_dsp);
  std::map<uint32_t, std::pair<uint32_t, uint32_t>> block_map = AbcBlocks.getBlockMap();

  d2->parameters().setValue("EventType", 2);//TODO: remove
  lcio::LCCollectionVec *zsDataCollection = nullptr;
  auto p_col_names = d2->getCollectionNames();
  if(std::find(p_col_names->begin(), p_col_names->end(), "zsdata_strip") != p_col_names->end()){
    zsDataCollection = dynamic_cast<lcio::LCCollectionVec*>(d2->getCollection("zsdata_strip"));
    if(!zsDataCollection)
      EUDAQ_THROW("dynamic_cast error: from lcio::LCCollection to lcio::LCCollectionVec");
  }
  else{
    zsDataCollection = new lcio::LCCollectionVec(lcio::LCIO::TRACKERDATA);
    d2->addCollection(zsDataCollection, "zsdata_strip");
  }

  lcio::CellIDEncoder<lcio::TrackerDataImpl> superDataEncoder("sensorID:7,sparsePixelType:5", zsDataCollection);
  superDataEncoder["sensorID"] = 19+AbcBlocks.getOffset();
  superDataEncoder["sparsePixelType"] = 2; // Can this be fixed in initial instatiation
  auto superFrame = new lcio::TrackerDataImpl;
  superDataEncoder.setCellID(superFrame);
  std::vector<int> timingplane(282);

  auto block_n_list = raw->GetBlockNumList();
  for(auto &block_n: block_n_list){
    std::vector<uint8_t> block = raw->GetBlock(block_n);
    auto it = block_map.find(block_n);
    if(it == block_map.end()) {
      continue;
    }

    if(it->second.second<5){
      uint32_t strN = it->second.first;
      uint32_t bcN = it->second.second;
      uint32_t plane_id = PLANE_ID_OFFSET_ABC + bcN*10 + strN + AbcBlocks.getOffset(); 
      if(bcN==4){//only because im lazy and i dont want to remove the condition
        ItsAbc::RawInfo info(block);
        if(info.valid) {
          if(deviceId==301 && block_n == 6){  //only for LS star for now
            d2->parameters().setValue("DUT.RAWBCID", std::to_string(info.BCID)); //ABCStar -- needed for desync correction
            d2->parameters().setValue("DUT.RAWparity", std::to_string(info.BCID_parity)); //ABCStar
            d2->parameters().setValue("DUT.RAWL0ID", std::to_string(info.L0ID)); //ABCStar
          }
          if(deviceId==201 && block_n == 2){  //only for LS star for now
            d2->parameters().setValue("TIMING.RAWBCID", std::to_string(info.BCID)); //ABCStar -- needed for desync correction
            d2->parameters().setValue("TIMING.RAWparity", std::to_string(info.BCID_parity)); //ABCStar
            d2->parameters().setValue("TIMING.RAWL0ID", std::to_string(info.L0ID)); //ABCStar
          }
        }  
      } else {
        std::vector<bool> channels;
        eudaq::uchar2bool(block.data(), block.data() + block.size(), channels);
        lcio::CellIDEncoder<lcio::TrackerDataImpl> zsDataEncoder("sensorID:7,sparsePixelType:5", zsDataCollection);
        zsDataEncoder["sensorID"] = plane_id;
        zsDataEncoder["sparsePixelType"] = 2; // Can this be fixed in initial instatiation
        auto zsFrame = new lcio::TrackerDataImpl;
        zsDataEncoder.setCellID(zsFrame);
        if (deviceId <300) {
          if (strN == 0) {
            for(size_t i = 406; i < 681; ++i) {
              if(channels[i]) {
                if(i < 494) {
                  if(i > 480) {
                    timingplane[i-2*(i%2)-406] = 1;
                  } else {
                    timingplane[i-406] = 1;
                  }
                }
                if(i >= 494 && i<= 511) {
                  timingplane[i-2*(i%2)-406] = 1;
                }
                if(i>= 577){
                  timingplane[i-399] = 1;
                }
              }
            }
          } else {
            for(size_t i = 513; i < 616; ++i) {
              if(channels[i]) {
                timingplane[i-425] = 1; //+19 in index
              }
            }
          }
          for(size_t i = 0; i < channels.size(); ++i) {
            if(channels[i]){
              addHit(zsFrame, i);
            }
          }
        } else {
          for(size_t i = 0; i < channels.size(); ++i) {
            if(channels[i]){
              addHit(zsFrame, i);//r0
              if (strN<2){
              	addHit(superFrame, i, 1-strN);
              } else {
              	addHit(superFrame, 1279-i, strN);
              }
            }
          }
        }
        zsDataCollection->push_back(zsFrame);
      }
    }
  }
  if(deviceId < 300) {
    for (unsigned int i=0; i<timingplane.size(); i++) {
      if (timingplane[i]) {
        addHit(superFrame, i);
      }
    }
  }
  zsDataCollection->push_back(superFrame);

  if(block_n_list.empty()){
    EUDAQ_WARN("No block in ItsAbcRawEvent EventN:TriggerN (" +
	       std::to_string(raw->GetEventN())+":"+std::to_string(raw->GetTriggerN())+")");
  }
  return true; 
}
