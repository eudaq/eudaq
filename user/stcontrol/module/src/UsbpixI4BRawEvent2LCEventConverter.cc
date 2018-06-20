#include "eudaq/LCEventConverter.hh"
#include "eudaq/RawEvent.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Logger.hh"


#include "IMPL/TrackerRawDataImpl.h"
#include "IMPL/TrackerDataImpl.h"
#include "UTIL/CellIDEncoder.h"

#include "ATLASFE4IInterpreter.hh"
#include <cstdlib>
#include <cstring>
#include <exception>

class UsbpixI4BRawEvent2LCEventConverter: public eudaq::LCEventConverter{
  typedef std::vector<uint8_t>::const_iterator datait;
public:
  bool Converting(eudaq::EventSPC d1, eudaq::LCEventSP d2, eudaq::ConfigurationSPC conf) const override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("USBPIXI4B");
private:

  uint32_t getWord(const std::vector<uint8_t>& data, size_t index) const;
  eudaq::StandardPlane ConvertPlane(const std::vector<uint8_t> & data, uint32_t id) const;
  bool isEventValid(const std::vector<uint8_t> & data) const;
  bool getHitData(uint32_t &Word, bool second_hit,
		  uint32_t &Col, uint32_t &Row, uint32_t &ToT) const;
  uint32_t getTrigger(const std::vector<uint8_t> & data) const;

  static const uint32_t CHIP_MIN_COL = 1;
  static const uint32_t CHIP_MAX_COL = 80;
  static const uint32_t CHIP_MIN_ROW = 1;
  static const uint32_t CHIP_MAX_ROW = 336;
  static const uint32_t CHIP_MAX_ROW_NORM = CHIP_MAX_ROW - CHIP_MIN_ROW;
  static const uint32_t CHIP_MAX_COL_NORM = CHIP_MAX_COL - CHIP_MIN_COL;
  uint32_t consecutive_lvl1 = 16;
  
  static ATLASFEI4Interpreter<0x00007C00, 0x000003FF> fei4b_intp;


  uint32_t first_sensor_id = 0;
  uint32_t chip_id_offset = 20;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::LCEventConverter>::
    Register<UsbpixI4BRawEvent2LCEventConverter>(UsbpixI4BRawEvent2LCEventConverter::m_id_factory);
}

ATLASFEI4Interpreter<0x00007C00, 0x000003FF> UsbpixI4BRawEvent2LCEventConverter::fei4b_intp;

bool UsbpixI4BRawEvent2LCEventConverter::
Converting(eudaq::EventSPC d1, eudaq::LCEventSP d2, eudaq::ConfigurationSPC conf)const {
  auto& lcioEvent = *(d2.get());
  lcioEvent.parameters().setValue("EventType",2); //( eutelescope::EUTELESCOPE::EVENTTYPE, eutelescope::kDE )
  //pointer to collection which will store data
  lcio::LCCollectionVec * zsDataCollection;
  //it can be already in event or has to be created
  bool zsDataCollectionExists = false;
  try{
    zsDataCollection = static_cast< lcio::LCCollectionVec* > ( lcioEvent.getCollection( "zsdata_apix" ) );
    zsDataCollectionExists = true;
  }
  catch( lcio::DataNotAvailableException& e ){
    zsDataCollection = new lcio::LCCollectionVec( lcio::LCIO::TRACKERDATA );
  }

  //create cell encoders to set sensorID and pixel type
  lcio::CellIDEncoder<lcio::TrackerDataImpl> zsDataEncoder("sensorID:7,sparsePixelType:5", zsDataCollection);
  //( eutelescope::EUTELESCOPE::ZSDATADEFAULTENCODING, zsDataCollection  )

  //this is an event as we sent from Producer, needs to be converted to concrete type RawEvent
  auto ev_raw = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);

  auto block_n_list = ev_raw->GetBlockNumList();
  for(auto &chip: block_n_list){
    auto buffer = ev_raw->GetBlock(chip);
    int sensorID  = chip + chip_id_offset + first_sensor_id;

    lcio::TrackerDataImpl *zsFrame = new lcio::TrackerDataImpl;

    zsDataEncoder["sensorID"] = sensorID;
    zsDataEncoder["sparsePixelType"] = 2;//eutelescope::kEUTelGenericSparsePixel
    zsDataEncoder.setCellID(zsFrame);

    uint32_t ToT = 0;
    uint32_t Col = 0;
    uint32_t Row = 0;
    uint32_t lvl1 = 0;

    for(size_t i=0; i < buffer.size()-4; i += 4){
      uint32_t Word = getWord(buffer, i);
      if(fei4b_intp.is_dh(Word)){
	lvl1++;
      }
      else{
	//First Hit
	if(getHitData(Word, false, Col, Row, ToT)){
	  zsFrame->chargeValues().push_back(Col);//x
	  zsFrame->chargeValues().push_back(Row);//y
	  zsFrame->chargeValues().push_back(ToT);//signal
	  zsFrame->chargeValues().push_back(lvl1-1);//time
	}
	//Second Hit
	if(getHitData(Word, true, Col, Row, ToT)){
	  zsFrame->chargeValues().push_back(Col);
	  zsFrame->chargeValues().push_back(Row);
	  zsFrame->chargeValues().push_back(ToT);
	  zsFrame->chargeValues().push_back(lvl1-1);
	}
      }
    }
    zsDataCollection->push_back( zsFrame);
  }
  if((!zsDataCollectionExists)  && ( zsDataCollection->size() != 0 ) ){
    lcioEvent.addCollection( zsDataCollection, "zsdata_apix" );
  }
}

bool UsbpixI4BRawEvent2LCEventConverter::
isEventValid(const std::vector<uint8_t> & data) const{
  //ceck data consistency
  uint32_t dh_found = 0;
  for (size_t i=0; i < data.size()-8; i += 4){
    uint32_t word = getWord(data, i);
    if(fei4b_intp.is_dh(word)){
      dh_found++;
    }
  }
  if(dh_found != consecutive_lvl1){
    return false;
  }
  else{
    return true;
  }
}


bool UsbpixI4BRawEvent2LCEventConverter::
getHitData(uint32_t &Word, bool second_hit,
	   uint32_t &Col, uint32_t &Row, uint32_t &ToT) const{
  //No data record
  if( !fei4b_intp.is_dr(Word)){
    return false;
  }
  uint32_t t_Col=0;
  uint32_t t_Row=0;
  uint32_t t_ToT=15;

  if(!second_hit){
    t_ToT = fei4b_intp.get_dr_tot1(Word);
    t_Col = fei4b_intp.get_dr_col1(Word);
    t_Row = fei4b_intp.get_dr_row1(Word);
  }
  else{
    t_ToT = fei4b_intp.get_dr_tot2(Word);
    t_Col = fei4b_intp.get_dr_col2(Word);
    t_Row = fei4b_intp.get_dr_row2(Word);
  }

  //translate FE-I4 ToT code into tot
  //tot_mode = 0
  if (t_ToT==14 || t_ToT==15)
    return false;
  ToT = t_ToT + 1;

  if(t_Row > CHIP_MAX_ROW || t_Row < CHIP_MIN_ROW){
    std::cout << "Invalid row: " << t_Row << std::endl;
    return false;
  }
  if(t_Col > CHIP_MAX_COL || t_Col < CHIP_MIN_COL){
    std::cout << "Invalid col: " << t_Col << std::endl;
    return false;
  }
  //Normalize Pixelpositions
  t_Col -= CHIP_MIN_COL;
  t_Row -= CHIP_MIN_ROW;
  Col = t_Col;
  Row = t_Row;
  return true;
}

uint32_t UsbpixI4BRawEvent2LCEventConverter::
getTrigger(const std::vector<uint8_t> & data) const{
  //Get Trigger Number and check for errors
  uint32_t i = data.size() - 8; //splitted in 2x 32bit words
  uint32_t Trigger_word1 = getWord(data, i);

  if(Trigger_word1==(uint32_t) -1){
    return (uint32_t)-1;
  }
  uint32_t Trigger_word2 = getWord(data, i+4);
  uint32_t trigger_number = fei4b_intp.get_tr_no_2(Trigger_word1, Trigger_word2);
  return trigger_number;
}

uint32_t UsbpixI4BRawEvent2LCEventConverter::
getWord(const std::vector<uint8_t>& data, size_t index) const{
  return (((uint32_t)data[index + 3]) << 24) | (((uint32_t)data[index + 2]) << 16)
    | (((uint32_t)data[index + 1]) << 8) | (uint32_t)data[index];
}
